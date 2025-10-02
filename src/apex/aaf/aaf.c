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
    for (uint32 i = 0; i < archive->header.section_count; ++i) {
        AAFSection *entry = DA_append_get(&archive->sections);
        buffer->read(buffer, &entry->header, sizeof(AAFSectionHeader), NULL);
        MemoryBuffer_allocate(&entry->buffer, entry->header.compressed_size);
        buffer->read(buffer, entry->buffer.data, entry->header.compressed_size, NULL);
    }
}

bool AAFArchive_get_section(AAFArchive *archive, uint32 index, MemoryBuffer *out) {
    //Sections are zlib compressed
    if (index >= archive->sections.count) return false;
    AAFSection *section = DA_at(&archive->sections, index);
    if (section->buffer.size == 0 || section->buffer.data == NULL) return false;
    if (MemoryBuffer_allocate(out, section->header.uncompressed_size) != BUFFER_SUCCESS) {
        return false;
    }

    int res = inflate_exact_raw(section->buffer.data, section->buffer.size, out->data, out->size, NULL, NULL);
    if (res != Z_OK) {
        printf("[ERROR]: Failed to decompress AAF section %u, zlib error: %d\n", index, res);
        out->close(out);
        return false;
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
