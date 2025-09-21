// Created by RED on 17.09.2025.

#include <stdbool.h>
#include <stddef.h>
#include "utils/buffer/buffer.h"

#include <assert.h>

#define CALL_AND_CHECK_ERROR(func, buffer, err_ptr) \
({ \
uint8 _ret = func_call(buffer, err_ptr); \
if ((err_ptr) && *(err_ptr) < BUFFER_SUCCESS) { \
/* handle error if needed */ \
} \
_ret; \
})

static BufferError Buffer__read_uint8(Buffer *buffer, uint8 *value) {
    return buffer->read(buffer, value, sizeof(*value), NULL);
}

static BufferError Buffer__read_uint16(Buffer *buffer, uint16 *value) {
    return buffer->read(buffer, value, sizeof(*value), NULL);
}

static BufferError Buffer__read_uint32(Buffer *buffer, uint32 *value) {
    return buffer->read(buffer, value, sizeof(*value), NULL);
}

static BufferError Buffer__read_uint64(Buffer *buffer, uint64 *value) {
    return buffer->read(buffer, value, sizeof(*value), NULL);
}

static BufferError Buffer__read_int8(Buffer *buffer, int8 *value) {
    return buffer->read(buffer, value, sizeof(*value), NULL);
}

static BufferError Buffer__read_int16(Buffer *buffer, int16 *value) {
    return buffer->read(buffer, value, sizeof(*value), NULL);
}

static BufferError Buffer__read_int32(Buffer *buffer, int32 *value) {
    return buffer->read(buffer, value, sizeof(*value), NULL);
}

static BufferError Buffer__read_int64(Buffer *buffer, int64 *value) {
    return buffer->read(buffer, value, sizeof(*value), NULL);
}

static BufferError Buffer__read_float(Buffer *buffer, float32 *value) {
    return buffer->read(buffer, value, sizeof(*value), NULL);
}

static BufferError Buffer__read_double(Buffer *buffer, float64 *value) {
    return buffer->read(buffer, value, sizeof(*value), NULL);
}

BufferError Buffer__read_cstring(Buffer *buffer, String *string) {
    assert(buffer!=NULL && "buffer is null");
    String_init(string, STRING_EMBEDDED_SIZE);
    while (1) {
        char buff[32] = {0};
        uint32 readResult;
        BufferError error = buffer->read(buffer, buff, sizeof(buff), &readResult);
        if (error < BUFFER_SUCCESS) {
            return error;
        }
        bool hasZero = false;
        int zeroPos;
        for (zeroPos = 0; zeroPos < 32; ++zeroPos) {
            if (buff[zeroPos] == 0) {
                hasZero = true;
                break;
            }
        }
        if (hasZero && zeroPos != 0) {
            String_append_cstr(string, buff, zeroPos);
            buffer->set_position(buffer, (zeroPos + 1) - 32, BUFFER_ORIGIN_CURRENT);
            break; // Null terminator found
        }
        String_append_cstr(string, buff, 32);
    }
    return BUFFER_SUCCESS;
}

static BufferError Buffer__read_string(Buffer *fb, uint32 size, String *string) {
    String_init(string, size);
    uint32 readResult;
    BufferError readErr;
    if ((readErr = fb->read(fb, String_data(string), size, &readResult)) <= BUFFER_FAILED) {
        return readErr;
    }
    String_trim_zeros(string);
    return readErr;
}

void Buffer_init(Buffer *buffer) {
    assert(buffer != NULL);

    buffer->set_position = NULL;
    buffer->get_position = NULL;
    buffer->read = NULL;
    buffer->write = NULL;
    buffer->getsize = NULL;
    buffer->close = NULL;
    buffer->read_uint8 = (ReadUInt8Fn) Buffer__read_uint8;
    buffer->read_uint16 = (ReadUInt16Fn) Buffer__read_uint16;
    buffer->read_uint32 = (ReadUInt32Fn) Buffer__read_uint32;
    buffer->read_uint64 = (ReadUInt64Fn) Buffer__read_uint64;
    buffer->read_int8 = (ReadInt8Fn) Buffer__read_int8;
    buffer->read_int16 = (ReadInt16Fn) Buffer__read_int16;
    buffer->read_int32 = (ReadInt32Fn) Buffer__read_int32;
    buffer->read_int64 = (ReadInt64Fn) Buffer__read_int64;
    buffer->read_float = (ReadFloatFn) Buffer__read_float;
    buffer->read_double = (ReadDoubleFn) Buffer__read_double;
    buffer->read_cstring = (ReadCStringFn) Buffer__read_cstring;
    buffer->read_string = (ReadStringFn) Buffer__read_string;
}

uint64 Buffer_remaining(Buffer *buffer, BufferError *error) {
    uint64 size;
    uint64 position;
    BufferError tmp_error = buffer->getsize(buffer, &size);
    if (error != NULL) {
        *error = tmp_error;
    }
    if (tmp_error <= BUFFER_FAILED) {
        return 0;
    }

    tmp_error = buffer->get_position(buffer, &position);
    if (error != NULL) {
        *error = tmp_error;
    }
    if (tmp_error <= BUFFER_FAILED) {
        return 0;
    }
    return size - position;
}
