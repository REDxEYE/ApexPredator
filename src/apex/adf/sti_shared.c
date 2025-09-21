// Created by RED on 21.09.2025.

#include "apex/adf/sti_shared.h"

bool read_STI_int8(Buffer *buffer, STI_int8 *out) {
    return buffer->read_int8(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_uint8(Buffer *buffer, STI_uint8 *out) {
    return buffer->read_uint8(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_int16(Buffer *buffer, STI_int16 *out) {
    return buffer->read_int16(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_uint16(Buffer *buffer, STI_uint16 *out) {
    return buffer->read_uint16(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_int32(Buffer *buffer, STI_int32 *out) {
    return buffer->read_int32(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_uint32(Buffer *buffer, STI_uint32 *out) {
    return buffer->read_uint32(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_int64(Buffer *buffer, STI_int64 *out) {
    return buffer->read_int64(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_uint64(Buffer *buffer, STI_uint64 *out) {
    return buffer->read_uint64(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_float32(Buffer *buffer, STI_float32 *out) {
    return buffer->read_float(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_float64(Buffer *buffer, STI_float64 *out) {
    return buffer->read_double(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_String(Buffer *buffer, STI_String *out) {
    return buffer->read_cstring(buffer, out) == BUFFER_SUCCESS;
}
