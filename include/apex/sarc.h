// Created by RED on 02.10.2025.

#ifndef APEXPREDATOR_SARC_H
#define APEXPREDATOR_SARC_H
#include "int_def.h"
#include "adf/adf.h"
#include "platform/archive.h"
#include "utils/buffer/buffer.h"
#include "utils/dynamic_insert_only_map.h"

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
DYNAMIC_INSERT_ONLY_INT_MAP_STRUCT(SArcEntry, SArcEntryMap);

typedef struct {
    struct ArchiveInterface;
    SArcHeader header;
    char* strings;
    DynamicInsertOnlyIntMap_SArcEntryMap entries;
    Buffer* buffer;
}SArchive;

SArchive* SArchive_new(Buffer* buffer);
// void SArchive_from_buffer(SArchive* archive, Buffer* buffer);
// void SArchive_free(SArchive* archive);

#endif //APEXPREDATOR_SARC_H