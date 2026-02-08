// Created by RED on 02.10.2025.

#include "platform/archive_manager.h"

#include "apex/hashes.h"
#include "platform/logger.h"
#include "utils/memory_profiling.h"
#include "utils/hash_helper.h"
#include "tracy/TracyC.h"

void ArchiveManager_init(ArchiveManager *manager) {
    DA_init(&manager->archives, Archive*, 4);
}

void ArchiveManager_set_archive_loader_function(ArchiveManager *manager, const load_archive_fn func) {
    manager->load_archive = func;
}

bool ArchiveManager_mounted(const ArchiveManager *manager, const uint32 hash) {
    for (uint32 i = 0; i < manager->archives.count; ++i) {
        const Archive *ar = manager->archives.items[i];
        uint32 ar_hash = 0;
        if (ar->get_hash != NULL) {
            ar_hash = ar->get_hash(ar);
        }
        if (ar_hash == hash) {
            return true;
        }
    }
    return false;
}

void ArchiveManager_add(const ArchiveManager *manager, Archive *archive) {
    *(Archive **) DA_append_get(&manager->archives) = archive;
}

// #define VERBOSE_ENSURE_PARENTS_LOADED

void ensure_parents_loaded(const ArchiveManager *manager, const uint32 file_hash) {
    for (uint32 i = 0; i < manager->archives.count; ++i) {
        const Archive *ar = manager->archives.items[i];
        if (Archive_has_file_by_hash(ar, file_hash)) {
            return;
        }
    }
    uint64 parent_id = 0;
#ifdef VERBOSE_ENSURE_PARENTS_LOADED
    String* out_name;
    if (!get_file_parent(file_hash, &parent_id, &out_name)) {
        GLog_Warning("Failed to get parent for file hash 0x%08X", file_hash);
        return;
    }
    String* parent_name =NULL;
    if (parent_id!=0)
        parent_name = find_name32((uint32) parent_id);

    if (out_name!=NULL) {
        GLog_Info("File \"%s\" not found, trying to load parent archive \"%s\"",
                  String_cstr(out_name),
                  parent_name!=NULL ? String_cstr(parent_name) : "unknown");
        String_free(out_name);
    } else {
        GLog_Info("File with hash 0x%08X not found, trying to load parent archive \"%s\"",
                  file_hash,
                  parent_name!=NULL ? String_cstr(parent_name) : "unknown");
    }

    if (parent_id == 0) {
        return;
    }
    String_free(parent_name);
#else
    if (!get_file_parent(file_hash, &parent_id, NULL)) {
        GLog_Warning("Failed to get parent for file hash 0x%08X", file_hash);
        return;
    }

    if (parent_id == 0) {
        return;
    }
#endif

    if (manager->load_archive != NULL) {
        manager->load_archive(manager, parent_id);
    }
}

bool ArchiveManager_get_file_by_hash(const ArchiveManager *manager, const uint32 path, MemoryBuffer *mb) {
    TracyCZoneN(ctx, "ArchiveManager_get_file_by_hash", 1);

    ensure_parents_loaded(manager, path);
    for (uint32 i = 0; i < manager->archives.count; ++i) {
        Archive *ar = manager->archives.items[i];
        if (Archive_get_file_by_hash(ar, path, mb)) {
            // String *filename = find_name32(path);
            // if (filename != NULL) {
            //     // GLog_Info("File \"%s\" found in archive \"%s\"", String_cstr(filename),
            //     //        String_cstr(Archive_get_name(ar)));
            //     String_free(filename);
            // }
            // else
            //     GLog_Info("File with hash %08X found in archive \"%s\"", path,
            //            String_cstr(Archive_get_name(ar)));
            TracyCZoneEnd(ctx)
            return true;
        }
    }
    GLog_Error("File with hash %08X not found in any archive", path);
    TracyCZoneEnd(ctx)
    return false;
}

bool ArchiveManager_has_file_by_hash(const ArchiveManager *manager, const uint32 hash) {
    TracyCZoneN(ctx, "ArchiveManager_has_file_by_hash", 1);
    ensure_parents_loaded(manager, hash);
    for (uint32 i = 0; i < manager->archives.count; ++i) {
        const Archive *ar = manager->archives.items[i];
        if (Archive_has_file_by_hash(ar, hash)) {
            TracyCZoneEnd(ctx)
            return true;
        }
    }
    // Try to load parents and try again
    ensure_parents_loaded(manager, hash);
    for (uint32 i = 0; i < manager->archives.count; ++i) {
        const Archive *ar = manager->archives.items[i];
        if (Archive_has_file_by_hash(ar, hash)) {
            TracyCZoneEnd(ctx)
            return true;
        }
    }
    TracyCZoneEnd(ctx)
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
                       mp_free(*ar);
                       });
}
