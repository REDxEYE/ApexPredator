// Created by RED on 01.10.2025.

#include "apex/aaf/aaf.h"
#include "utils/zlib_wrapper.h"

void AAFArchive_from_buffer(AAFArchive *archive, Buffer *buffer) {
    buffer->read(buffer, &archive->header, sizeof(AAFHeader), NULL);
    if (strncmp(archive->header.ident, "AAF", 3) != 0) {
        printf("[ERROR]: Invalid AAF format\n");
        exit(1);
    }
    if (archive->header.version != 1) {
        printf("[ERROR]: Unsupported AAF version: %d\n", archive->header.version);
        exit(1);
    }
    DA_init(&archive->sections, AAFSection, archive->header.section_count);
    uint64 total_size = 0;
    int64 entry_offset = 0;
    buffer->get_position(buffer, &entry_offset);
    for (uint32 i = 0; i < archive->header.section_count; ++i) {
        buffer->set_position(buffer, entry_offset, BUFFER_ORIGIN_START);
        AAFSection *entry = DA_append_get(&archive->sections);
        buffer->read(buffer, &entry->header, sizeof(AAFSectionHeader), NULL);
        if (memcmp(entry->header.magic,"EWAM",4)!=0) {
            printf("[ERROR]: Invalid AAF section magic\n");
            exit(1);
        }
        MemoryBuffer_allocate(&entry->buffer, entry->header.compressed_size);
        buffer->read(buffer, entry->buffer.data, entry->header.compressed_size, NULL);
        total_size += entry->header.uncompressed_size;
        entry_offset+=entry->header.total_size;
    }
    if (total_size != archive->header.uncompressed_size) {
        printf("[ERROR]: AAF archive uncompressed size mismatch, expected: %u, actual: %llu\n",
               archive->header.uncompressed_size, total_size);
        exit(1);
    }
}

bool AAFArchive_get_data(AAFArchive *archive, MemoryBuffer *out) {
    if (MemoryBuffer_allocate(out, archive->header.uncompressed_size) != BUFFER_SUCCESS) {
        return false;
    }
    uint64 offset = 0;
    for (int index = 0; index < archive->header.section_count; ++index) {
        const AAFSection *section = DA_at(&archive->sections, index);
        if (section->buffer.size == 0 || section->buffer.data == NULL) return false;


        int res = inflate_exact_raw(section->buffer.data, section->buffer.size, out->data + offset,
                                    section->header.uncompressed_size, NULL, NULL);
        if (res != Z_OK) {
            printf("[ERROR]: Failed to decompress AAF section %u, zlib error: %d\n", index, res);
            out->close(out);
            return false;
        }
        offset += section->header.uncompressed_size;
    }

    return true;
}

void AAFArchive_free(AAFArchive *archive) {
    for (uint32 i = 0; i < archive->sections.count; ++i) {
        AAFSection *section = DA_at(&archive->sections, i);
        section->buffer.close(&section->buffer);
    }
    DA_free(&archive->sections);
}
