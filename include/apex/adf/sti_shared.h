// Created by RED on 20.09.2025.

#ifndef APEXPREDATOR_STI_SHARED_H
#define APEXPREDATOR_STI_SHARED_H

#include <stdio.h>

#include "int_def.h"
#include "utils/string.h"
#include "utils/dynamic_array.h"
#include "utils/buffer/buffer.h"

typedef int8 STI_int8;
typedef uint8 STI_uint8;
typedef int16 STI_int16;
typedef uint16 STI_uint16;
typedef int32 STI_int32;
typedef uint32 STI_uint32;
typedef int64 STI_int64;
typedef uint64 STI_uint64;
typedef float32 STI_float32;
typedef float64 STI_float64;
typedef String STI_String;

bool read_STI_int8(Buffer *buffer, STI_int8 *out);

bool read_STI_uint8(Buffer *buffer, STI_uint8 *out);

bool read_STI_int16(Buffer *buffer, STI_int16 *out);

bool read_STI_uint16(Buffer *buffer, STI_uint16 *out);

bool read_STI_int32(Buffer *buffer, STI_int32 *out);

bool read_STI_uint32(Buffer *buffer, STI_uint32 *out);

bool read_STI_int64(Buffer *buffer, STI_int64 *out);

bool read_STI_uint64(Buffer *buffer, STI_uint64 *out);

bool read_STI_float32(Buffer *buffer, STI_float32 *out);

bool read_STI_float64(Buffer *buffer, STI_float64 *out);

bool read_STI_String(Buffer *buffer, STI_String *out);

void free_STI_String(String* obj);

void print_STI_int8(STI_int8 *obj, FILE* handle, uint32 indent);

void print_STI_uint8(STI_uint8 *obj, FILE* handle, uint32 indent);

void print_STI_int16(STI_int16 *obj, FILE* handle, uint32 indent);

void print_STI_uint16(STI_uint16 *obj, FILE* handle, uint32 indent);

void print_STI_int32(STI_int32 *obj, FILE* handle, uint32 indent);

void print_STI_uint32(STI_uint32 *obj, FILE* handle, uint32 indent);

void print_STI_int64(STI_int64 *obj, FILE* handle, uint32 indent);

void print_STI_uint64(STI_uint64 *obj, FILE* handle, uint32 indent);

void print_STI_float32(STI_float32 *obj, FILE* handle, uint32 indent);

void print_STI_float64(STI_float64 *obj, FILE* handle, uint32 indent);

void print_STI_String(STI_String *obj, FILE* handle, uint32 indent);

typedef enum {
    None = 0,
    zlib = 1,
    lz4f = 2,
    zstd = 3
}CompType;

typedef struct {
    char ident[4];
    uint8 a;
    uint8 comp_type;
    uint8 c;
    uint8 d;
    uint64 decomp_size;
}CompressedHeader;

typedef struct {
    uint32 unk;
    uint32 offset;
    uint32 unk2;
    uint32 size;
    uint32 unk3;
    uint32 unk4;
}CompressedDataInfo;

typedef bool (*readSTIObject)(Buffer* buffer, void* obj);
typedef void (*freeSTIObject)(void* obj);
typedef void (*printSTIObject)(void* obj, FILE* handle, uint32 indent);

typedef struct {
    readSTIObject read;
    freeSTIObject free;
    printSTIObject print;
}STI_ObjectMethods;

// void decompress_data() {
//     CompressedHeader* header =(CompressedHeader*)&compressed_data.items[0];
//     assert((*(uint32*)header->ident == 'PMOC') && "Invalid compressed data magic");
//     if (header->comp_type==zstd) {
//         size_t zstd_res = ZSTD_decompress(out->DecompressedData.items, out->DecompressedData.count, &compressed_data.items[sizeof(CompressedHeader)], out->Data.size-sizeof(CompressedHeader));
//         ZSTD_ErrorCode error = ZSTD_isError(zstd_res);
//         if (error!=ZSTD_error_no_error) {
//             printf("ZSTD decompression error: %s\n", ZSTD_getErrorName(error));
//             assert(false && "ZSTD decompression failed");
//         }
//         assert(zstd_res==header->decomp_size &&  "ZSTD decompression size mismatch");
//     }else {
//         printf("Unsupported compression type: %d\n", header->comp_type);
//         assert(false && "Unsupported compression type");
//     }
// }

#endif //APEXPREDATOR_STI_SHARED_H
