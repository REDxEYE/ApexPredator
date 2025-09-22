// Created by RED on 19.09.2025.

#ifndef APEXPREDATOR_ADF_H
#define APEXPREDATOR_ADF_H
#include <stdbool.h>

#include "apex/adf/sti.h"

#include "utils/buffer/buffer.h"
#include "utils/dynamic_array.h"


DYNAMIC_ARRAY_STRUCT(String, String);

#pragma pack(push, 1)
typedef struct {
    char ident[4];
    uint32 version;
    uint32 instance_count;
    uint32 instance_offset;

    uint32 typedef_count;
    uint32 typedef_offset;

    uint32 stringhash_count;
    uint32 stringhash_offset;

    uint32 nametable_count;
    uint32 nametable_offset;

    uint32 total_size;

    uint32 unk[5];
} ADFHeader;

typedef struct {
    uint32 name_hash;
    uint32 type_hash;
    uint32 offset;
    uint32 size;
    uint64 name_id;
}ADFInstance;

#pragma pack(pop)

DYNAMIC_ARRAY_STRUCT(ADFInstance, ADFInstance);

typedef struct {
    ADFHeader header;
    String comment;
    DynamicArray_String strings;
    DynamicArray_ADFInstance instances;
    DynamicArray_STI_TypeDef type_defs;
    STI_TypeLibrary type_library;
} ADF;

bool ADF_from_buffer(ADF *adf, Buffer *buffer);

void ADF_generate_readers(ADF* adf, String* namespace, FILE* output);

void ADF_free(ADF *adf);

#endif //APEXPREDATOR_ADF_H
