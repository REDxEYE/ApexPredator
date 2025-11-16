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

typedef enum {
    // Below zero - error, above zero - non-critical, zero = success
    BUFFER_FAILED = -1,
    BUFFER_SUCCESS = 0,
    BUFFER_UNDERFLOW,
} BufferError;


// Define standard read/write function types:
typedef BufferError (*BufferSetPositionFn)(void *buffer, int64 position, BufferPositionOrigin origin);

typedef BufferError (*BufferGetPositionFn)(void *buffer, int64 *position);

typedef BufferError (*BufferReadFn)(void *buffer, void *dest, uint32 size, uint32 *read);

typedef BufferError (*BufferWriteFn)(void *buffer, const void *src, uint64 size, uint32 *written);

typedef BufferError (*BufferGetSizeFn)(void *buffer, uint64 *size);

typedef BufferError (*BufferCloseFn)(void *buffer);

typedef BufferError (*BufferSkipFn)(void *buffer, uint32 size);

typedef BufferError (*ReadUInt8Fn)(void *buffer, uint8 *data);

typedef BufferError (*ReadUInt16Fn)(void *buffer, uint16 *data);

typedef BufferError (*ReadUInt32Fn)(void *buffer, uint32 *data);

typedef BufferError (*ReadUInt64Fn)(void *buffer, uint64 *data);

typedef BufferError (*ReadInt8Fn)(void *buffer, int8 *data);

typedef BufferError (*ReadInt16Fn)(void *buffer, int16 *data);

typedef BufferError (*ReadInt32Fn)(void *buffer, int32 *data);

typedef BufferError (*ReadInt64Fn)(void *buffer, int64 *data);

typedef BufferError (*ReadFloatFn)(void *buffer, float32 *data);

typedef BufferError (*ReadDoubleFn)(void *buffer, float64 *data);

typedef BufferError (*ReadCStringFn)(void *buffer, String *string);

typedef BufferError (*ReadStringFn)(void *buffer, uint32 size, String *string);

typedef BufferError (*WriteUInt8Fn)(void *buffer, uint8 data);

typedef BufferError (*WriteUInt16Fn)(void *buffer, uint16 data);

typedef BufferError (*WriteUInt32Fn)(void *buffer, uint32 data);

typedef BufferError (*WriteUInt64Fn)(void *buffer, uint64 data);

typedef BufferError (*WriteInt8Fn)(void *buffer, int8 data);

typedef BufferError (*WriteInt16Fn)(void *buffer, int16 data);

typedef BufferError (*WriteInt32Fn)(void *buffer, int32 data);

typedef BufferError (*WriteInt64Fn)(void *buffer, int64 data);

typedef BufferError (*WriteFloatFn)(void *buffer, float32 data);

typedef BufferError (*WriteDoubleFn)(void *buffer, float64 data);

typedef BufferError (*WriteCStringFn)(void *buffer, String *string);

typedef BufferError (*WriteStringFn)(void *buffer, uint32 size, String *string);


typedef struct BufferInterface_s {
    BufferSetPositionFn set_position;
    BufferGetPositionFn get_position;
    BufferReadFn read;
    BufferWriteFn write;
    BufferGetSizeFn getsize;
    BufferCloseFn close;
    BufferSkipFn skip;
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
    WriteUInt8Fn write_uint8;
    WriteUInt16Fn write_uint16;
    WriteUInt32Fn write_uint32;
    WriteUInt64Fn write_uint64;
    WriteInt8Fn write_int8;
    WriteInt16Fn write_int16;
    WriteInt32Fn write_int32;
    WriteInt64Fn write_int64;
    WriteFloatFn write_float;
    WriteDoubleFn write_double;
    WriteCStringFn write_cstring;
    WriteStringFn write_string;
} BufferInterface;

typedef struct Buffer_s {
    struct BufferInterface_s;
} Buffer;

void Buffer_init(Buffer *buffer);

uint64 Buffer_remaining(Buffer *buffer, BufferError *error);

void Buffer_align(Buffer *buffer, uint32 alignment);

#define IS_SUCCESS(expr)  (expr)==BUFFER_SUCCESS
#define IS_FAILED(expr)   (expr)<=BUFFER_FAILED

#endif //APEXPREDATOR_BUFFER_H
