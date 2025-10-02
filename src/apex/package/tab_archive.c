// Created by RED on 18.09.2025.

#include "apex/package/tab_archive.h"
#include "utils/buffer/file_buffer.h"

#include <assert.h>
#include <stdio.h>

void TabArchive_open(TabArchive *ar, String *path) {
    String_copy_from(&ar->tab_path, path);
    int32 dot_pos = String_find_chr(path, '.');
    assert(dot_pos>0 && "Invalid .tab file path");
    String_sub_string(&ar->tab_path, 0, dot_pos, &ar->arc_path);
    String_append_cstr2(&ar->arc_path, ".arc", 4);

    FileBuffer tab_buffer={0};
    if(FileBuffer_open_read(&tab_buffer, String_data(path))!=BUFFER_SUCCESS) {
        printf("Failed to open tab file %s\n", String_data(path));
        return;
    }
    DM_init(&ar->entries, TabEntry, 128);

    TabHeader header;
    tab_buffer.read(&tab_buffer, &header, sizeof(header),NULL);
    TabEntry entry;

    BufferError error;
    uint32 entry_count = Buffer_remaining((Buffer *) &tab_buffer, &error) / sizeof(TabEntry);
    for (int i = 0; i < entry_count; i++) {
        error = tab_buffer.read(&tab_buffer, &entry, sizeof(TabEntry), NULL);
        if (error < BUFFER_FAILED) {
            printf("Failed to read entry %d\n", i);
            return;
        }
        *(TabEntry*)DM_insert(&ar->entries, entry.hash) = entry;
    }
    tab_buffer.close(&tab_buffer);
}

TabEntry *Archive__find_entry(TabArchive *ar, uint32 hash) {
    return DM_get(&ar->entries, hash);
}

bool TabArchive_get_data(TabArchive *ar, uint32 key, MemoryBuffer *mb) {
    FileBuffer arc_file = {0};
    FileBuffer_open_read(&arc_file, String_data(&ar->arc_path));
    TabEntry *entry = Archive__find_entry(ar, key);
    if (entry == NULL) {
        arc_file.close(&arc_file);
        return false;
    }

    arc_file.set_position(&arc_file, entry->offset, BUFFER_ORIGIN_START);
    if (MemoryBuffer_allocate(mb, entry->size) != BUFFER_SUCCESS) {
        arc_file.close(&arc_file);
        return false;
    }
    uint32 read_bytes = 0;
    if (arc_file.read(&arc_file, mb->data, entry->size, &read_bytes) != BUFFER_SUCCESS) {
        arc_file.close(&arc_file);
        return false;
    }
    if (read_bytes != entry->size) {
        arc_file.close(&arc_file);
        return false;
    }
    arc_file.close(&arc_file);
    return true;
}

bool TabArchive_has_hash(const TabArchive *ar, uint32 hash) {
    return DM_get(&ar->entries, hash) != NULL;
}

void TabArchive_free(TabArchive *ar) {
    String_free(&ar->tab_path);
    String_free(&ar->arc_path);
    DM_free(&ar->entries);
}
