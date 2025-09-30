// Created by RED on 28.09.2025.

#ifndef APEXPREDATOR_ARCHIVES_H
#define APEXPREDATOR_ARCHIVES_H

#include "apex/package//archive.h"
#include "utils/dynamic_array.h"

DYNAMIC_ARRAY_STRUCT(Archive, Archive);

typedef struct {
    DynamicArray_Archive archives;
} Archives;

void Archives_init(Archives *ars, String *game_root);
void Archives_free(Archives *ars);
bool Archives_find_hash(Archives *ars, uint32 hash, MemoryBuffer *mb);
bool Archives_find_path(Archives *ars, const String *path, MemoryBuffer *mb);

uint32 path_hash(const char *str);
#endif //APEXPREDATOR_ARCHIVES_H
