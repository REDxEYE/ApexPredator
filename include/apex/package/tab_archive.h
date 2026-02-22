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
#include "utils/buffer/file_buffer.h"

DYNAMIC_ARRAY_STRUCT(TabEntry, TabEntry);

DYNAMIC_INT_MAP_STRUCT(TabEntry, TabEntryMap);

struct TabArchive : Archive {
    String tab_path;
    FileBuffer arc_buffer;
    DynamicIntMap_TabEntryMap entries;
};

TabArchive *TabArchive_new(const String *path);

void TabArchives_init(const ArchiveManager *manager, const String *game_root);


#endif //APEXPREDATOR_TAB_ARCHIVE_H
