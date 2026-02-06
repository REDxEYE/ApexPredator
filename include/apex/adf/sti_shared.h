// Created by RED on 20.09.2025.

#ifndef APEXPREDATOR_STI_SHARED_H
#define APEXPREDATOR_STI_SHARED_H

#include "adf_type_info_map.h"
#include "int_def.h"
#include "utils/string.h"
#include "utils/dynamic_array.h"
#include "utils/json.h"
#include "utils/buffer/buffer.h"

typedef String STI_String;
typedef uint32 StringHash_48c5294d_4;
typedef uint64 StringHash_99cfa095_6;
typedef uint64 StringHash_48c5294d_8;

#pragma pack(push, 1)
typedef struct {
    const STITypeInfo* type_info_;
    uint32 offset;
    uint32 size;
    uint32 type_hash;
    uint32 pad;
    void *data;
} Deferred;
#pragma pack(pop)

typedef enum {
    None = 0,
    zlib = 1,
    lz4f = 2,
    zstd = 3
} CompType;

typedef struct {
    char ident[4];
    uint8 a;
    uint8 comp_type;
    uint8 c;
    uint8 d;
    uint64 decomp_size;
} CompressedHeader;


#endif //APEXPREDATOR_STI_SHARED_H
