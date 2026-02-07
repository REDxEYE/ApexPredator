// Created by RED on 02.10.2025.

#ifndef APEXPREDATOR_ARCHIVE_MANAGER_H
#define APEXPREDATOR_ARCHIVE_MANAGER_H

#include "platform/archive.h"
#include "utils/dynamic_array.h"
#include "utils/string_view.h"

DYNAMIC_ARRAY_STRUCT(Archive*, ArchivePtr);

typedef struct ArchiveManager ArchiveManager;

typedef void(*load_archive_fn)(const ArchiveManager* manager, uint32 hash);

typedef struct ArchiveManager{
    DynamicArray_ArchivePtr archives;
    load_archive_fn load_archive;
} ArchiveManager;


void ArchiveManager_init(ArchiveManager *manager);

bool ArchiveManager_mounted(const ArchiveManager *manager, uint32 hash);

void ArchiveManager_set_archive_loader_function(ArchiveManager *manager, load_archive_fn func);

void ArchiveManager_add(const ArchiveManager *manager, Archive *archive);

bool ArchiveManager_get_file(const ArchiveManager *manager, StringView path, MemoryBuffer *mb);

bool ArchiveManager_get_file_by_hash(const ArchiveManager *manager, uint32 path, MemoryBuffer *mb);

bool ArchiveManager_has_file(const ArchiveManager *manager, StringView path);

bool ArchiveManager_has_file_by_hash(const ArchiveManager *manager, uint32 hash);

void ArchiveManager_get_all_entries(const ArchiveManager* manager, DynamicArray_ArchiveEntry* entries);

void ArchiveManager_foreach_file(const ArchiveManager *manager, foreach_callback callback, void *user_data);

void ArchiveManager_free(ArchiveManager *manager);

#endif //APEXPREDATOR_ARCHIVE_MANAGER_H
