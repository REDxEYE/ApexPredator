// Created by RED on 01.10.2025.

#include "apex/aaf/aaf.h"

#include "platform/logger.h"
#include "utils/zlib_wrapper.h"

void AAFArchive_from_buffer(AAFArchive *archive, Buffer *buffer) {
    TracyCZoneN(ctx, "AAFArchive_from_buffer", 1);
    buffer->read(buffer, &archive->header, sizeof(AAFHeader), NULL);
    if (strncmp(archive->header.ident, "AAF", 3) != 0) {
        GLog_Error("Invalid AAF format");
        abort();
    }
    if (archive->header.version != 1) {
        GLog_Error("Unsupported AAF version: %d", archive->header.version);
        abort();
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
            GLog_Error("Invalid AAF section magic");
            abort();
        }
        MemoryBuffer_allocate(&entry->buffer, entry->header.compressed_size);
        buffer->read(buffer, entry->buffer.data, entry->header.compressed_size, NULL);
        total_size += entry->header.uncompressed_size;
        entry_offset+=entry->header.total_size;
    }
    if (total_size != archive->header.uncompressed_size) {
        GLog_Error("AAF archive uncompressed size mismatch, expected: %u, actual: %llu",
               archive->header.uncompressed_size, total_size);
        abort();
    }
    TracyCZoneEnd(ctx);
}

bool AAFArchive_get_data(AAFArchive *archive, MemoryBuffer *out) {
    TracyCZoneN(ctx, "AAFArchive_get_data", 1);
    if (MemoryBuffer_allocate(out, archive->header.uncompressed_size) != BUFFER_SUCCESS) {
        return false;
    }
    uint64 offset = 0;
    for (int index = 0; index < archive->header.section_count; ++index) {
        const AAFSection *section = DA_at(&archive->sections, index);
        if (section->buffer.size == 0 || section->buffer.data == NULL) return false;


        const int res = inflate_exact_raw(section->buffer.data, section->buffer.size, out->data + offset,
                                    section->header.uncompressed_size, NULL, NULL);
        if (res != Z_OK) {
            GLog_Error("Failed to decompress AAF section %u, zlib error: %d", index, res);
            out->close(out);
            TracyCZoneEnd(ctx);
            return false;
        }
        offset += section->header.uncompressed_size;
    }

    TracyCZoneEnd(ctx);
    return true;
}

void AAFArchive_free(AAFArchive *archive) {
    for (uint32 i = 0; i < archive->sections.count; ++i) {
        AAFSection *section = DA_at(&archive->sections, i);
        section->buffer.close(&section->buffer);
    }
    DA_free(&archive->sections);
}
