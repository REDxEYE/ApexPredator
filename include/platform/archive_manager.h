// Created by RED on 02.10.2025.

#ifndef APEXPREDATOR_ARCHIVE_MANAGER_H
#define APEXPREDATOR_ARCHIVE_MANAGER_H

#include "platform/archive.h"
#include "utils/dynamic_array.h"

DYNAMIC_ARRAY_STRUCT(Archive*, ArchivePtr);

typedef struct {
    DynamicArray_ArchivePtr archives;
} ArchiveManager;

void ArchiveManager_init(ArchiveManager *manager);

void ArchiveManager_add(ArchiveManager *manager, Archive *archive);

void ArchiveManager_get_file(ArchiveManager *manager, String *path, MemoryBuffer *mb);

void ArchiveManager_get_file_by_hash(ArchiveManager *manager, uint32 path, MemoryBuffer *mb);

bool ArchiveManager_has_file(ArchiveManager *manager, String *path);

bool ArchiveManager_has_file_by_hash(ArchiveManager *manager, uint32 hash);

void ArchiveManager_free(ArchiveManager *manager);

#endif //APEXPREDATOR_ARCHIVE_MANAGER_H
