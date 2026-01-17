// Created by RED on 02.10.2025.

#include "platform/archive_manager.h"

#include "platform/logger.h"
#include "utils/hash_helper.h"

void ArchiveManager_init(ArchiveManager *manager) {
    DA_init(&manager->archives, Archive*, 4);
}

void ArchiveManager_add(ArchiveManager *manager, Archive *archive) {
    *(Archive **) DA_append_get(&manager->archives) = archive;
}

bool ArchiveManager_get_file(ArchiveManager *manager, const String *path, MemoryBuffer *mb) {
    uint32 hash = hash_string(path);
    for (uint32 i = 0; i < manager->archives.count; ++i) {
        Archive *ar = manager->archives.items[i];
        if (Archive_get_file_by_hash(ar, hash, mb)) {
            // printf("[INFO]: File \"%s\" found in archive \"%s\"\n", String_data(path),
            // String_data(Archive_get_name(ar)));
            return true;
        }
    }
    GLog_Error("File \"%s\" not found in any archive", String_data(path));
    return false;
}

bool ArchiveManager_get_file_by_hash(ArchiveManager *manager, const uint32 path, MemoryBuffer *mb) {
    for (uint32 i = 0; i < manager->archives.count; ++i) {
        Archive *ar = manager->archives.items[i];
        if (Archive_get_file_by_hash(ar, path, mb)) {
            // printf("[INFO]: File with hash %08X found in archive \"%s\"\n", path, String_data(Archive_get_name(ar)));
            return true;
        }
    }
    GLog_Error("File with hash %08X not found in any archive", path);
    return false;
}

bool ArchiveManager_has_file(const ArchiveManager *manager, const String *path) {
    uint32 hash = hash_string(path);
    for (uint32 i = 0; i < manager->archives.count; ++i) {
        const Archive *ar = manager->archives.items[i];
        if (Archive_has_file_by_hash(ar, hash)) {
            return true;
        }
    }
    return false;
}

bool ArchiveManager_has_file_by_hash(const ArchiveManager *manager, uint32 hash) {
    for (uint32 i = 0; i < manager->archives.count; ++i) {
        const Archive *ar = manager->archives.items[i];
        if (Archive_has_file_by_hash(ar, hash)) {
            return true;
        }
    }
    return false;
}

void ArchiveManager_get_all_entries(const ArchiveManager *manager, DynamicArray_ArchiveEntry *entries) {
    DA_init(entries, ArchiveEntry, 16);
    for (uint32 i = 0; i < manager->archives.count; ++i) {
        const Archive *ar = manager->archives.items[i];
        Archive_get_all_entries(ar, entries);
    }
}

void ArchiveManager_foreach_file(const ArchiveManager *manager,
                                 const foreach_callback callback,
                                 void *user_data) {
    for (uint32 i = 0; i < manager->archives.count; ++i) {
        const Archive *ar = manager->archives.items[i];
        if (!Archive_foreach_file(ar, callback, user_data)) {
            break;
        }
    }
}

void ArchiveManager_free(ArchiveManager *manager) {
    DA_free_with_inner(&manager->archives, {
                       Archive** ar= it;
                       Archive_free(*ar);
                       free(*ar);
                       });
}
