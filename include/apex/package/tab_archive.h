// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_TAB_ARCHIVE_H
#define APEXPREDATOR_TAB_ARCHIVE_H

#include <stdbool.h>
#include "apex/package/tab.h"
#include "platform/archive.h"
#include "platform/archive_manager.h"
#include "utils/string.h"
#include "utils/dynamic_array.h"
#include "utils/dynamic_insert_only_map.h"
#include "utils/path.h"
#include "utils/buffer/memory_buffer.h"

DYNAMIC_ARRAY_STRUCT(TabEntry, TabEntry);
DYNAMIC_INSERT_ONLY_INT_MAP_STRUCT(TabEntry, TabEntryMap);

static const String tab_ext = {.buffer = ".tab", .size = 4, .capacity = 4, .statically_allocated = 1};

typedef struct {
    Archive;
    String tab_path;
    String arc_path;
    DynamicInsertOnlyIntMap_TabEntryMap entries;
} TabArchive;
TabArchive* TabArchive_new(String* path);

static inline void TabArchives_init(ArchiveManager *manager, const String *game_root) {
    DynamicArray_Path archive_paths = {};

    Path_rglob(game_root, &tab_ext, &archive_paths);
    for (uint32 i = 0; i < archive_paths.count; ++i) {

        TabArchive* archive = TabArchive_new(DA_at(&archive_paths, i));
        ArchiveManager_add(manager, (Archive*)archive);
    }
    DA_free_with_inner(&archive_paths, {
                       String_free(it);
                       });
}


#endif //APEXPREDATOR_TAB_ARCHIVE_H
