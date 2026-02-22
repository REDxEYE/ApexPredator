// Created by RED on 02.02.2026.

#ifndef APEXPREDATOR_HAVOK_TYPE_INFO_MAP_H
#define APEXPREDATOR_HAVOK_TYPE_INFO_MAP_H
#include "tag_file/havok_tag_file.h"
#include "utils/dynamic_map.h"
#include "utils/json.h"


typedef void (*initHavokObject)(void *obj);

typedef void (*readHavokObject)(void *obj, const TagFile *tf, const uint8 *src);

typedef void (*freeHavokObject)(void *obj);

typedef void (*printHavokObject)(void *obj, JsonContext *ctx);

typedef struct HavokVTable {
    initHavokObject init;
    freeHavokObject free;
    readHavokObject read;
    printHavokObject print;
    uint32 size;
    uint32 disk_size:30;
    uint32 is_record:1;
    uint32 is_array:1;
    uint32 hash;
    const char* name;
}HavokTypeInfo;

DYNAMIC_ARRAY_STRUCT(HavokTypeInfo, HavokTypeInfo);
DYNAMIC_INT_MAP_STRUCT(HavokTypeInfo, HavokTypeInfo);

typedef DynamicIntMap_HavokTypeInfo TypeInfoMap;

#endif //APEXPREDATOR_HAVOK_TYPE_INFO_MAP_H