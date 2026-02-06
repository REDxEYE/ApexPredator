// Created by RED on 02.02.2026.

#ifndef APEXPREDATOR_ADF_TYPE_INFO_MAP_H
#define APEXPREDATOR_ADF_TYPE_INFO_MAP_H
#include "utils/dynamic_map.h"
#include "utils/json.h"
#include "utils/buffer/buffer.h"


typedef void (*initSTIObject)(void *obj);

typedef bool (*readSTIObject)(void *obj, Buffer *buffer);

typedef void (*freeSTIObject)(void *obj);

typedef void (*printSTIObject)(const void *obj, JsonContext *ctx);

typedef struct STITypeInfo {
    initSTIObject init;
    readSTIObject read;
    freeSTIObject free;
    printSTIObject print;
    uint32 size;
    uint32 disk_size:30;
    uint32 is_struct:1;
    uint32 is_array:1;
    uint32 hash;
    const char* name;
}STITypeInfo;

DYNAMIC_ARRAY_STRUCT(STITypeInfo, STITypeInfo);
DYNAMIC_INT_MAP_STRUCT(STITypeInfo, STITypeInfo);

typedef DynamicIntMap_STITypeInfo STITypeInfoMap;


#endif //APEXPREDATOR_ADF_TYPE_INFO_MAP_H