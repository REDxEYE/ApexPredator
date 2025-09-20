// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_ARCHIVE_H
#define APEXPREDATOR_ARCHIVE_H

#include <stdbool.h>

#include "apex/package/tab.h"
#include "utils/string.h"
#include "utils/dynamic_array.h"
#include "utils/buffer/memory_buffer.h"

DYNAMIC_ARRAY_STRUCT(TabEntry, TabEntry);

typedef struct {
    String tab_path;
    String arc_path;
    DynamicArray_TabEntry entries;

} Archive;

void Archive_open(Archive* ar, String* path);
bool Archive_get_data(Archive* ar, uint32 key, MemoryBuffer* mb);


#endif //APEXPREDATOR_ARCHIVE_H
