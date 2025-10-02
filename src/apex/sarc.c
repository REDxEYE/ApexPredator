// Created by RED on 02.10.2025.

#include "apex/sarc.h"

void SArchive_from_buffer(SArchive *archive, Buffer *buffer) {
    memset(archive, 0, sizeof(*archive));
    DA_init(&archive->entries, SArcEntry, 16);
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
        uint32 entry_count = (archive->header.dir_block_len - 4/*strings_size int*/ - strings_size) / 20;
        for (uint32 i = 0; i < entry_count; ++i) {
            SArcEntry *entry = DA_append_get(&archive->entries);
            uint32 name_offset;
            buffer->read_uint32(buffer, &name_offset);
            String_from_cstr(&entry->name, &strings_memory[name_offset]);
            buffer->read_uint32(buffer, &entry->offset);
            buffer->read_uint32(buffer, &entry->size);
            buffer->read_uint32(buffer, &entry->hash);
            buffer->read_uint32(buffer, &entry->ext_hash);
        }
    }
}

void SArchive_free(SArchive *archive) {
    DA_free_with_inner(&archive->entries, {
        String_free(&((SArcEntry*)it)->name);
    });
    if (archive->strings) {
        free(archive->strings);
        archive->strings = NULL;
    }
}
