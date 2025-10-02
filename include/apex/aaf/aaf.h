// Created by RED on 01.10.2025.

#ifndef APEXPREDATOR_AAF_H
#define APEXPREDATOR_AAF_H

#include "int_def.h"
#include "utils/dynamic_array.h"
#include "utils/buffer/memory_buffer.h"

typedef struct {
    char ident[4];
    uint32 version;
    char awesome[28];
    uint32 uncompressed_size;
    uint32 section_size;
    uint32 section_count;
}AAFHeader;

typedef struct {
    uint32 compressed_size;
    uint32 uncompressed_size;
    uint32 total_size;
    char magic[4];
}AAFSectionHeader;

typedef struct {
    AAFSectionHeader header;
    MemoryBuffer buffer;
}AAFSection;

DYNAMIC_ARRAY_STRUCT(AAFSection, AAFSection);

typedef struct {
    AAFHeader header;
    MemoryBuffer buffer;
    DynamicArray_AAFSection sections;
}AAFArchive;

void AAFArchive_from_buffer(AAFArchive *archive, Buffer* buffer);

bool AAFArchive_get_section(AAFArchive *archive, uint32 index, MemoryBuffer *out);

void AAFArchive_free(AAFArchive *archive);

#endif //APEXPREDATOR_AAF_H