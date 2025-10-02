// Created by RED on 02.10.2025.

#ifndef APEXPREDATOR_SARC_H
#define APEXPREDATOR_SARC_H
#include "int_def.h"
#include "adf/adf.h"
#include "utils/buffer/buffer.h"

typedef struct {
    uint32 version;
    char ident[4];
    uint32 version2;
    uint32 dir_block_len;
}SArcHeader;

typedef struct {
    String name;
    uint32 offset;
    uint32 size;
    uint32 hash;
    uint32 ext_hash;
}SArcEntry;

DYNAMIC_ARRAY_STRUCT(SArcEntry, SArcEntry);

typedef struct {
    SArcHeader header;
    char* strings;
    DynamicArray_SArcEntry entries;
}SArchive;

void SArchive_from_buffer(SArchive* archive, Buffer* buffer);
void SArchive_free(SArchive* archive);

#endif //APEXPREDATOR_SARC_H