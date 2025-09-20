// Created by RED on 17.09.2025.

#ifndef APEXPREDATOR_BUFFER_H
#define APEXPREDATOR_BUFFER_H

#include "int_def.h"
#include "utils/string.h"
struct Buffer_s;
struct BufferInterface_s;

typedef enum {
    BUFFER_ORIGIN_START = 0,
    BUFFER_ORIGIN_CURRENT = 1,
    BUFFER_ORIGIN_END = 2
} BufferPositionOrigin;

typedef enum { // Below zero - error, above zero - non-critical, zero = success
    BUFFER_FAILED = -1,
    BUFFER_SUCCESS = 0,
    BUFFER_UNDERFLOW,
} BufferError;

// Define standard read/write function types:
typedef BufferError (*BufferSetPositionFn)(void *buffer, int64 position, BufferPositionOrigin origin);

typedef BufferError (*BufferGetPositionFn)(void *buffer, uint64 *position);

typedef BufferError (*BufferReadFn)(void *buffer, void *dest, uint32 size, uint32 *read);

typedef BufferError (*BufferWriteFn)(void *buffer, const void *src, uint64 size, uint32 *written);

typedef BufferError (*BufferGetSizeFn)(void *buffer, uint64 *size);

typedef BufferError (*BufferCloseFn)(void *buffer);

typedef uint8 (*ReadUInt8Fn)(void *buffer, BufferError* error);

typedef uint16 (*ReadUInt16Fn)(void *buffer, BufferError* error);

typedef uint32 (*ReadUInt32Fn)(void *buffer, BufferError* error);

typedef uint64 (*ReadUInt64Fn)(void *buffer, BufferError* error);

typedef int8 (*ReadInt8Fn)(void *buffer, BufferError* error);

typedef int16 (*ReadInt16Fn)(void *buffer, BufferError* error);

typedef int32 (*ReadInt32Fn)(void *buffer, BufferError* error);

typedef int64 (*ReadInt64Fn)(void *buffer, BufferError* error);

typedef float (*ReadFloatFn)(void *buffer, BufferError* error);

typedef double (*ReadDoubleFn)(void *buffer, BufferError* error);

typedef BufferError (*ReadCStringFn)(void *buffer, String* string);
typedef BufferError (*ReadStringFn)(void *buffer, uint32 size, String* string);


typedef struct BufferInterface_s {
    BufferSetPositionFn set_position;
    BufferGetPositionFn get_position;
    BufferReadFn read;
    BufferWriteFn write;
    BufferGetSizeFn getsize;
    BufferCloseFn close;
    ReadUInt8Fn read_uint8;
    ReadUInt16Fn read_uint16;
    ReadUInt32Fn read_uint32;
    ReadUInt64Fn read_uint64;
    ReadInt8Fn read_int8;
    ReadInt16Fn read_int16;
    ReadInt32Fn read_int32;
    ReadInt64Fn read_int64;
    ReadFloatFn read_float;
    ReadDoubleFn read_double;
    ReadCStringFn read_cstring;
    ReadStringFn read_string;
} BufferInterface;

typedef struct Buffer_s {
    struct BufferInterface_s;
} Buffer;

void Buffer_init(Buffer* buffer);

uint64 Buffer_remaining(Buffer* buffer, BufferError *error);

#endif //APEXPREDATOR_BUFFER_H
