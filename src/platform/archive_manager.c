// Created by RED on 02.10.2025.

#include "platform/archive_manager.h"

#include "apex/package/archives.h"

void ArchiveManager_init(ArchiveManager *manager) {
    DA_init(&manager->archives, Archive*, 4);
}

void ArchiveManager_add(ArchiveManager *manager, Archive *archive) {
    *(Archive **) DA_append_get(&manager->archives) = archive;
}

void ArchiveManager_get_file(ArchiveManager *manager, String *path, MemoryBuffer *mb) {
    uint32 hash = path_hash(path);
    for (uint32 i = 0; i < manager->archives.count; ++i) {
        Archive *ar = manager->archives.items[i];
        if (Archive_get_file_by_hash(ar, hash, mb)) {
            printf("[INFO]: File \"%s\" found in archive \"%s\"\n", String_data(path), String_data(Archive_get_name(ar)));
            return;
        }
    }
    printf("[ERROR]: File \"%s\" not found in any archive\n", String_data(path));
}

void ArchiveManager_get_file_by_hash(ArchiveManager *manager, uint32 path, MemoryBuffer *mb) {
    for (uint32 i = 0; i < manager->archives.count; ++i) {
        Archive *ar = manager->archives.items[i];
        if (Archive_get_file_by_hash(ar, path, mb)) {
            printf("[INFO]: File with hash %08X found in archive \"%s\"\n", path, String_data(Archive_get_name(ar)));
            return;
        }
    }
    printf("[ERROR]: File with hash %08X not found in any archive\n", path);
}

void ArchiveManager_free(ArchiveManager *manager) {
    DA_free_with_inner(&manager->archives, {
                       Archive_free(it);
                       free(it);
                       });
}
