// Created by RED on 02.10.2025.

#include "apex/sarc.h"

#include "utils/hash_helper.h"

void SArchive__from_buffer(SArchive *archive, Buffer *buffer);

static const String internal_sarc_name_ = {
    .buffer = "SARC",
    .size = 4,
    .capacity = 5,
    .statically_allocated = 1,
    .can_be_moved = 0,
    .heap_allocated = 0
};

bool SArchive__has_file(const SArchive *archive, const String *path) {
    uint32 hash = hash_string(path);
    return DM_get(&archive->entries, hash) != NULL;
}

bool SArchive__has_file_by_hash(const SArchive *archive, uint32 hash) {
    return DM_get(&archive->entries, hash) != NULL;
}

bool SArchive__get_file_by_hash(SArchive *archive, uint32 hash, MemoryBuffer *out);

void SArchive__free(SArchive *archive);

bool SArchive__get_file(SArchive *archive, const String *path, MemoryBuffer *out) {
    uint32 hash = hash_string(path);
    return SArchive__get_file_by_hash(archive, hash, out);
}

bool SArchive__get_file_by_hash(SArchive *archive, uint32 hash, MemoryBuffer *out) {
    const SArcEntry *entry = DM_get(&archive->entries, hash);
    if (entry == NULL) return false;
    uint64 buffer_size = 0;
    if (archive->buffer->getsize(archive->buffer, &buffer_size) != BUFFER_SUCCESS)return false;
    if (entry->offset + entry->size > buffer_size) {
        printf("[ERROR]: Invalid SARC entry size\n");
        return false;
    }
    MemoryBuffer_allocate(out, entry->size);
    archive->buffer->set_position(archive->buffer, entry->offset, BUFFER_ORIGIN_START);
    uint32 actuallyRead = 0;
    archive->buffer->read(archive->buffer, out->data, entry->size, &actuallyRead);
    if (actuallyRead != entry->size) {
        printf("[ERROR]: Failed to read SARC entry data, expected size: %u, actual size: %u\n", entry->size,
               actuallyRead);
        out->close(out);
        return false;
    }
    return true;
}

const String *SArchive__get_name(SArchive *archive) {
    return &internal_sarc_name_;
}

void SArchive__get_all_entries(const SArchive *archive, DynamicArray_ArchiveEntry *out) {
    for (int i = 0; i < archive->entries.values.count; ++i) {
        SArcEntry *entry = DA_at(&archive->entries.values, i);
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

void SArchive_print_files(SArchive *archive) {
    DynamicArray_ArchiveEntry entries = {0};
    DA_init(&entries, ArchiveEntry, 16);
    SArchive__get_all_entries(archive, &entries);
    for (int i = 0; i < entries.count; ++i) {
        ArchiveEntry *entry = DA_at(&entries, i);
        printf("File: %s, Size: %u, Hash: 0x%08X\n", String_data(entry->path), entry->size,
               entry->path_hash);
    }
    DA_free(&entries);
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
}

SArchive *SArchive_new(Buffer *buffer) {
    SArchive *archive = malloc(sizeof(SArchive));
    if (archive == NULL) {
        printf("[ERROR]: Failed to allocate memory\n");
        exit(1);
    }
    memset(archive, 0, sizeof(SArchive));
    SArchive__init_interface(archive);
    SArchive__from_buffer(archive, buffer);
    return archive;
}

void SArchive__from_buffer(SArchive *archive, Buffer *buffer) {
    DM_init(&archive->entries, SArcEntry, 16);
    archive->buffer = buffer;
    buffer->read(buffer, &archive->header, sizeof(SArcHeader), NULL);
    if (memcmp(archive->header.ident, "SARC", 4) != 0) {
        printf("Invalid SARC magic\n");
        return;
    }
    if (archive->header.version2 == 2) {
        printf("[ERROR] SARC version 2 is not supported\n");
        exit(1);
    } else if (archive->header.version2 == 3) {
        uint32 strings_size = 0;
        buffer->read_uint32(buffer, &strings_size);
        char *strings_memory = malloc(strings_size);
        archive->strings = strings_memory;
        buffer->read(buffer, strings_memory, strings_size, NULL);
        uint32 entry_count = (archive->header.dir_block_len - 4/* strings_size int */ - strings_size) / 20;
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
                printf("[ERROR]: SARC entry hash mismatch for file %s, expected: %08X, actual: %08X\n",
                       String_data(&entry.name), entry.hash, hash_string(&entry.name));
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
        free(archive->strings);
        archive->strings = NULL;
    }
}
