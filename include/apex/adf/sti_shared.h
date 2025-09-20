// Created by RED on 20.09.2025.

#ifndef APEXPREDATOR_STI_SHARED_H
#define APEXPREDATOR_STI_SHARED_H

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

#endif //APEXPREDATOR_STI_SHARED_H
