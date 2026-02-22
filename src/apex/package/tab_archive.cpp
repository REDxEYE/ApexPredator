// Created by RED on 18.09.2025.

#include "apex/package/tab_archive.h"
#include "utils/buffer/file_buffer.h"

#include <assert.h>

#include "platform/logger.h"
#include "utils/memory_profiling.h"
#include "tracy/TracyC.h"
#include "utils/hash_helper.h"

bool TabArchive__has_file(const TabArchive *ar, const String *path);

bool TabArchive__has_file_by_hash(const TabArchive *ar, uint32 hash);

bool TabArchive__get_file(TabArchive *ar, const String *path, MemoryBuffer *mb);

bool TabArchive__get_file_by_hash(TabArchive *ar, uint32 hash, MemoryBuffer *mb);

const String *TabArchive__get_name(const TabArchive *ar);

void TabArchive__free(TabArchive *ar);

void TabArchive__open(TabArchive *ar, const String *path);

const String *TabArchive__get_name(const TabArchive *ar) {
    return &ar->tab_path;
}

void TabArchive_get_all_entries(TabArchive *ar, DynamicArray_ArchiveEntry *entries) {
    for (uint32 i = 0; i < ar->entries.values.count; ++i) {
        const TabEntry *tab_entry = static_cast<const TabEntry *>(DM_get_value(&ar->entries, i));
        ArchiveEntry *entry = (ArchiveEntry *)DA_append_get(entries);
        entry->path_hash = tab_entry->hash;
        entry->archive = (Archive *) ar;
        entry->size = tab_entry->size;
    }
}

static inline const char *
path_find_second_last_sep(const char *path) {
    if (!path || !*path) return NULL;

    const char *last = NULL;
    const char *second = NULL;

    for (const char *p = path; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            second = last;
            last = p;
        }
    }

    return second;
}

uint32 TabArchive__get_hash(const TabArchive *ar) {
    const String *archive_name = &ar->tab_path;
    const char *archive_base_name = path_find_second_last_sep(String_cstr(archive_name));
    return hash_cstring(archive_base_name);
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

TabArchive *TabArchive_new(const String *path) {
    TabArchive *ar = static_cast<TabArchive *>(mp_malloc(sizeof(TabArchive)));
    if (ar == NULL) {
        GLog_Error("Failed to allocate memory for TabArchive");
        abort();
    }
    memset(ar, 0, sizeof(TabArchive));
    TabArchive__init_interface(ar);
    TabArchive__open(ar, path);
    return ar;
}

void TabArchives_init(const ArchiveManager *manager, const String *game_root) {
    TracyCZoneN(ctx, "TabArchives_init", 1);
    DynamicArray_Path archive_paths = {};

    static String tab_ext = {};
    if (String_size(&tab_ext)==0) {
        String_from_cstr(&tab_ext, ".tab");
    }

    Path_rglob(game_root, &tab_ext, &archive_paths);
    for (uint32 i = 0; i < archive_paths.count; ++i) {
        TabArchive* archive = TabArchive_new(static_cast<const String *>(DA_at(&archive_paths, i)));
        ArchiveManager_add(manager, (Archive*)archive);
    }
    DA_free_with_inner(&archive_paths, { String_free(static_cast<String *>(it)); });
    TracyCZoneEnd(ctx);
}

void TabArchive__open(TabArchive *ar, const String *path) {
    TracyCZoneN(ctx, "TabArchive__open", 1);
    GLog_Info("Opening tab archive: %s", String_cstr(path));
    String_copy_from(&ar->tab_path, path);

    FileBuffer tab_buffer = {};
    if (FileBuffer_open_read(&tab_buffer, String_cstr(path)) != BUFFER_SUCCESS) {
        GLog_Error("Failed to open tab file %s", String_cstr(path));
        TracyCZoneEnd(ctx);
        return;
    }

    String arc_path = {};
    Path_replace_extension(path, "arc", &arc_path);
    FileBuffer_open_read(&ar->arc_buffer, String_cstr(&arc_path));
    String_free(&arc_path);
    
    TabHeader header;
    tab_buffer.read(&tab_buffer, &header, sizeof(header),NULL);
    TabEntry entry;

    BufferError error;
    const uint32 entry_count = Buffer_remaining((Buffer *) &tab_buffer, &error) / sizeof(TabEntry);
    DM_init(&ar->entries, TabEntry, entry_count);
    for (int i = 0; i < entry_count; i++) {
        error = tab_buffer.read(&tab_buffer, &entry, sizeof(TabEntry), NULL);
        if (error < BUFFER_FAILED) {
            GLog_Error("Failed to read entry %d", i);
            TracyCZoneEnd(ctx);
            return;
        }
        *(TabEntry *) DM_insert(&ar->entries, entry.hash) = entry;
    }
    tab_buffer.close(&tab_buffer);
    TracyCZoneEnd(ctx);
}

const TabEntry *Archive__find_entry(const TabArchive *ar, const uint32 hash) {
    return static_cast<const TabEntry *>(DM_get(&ar->entries, hash));
}

bool TabArchive__get_file(TabArchive *ar, const String *path, MemoryBuffer *mb) {
    const uint32 hash = hash_string(path);
    return TabArchive__get_file_by_hash(ar, hash, mb);
}

bool TabArchive__get_file_by_hash(TabArchive *ar, const uint32 hash, MemoryBuffer *mb) {
    const TabEntry *entry = Archive__find_entry(ar, hash);
    if (entry == NULL) {
        return false;
    }

    ar->arc_buffer.set_position(&ar->arc_buffer, entry->offset, BUFFER_ORIGIN_START);
    if (MemoryBuffer_allocate(mb, entry->size) != BUFFER_SUCCESS) {
        return false;
    }
    uint32 read_bytes = 0;
    if (ar->arc_buffer.read(&ar->arc_buffer, mb->data, entry->size, &read_bytes) != BUFFER_SUCCESS) {
        return false;
    }
    if (read_bytes != entry->size) {
        return false;
    }
    return true;
}

bool TabArchive__has_file(const TabArchive *ar, const String *path) {
    const uint32 hash = hash_string(path);
    return DM_get(&ar->entries, hash) != NULL;
}

bool TabArchive__has_file_by_hash(const TabArchive *ar, const uint32 hash) {
    return DM_get(&ar->entries, hash) != NULL;
}

void TabArchive__free(TabArchive *ar) {
    String_free(&ar->tab_path);
    Buffer_close((Buffer *) &ar->arc_buffer);
    DM_free(&ar->entries);
}
