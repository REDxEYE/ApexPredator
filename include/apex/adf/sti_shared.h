// Created by RED on 20.09.2025.

#ifndef APEXPREDATOR_STI_SHARED_H
#define APEXPREDATOR_STI_SHARED_H

#include <stdio.h>

#include "int_def.h"
#include "utils/string.h"
#include "utils/dynamic_array.h"
#include "utils/buffer/buffer.h"
#include "utils/buffer/memory_buffer.h"

typedef struct STI_TypeLibrary STI_TypeLibrary;

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
typedef uint32 StringHash_48c5294d_4;
typedef uint64 StringHash_99cfa095_6;
typedef uint64 StringHash_48c5294d_8;

typedef struct {
    uint32 offset;
    uint32 size;
    uint32 type_hash;
    uint32 pad;
    void *data;
} STI_Deferred;



bool read_STI_int8(Buffer *buffer, STI_TypeLibrary *lib, STI_int8 *out);

bool read_STI_uint8(Buffer *buffer, STI_TypeLibrary *lib, STI_uint8 *out);

bool read_STI_int16(Buffer *buffer, STI_TypeLibrary *lib, STI_int16 *out);

bool read_STI_uint16(Buffer *buffer, STI_TypeLibrary *lib, STI_uint16 *out);

bool read_STI_int32(Buffer *buffer, STI_TypeLibrary *lib, STI_int32 *out);

bool read_STI_uint32(Buffer *buffer, STI_TypeLibrary *lib, STI_uint32 *out);

bool read_StringHash_48c5294d_4(Buffer *buffer, STI_TypeLibrary *lib, StringHash_48c5294d_4 *out);

bool read_StringHash_99cfa095_6(Buffer *buffer, STI_TypeLibrary *lib, StringHash_99cfa095_6 *out);

bool read_StringHash_48c5294d_8(Buffer *buffer, STI_TypeLibrary *lib, StringHash_48c5294d_8 *out);

bool read_STI_int64(Buffer *buffer, STI_TypeLibrary *lib, STI_int64 *out);

bool read_STI_uint64(Buffer *buffer, STI_TypeLibrary *lib, STI_uint64 *out);

bool read_STI_float32(Buffer *buffer, STI_TypeLibrary *lib, STI_float32 *out);

bool read_STI_float64(Buffer *buffer, STI_TypeLibrary *lib, STI_float64 *out);

bool read_STI_String(Buffer *buffer, STI_TypeLibrary *lib, STI_String *out);

bool read_STI_Deferred(Buffer *buffer, STI_TypeLibrary *lib, STI_Deferred *out);

void free_STI_String(String *obj, STI_TypeLibrary *lib);

void free_STI_Deferred(STI_Deferred *obj, STI_TypeLibrary *lib);

void print_STI_int8(const STI_int8 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_STI_uint8(const STI_uint8 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_STI_int16(const STI_int16 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_STI_uint16(const STI_uint16 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_STI_int32(const STI_int32 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_STI_uint32(const STI_uint32 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_StringHash_48c5294d_4(const StringHash_48c5294d_4 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_StringHash_99cfa095_6(const StringHash_99cfa095_6 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_StringHash_48c5294d_8(const StringHash_48c5294d_8 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_STI_int64(const STI_int64 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_STI_uint64(const STI_uint64 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_STI_float32(const STI_float32 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_STI_float64(const STI_float64 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_STI_String(const STI_String *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

void print_STI_Deferred(const STI_Deferred *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent);

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


typedef bool (*readSTIObject)(Buffer *buffer, STI_TypeLibrary *lib, void *obj);

typedef void (*freeSTIObject)(void *obj, STI_TypeLibrary *lib);

typedef void (*printSTIObject)(const void *obj, const STI_TypeLibrary *lib, FILE *handle, uint32 indent);

typedef struct {
    readSTIObject read;
    freeSTIObject free;
    printSTIObject print;
    uint32 size;
} STI_ObjectMethods;


#endif //APEXPREDATOR_STI_SHARED_H
