// Created by RED on 18.09.2025.

#include "apex/package/tab_archive.h"
#include "utils/buffer/file_buffer.h"

#include <assert.h>
#include <stdio.h>

bool TabArchive__has_file(const TabArchive *ar, const String *path);

bool TabArchive__has_file_by_hash(const TabArchive *ar, uint32 hash);

bool TabArchive__get_file(TabArchive *ar, const String *path, MemoryBuffer *mb);

bool TabArchive__get_file_by_hash(TabArchive *ar, uint32 hash, MemoryBuffer *mb);

const String *TabArchive__get_name(TabArchive *ar);

void TabArchive__free(TabArchive *ar);

void TabArchive__open(TabArchive *ar, String *path);

const String *TabArchive__get_name(TabArchive *ar) {
    return &ar->tab_path;
}

void TabArchive_get_all_entries(TabArchive *ar, DynamicArray_ArchiveEntry *entries) {
    for (uint32 i = 0; i < ar->entries.values.count; ++i) {
        const TabEntry *tab_entry = DM_get_value(&ar->entries, i);
        ArchiveEntry *entry = DA_append_get(entries);
        entry->path_hash = tab_entry->hash;
        entry->path = NULL;
        entry->archive = (Archive*)ar;
        entry->size = tab_entry->size;
    }
}

void TabArchive__init_interface(TabArchive *ar) {
    ar->has_file = (ArchiveHasFileFn) TabArchive__has_file;
    ar->has_file_by_hash = (ArchiveHasFileByHashFn) TabArchive__has_file_by_hash;
    ar->get_file = (ArchiveGetFileFn) TabArchive__get_file;
    ar->get_file_by_hash = (ArchiveGetFileByHashFn) TabArchive__get_file_by_hash;
    ar->get_name = (ArchiveGetNameFn) TabArchive__get_name;
    ar->get_all_entries = (ArchiveGetAllEntriesFn) TabArchive_get_all_entries;
    ar->free = (ArchiveFreeFn) TabArchive__free;
}

TabArchive *TabArchive_new(String *path) {
    TabArchive *ar = malloc(sizeof(TabArchive));
    if (ar == NULL) {
        printf("Failed to allocate memory for TabArchive\n");
        exit(1);
    }
    memset(ar, 0, sizeof(TabArchive));
    TabArchive__init_interface(ar);
    TabArchive__open(ar, path);
    return ar;
}

void TabArchive__open(TabArchive *ar, String *path) {
    String_copy_from(&ar->tab_path, path);
    int32 dot_pos = String_find_chr(path, '.');
    assert(dot_pos>0 && "Invalid .tab file path");
    String_sub_string(&ar->tab_path, 0, dot_pos, &ar->arc_path);
    String_append_cstr2(&ar->arc_path, ".arc", 4);

    FileBuffer tab_buffer = {0};
    if (FileBuffer_open_read(&tab_buffer, String_data(path)) != BUFFER_SUCCESS) {
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
        *(TabEntry *) DM_insert(&ar->entries, entry.hash) = entry;
    }
    tab_buffer.close(&tab_buffer);
}

const TabEntry *Archive__find_entry(const TabArchive *ar, uint32 hash) {
    return DM_get(&ar->entries, hash);
}

bool TabArchive__get_file(TabArchive *ar, const String *path, MemoryBuffer *mb) {
    uint32 hash = path_hash(path);
    return TabArchive__get_file_by_hash(ar, hash, mb);
}

bool TabArchive__get_file_by_hash(TabArchive *ar, uint32 key, MemoryBuffer *mb) {
    FileBuffer arc_file = {0};
    FileBuffer_open_read(&arc_file, String_data(&ar->arc_path));
    const TabEntry *entry = Archive__find_entry(ar, key);
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

bool TabArchive__has_file(const TabArchive *ar, const String *path) {
    uint32 hash = path_hash(path);
    return DM_get(&ar->entries, hash) != NULL;
}

bool TabArchive__has_file_by_hash(const TabArchive *ar, uint32 hash) {
    return DM_get(&ar->entries, hash) != NULL;
}

void TabArchive__free(TabArchive *ar) {
    String_free(&ar->tab_path);
    String_free(&ar->arc_path);
    DM_free(&ar->entries);
}
