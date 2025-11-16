// Created by RED on 10.10.2025.

#ifndef APEXPREDATOR_TAG_FILE_H
#define APEXPREDATOR_TAG_FILE_H
#include <assert.h>
#include "int_def.h"
#include "platform/common_arrays.h"
#include "utils/dynamic_array.h"
#include "utils/buffer/buffer.h"
#include "havok_tag_types.h"

typedef struct {
    uint32 size: 30;
    uint32 flags: 2;
    char ident[4];
} TagHeader;

bool TagHeader_from_buffer(TagHeader *header, Buffer *buffer);


static_assert(sizeof(TagHeader) == 8, "Tag struct size mismatch");

typedef struct Tag {
    TagHeader header;
} Tag;

typedef struct TAG0Tag TAG0Tag;

bool SkipChunk_from_buffer(Buffer *buffer);

typedef enum {
    HK_SDK2015,
    HK_SDK2016,
    HK_SDK2017,
} HK_SDKVersion;


typedef struct {
    uint32 type: 24;
    uint32 flags: 8;
    uint32 offset;
    uint32 count;
} HKItem;

DYNAMIC_ARRAY_STRUCT(HKItem, HKItem);

typedef struct {
    char ver[8];
    DynamicArray_uint8 data;
    DynamicArray_HKTagType types;
    DynamicArray_HKItem items;
} TagFile;

bool TagFile_from_buffer(TagFile *tf, Buffer *buffer);

HK_SDKVersion TagFile_get_sdk_version(TagFile *tf);

void TagFile_free(TagFile *tf);

#endif //APEXPREDATOR_TAG_FILE_H
