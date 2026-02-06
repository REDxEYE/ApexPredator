// Created by RED on 02.10.2025.

#include "apex/sarc.h"

#include "apex/hashes.h"
#include "platform/logger.h"
#include "utils/memory_profiling.h"
#include "utils/hash_helper.h"

void SArchive__from_buffer(SArchive *archive, Buffer *buffer);

bool SArchive__has_file(const SArchive *archive, const String *path) {
    const uint32 hash = hash_string(path);
    return DM_get(&archive->entries, hash) != NULL;
}

bool SArchive__has_file_by_hash(const SArchive *archive, uint32 hash) {
    return DM_get(&archive->entries, hash) != NULL;
}

bool SArchive__get_file_by_hash(const SArchive *archive, uint32 hash, MemoryBuffer *out);

void SArchive__free(SArchive *archive);

bool SArchive__get_file(const SArchive *archive, const String *path, MemoryBuffer *out) {
    const uint32 hash = hash_string(path);
    return SArchive__get_file_by_hash(archive, hash, out);
}

bool SArchive__get_file_by_hash(const SArchive *archive, const uint32 hash, MemoryBuffer *out) {
    const SArcEntry *entry = DM_get(&archive->entries, hash);
    if (entry == NULL) return false;
    uint64 buffer_size = 0;
    if (archive->buffer->getsize(archive->buffer, &buffer_size) != BUFFER_SUCCESS)return false;
    if (entry->offset + entry->size > buffer_size) {
        GLog_Error("Invalid SARC entry size");
        return false;
    }
    MemoryBuffer_allocate(out, entry->size);
    archive->buffer->set_position(archive->buffer, entry->offset, BUFFER_ORIGIN_START);
    uint32 actuallyRead = 0;
    archive->buffer->read(archive->buffer, out->data, entry->size, &actuallyRead);
    if (actuallyRead != entry->size) {
        GLog_Error("Failed to read SARC entry data, expected size: %u, actual size: %u", entry->size,
               actuallyRead);
        out->close(out);
        return false;
    }
    return true;
}

const String *SArchive__get_name(SArchive *archive) {
    String* name = find_name32(archive->hash);
    if (name==NULL) {
        name = String_new(32);
        String_format(name, "SARC 0x%08X", archive->hash);
    }
    return name;
}

void SArchive__get_all_entries(const SArchive *archive, DynamicArray_ArchiveEntry *out) {
    for (int i = 0; i < archive->entries.values.count; ++i) {
        const SArcEntry *entry = DA_at(&archive->entries.values, i);
        if (entry->offset == 0) {
            continue;
        }
        ArchiveEntry *out_entry = DA_append_get(out);
        out_entry->path = &entry->name;
        out_entry->size = entry->size;
        out_entry->path_hash = entry->hash;
        out_entry->archive = (Archive *) archive;
    }
}

void SArchive_print_files(const SArchive *archive) {
    DynamicArray_ArchiveEntry entries = {0};
    DA_init(&entries, ArchiveEntry, 16);
    SArchive__get_all_entries(archive, &entries);
    for (int i = 0; i < entries.count; ++i) {
        const ArchiveEntry *entry = DA_at(&entries, i);
        GLog_Info("File: %s, Size: %u, Hash: 0x%08X", String_cstr(entry->path), entry->size,
               entry->path_hash);
    }
    DA_free(&entries);
}

uint32 SArchive__get_hash(const SArchive *archive) {
    return archive->hash;
}

void SArchive__init_interface(SArchive *archive) {
    archive->has_file = (ArchiveHasFileFn) SArchive__has_file;
    archive->has_file_by_hash = (ArchiveHasFileByHashFn) SArchive__has_file_by_hash;
    archive->get_file = (ArchiveGetFileFn) SArchive__get_file;
    archive->get_file_by_hash = (ArchiveGetFileByHashFn) SArchive__get_file_by_hash;
    archive->get_all_entries = (ArchiveGetAllEntriesFn) SArchive__get_all_entries;
    archive->get_name = (ArchiveGetNameFn) SArchive__get_name;
    archive->print_all_files = (ArchivePrintAllFilesFn) SArchive_print_files;
    archive->free = (ArchiveFreeFn) SArchive__free;
    archive->get_hash = (ArchiveGetHashFn) SArchive__get_hash;
}

SArchive *SArchive_new(Buffer *buffer, uint32 self_hash) {
    SArchive *archive = mp_calloc(sizeof(SArchive),1);
    if (archive == NULL) {
        GLog_Error("Failed to allocate memory");
        exit(1);
    }
    SArchive_init(archive, buffer, self_hash);
    return archive;
}

void SArchive_init(SArchive* archive, Buffer *buffer, uint32 self_hash) {
    SArchive__init_interface(archive);
    archive->hash = self_hash;
    SArchive__from_buffer(archive, buffer);
}

void SArchive__from_buffer(SArchive *archive, Buffer *buffer) {
    DM_init(&archive->entries, SArcEntry, 16);
    archive->buffer = buffer;
    buffer->read(buffer, &archive->header, sizeof(SArcHeader), NULL);
    if (memcmp(archive->header.ident, "SARC", 4) != 0) {
        GLog_Error("Invalid SARC magic");
        return;
    }
    if (archive->header.version2 == 2) {
        GLog_Error(" SARC version 2 is not supported");
        exit(1);
    } else if (archive->header.version2 == 3) {
        uint32 strings_size = 0;
        buffer->read_uint32(buffer, &strings_size);
        char *strings_memory = mp_malloc(strings_size);
        archive->strings = strings_memory;
        buffer->read(buffer, strings_memory, strings_size, NULL);
        const uint32 entry_count = (archive->header.dir_block_len - 4/* strings_size int */ - strings_size) / 20;
        for (uint32 i = 0; i < entry_count; ++i) {
            SArcEntry entry = {0};
            uint32 name_offset;
            buffer->read_uint32(buffer, &name_offset);
            String_from_cstr(&entry.name, &strings_memory[name_offset]);
            buffer->read_uint32(buffer, &entry.offset);
            buffer->read_uint32(buffer, &entry.size);
            buffer->read_uint32(buffer, &entry.hash);
            buffer->read_uint32(buffer, &entry.ext_hash);
            if (hash_string(&entry.name) != entry.hash) {
                GLog_Error("SARC entry hash mismatch for file %s, expected: %08X, actual: %08X",
                       String_cstr(&entry.name), entry.hash, hash_string(&entry.name));
                exit(1);
            }
            SArcEntry *slot = DM_insert(&archive->entries, entry.hash);
            *slot = entry;
        }
    }
}

void SArchive__free(SArchive *archive) {
    for (int i = 0; i < archive->entries.values.count; ++i) {
        SArcEntry *entry = DA_at(&archive->entries.values, i);
        String_free(&entry->name);
    }
    DM_free(&archive->entries);
    archive->buffer->close(archive->buffer);
    if (archive->strings) {
        mp_free(archive->strings);
        archive->strings = NULL;
    }
}
