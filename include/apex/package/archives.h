// Created by RED on 28.09.2025.

#ifndef APEXPREDATOR_ARCHIVES_H
#define APEXPREDATOR_ARCHIVES_H

#include "apex/package/tab_archive.h"
#include "utils/dynamic_array.h"

DYNAMIC_ARRAY_STRUCT(TabArchive, TabArchive);

typedef struct {
    TabEntry *info;
    TabArchive *archive;
}ArchiveEntryInfo;

DYNAMIC_ARRAY_STRUCT(ArchiveEntryInfo, ArchiveEntryInfo);

typedef struct {
    DynamicArray_TabArchive archives;
} Archives;

void Archives_init(Archives *ars, String *game_root);
void Archives_free(Archives *ars);
bool Archives_find_hash(const Archives *ars, uint32 hash, MemoryBuffer *mb);
bool Archives_find_path(const Archives *ars, const String *path, MemoryBuffer *mb);
bool Archives_has_path(const Archives *ars, const String *path);
bool Archives_has_hash(const Archives *ars, uint32 hash);
DynamicArray_ArchiveEntryInfo* Archives_get_all_entries(const Archives* ars);

uint32 path_hash(const String *str);
#endif //APEXPREDATOR_ARCHIVES_H
