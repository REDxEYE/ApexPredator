// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_TAB_ARCHIVE_H
#define APEXPREDATOR_TAB_ARCHIVE_H

#include <stdbool.h>
#include "apex/package/tab.h"
#include "utils/string.h"
#include "utils/dynamic_array.h"
#include "utils/dynamic_insert_only_map.h"
#include "utils/buffer/memory_buffer.h"

DYNAMIC_ARRAY_STRUCT(TabEntry, TabEntry);
DYNAMIC_INSERT_ONLY_INT_MAP_STRUCT(TabEntry, TabEntryMap);

typedef struct {
    String tab_path;
    String arc_path;
    DynamicInsertOnlyIntMap_TabEntryMap entries;
} TabArchive;

void TabArchive_open(TabArchive* ar, String* path);
bool TabArchive_get_data(TabArchive* ar, uint32 key, MemoryBuffer* mb);
bool TabArchive_has_hash(const TabArchive* ar, uint32 hash);
void TabArchive_free(TabArchive* ar);

#endif //APEXPREDATOR_TAB_ARCHIVE_H
