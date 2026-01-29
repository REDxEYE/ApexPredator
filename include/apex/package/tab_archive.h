// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_TAB_ARCHIVE_H
#define APEXPREDATOR_TAB_ARCHIVE_H

#include <stdbool.h>
#include "apex/package/tab.h"
#include "platform/archive.h"
#include "platform/archive_manager.h"
#include "utils/string.h"
#include "utils/dynamic_array.h"
#include "utils/dynamic_map.h"
#include "utils/path.h"

DYNAMIC_ARRAY_STRUCT(TabEntry, TabEntry);
DYNAMIC_INT_MAP_STRUCT(TabEntry, TabEntryMap);

typedef struct {
    Archive;
    String tab_path;
    String arc_path;
    DynamicIntMap_TabEntryMap entries;
} TabArchive;
TabArchive* TabArchive_new(const String* path);

static inline void TabArchives_init(ArchiveManager *manager, const String *game_root) {
    TracyCZoneN(ctx, "TabArchives_init", 1);
    DynamicArray_Path archive_paths = {0};

    static String tab_ext = {0};
    if (String_size(&tab_ext)==0) {
        String_from_cstr(&tab_ext, ".tab");
    }

    Path_rglob(game_root, &tab_ext, &archive_paths);
    for (uint32 i = 0; i < archive_paths.count; ++i) {

        TabArchive* archive = TabArchive_new(DA_at(&archive_paths, i));
        ArchiveManager_add(manager, (Archive*)archive);
    }
    DA_free_with_inner(&archive_paths, {
                       String_free(it);
                       });
    TracyCZoneEnd(ctx);
}


#endif //APEXPREDATOR_TAB_ARCHIVE_H
