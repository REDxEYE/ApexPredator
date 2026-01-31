// Created by RED on 18.09.2025.

#include "utils/buffer/memory_buffer.h"

#include <stdlib.h>
#include <string.h>

#include "platform/logger.h"
#include "utils/memory_profiling.h"

static BufferError MemoryBuffer__set_position(MemoryBuffer *self, const int64 position, const BufferPositionOrigin origin) {
    int64 new_position = 0;
    switch (origin) {
        case BUFFER_ORIGIN_START:
            if (position < 0) return BUFFER_FAILED;
            new_position = position;
            break;
        case BUFFER_ORIGIN_CURRENT:
            if (position < 0 && (-position) > self->position) return BUFFER_FAILED;
            new_position = self->position + position;
            break;
        case BUFFER_ORIGIN_END:
            if (position < 0 && (-position) > self->size) return BUFFER_FAILED;
            new_position = self->size + position;
            break;
        default:
            return BUFFER_FAILED; // Invalid seek direction
    }
    if (new_position > self->size) {
        return BUFFER_FAILED;
    }
    self->position = new_position;
    return BUFFER_SUCCESS;
}

static BufferError MemoryBuffer__get_position(MemoryBuffer *self, int64 *position) {
    *position = self->position;
    return BUFFER_SUCCESS;
}

static BufferError MemoryBuffer__read(MemoryBuffer *self, void *dst, uint32 size, uint32 *read) {
    if (self->position + size > self->size) {
        size = (uint32)(self->size - self->position);
    }
    memcpy(dst, self->data + self->position, size);
    self->position += size;
    if (read) {
        *read = size;
    }
    if (size == 0) {
        return BUFFER_UNDERFLOW;
    }
    return BUFFER_SUCCESS;
}

static BufferError MemoryBuffer__write(MemoryBuffer *self, const void *src, uint64 size, uint32 *written) {
    if (self->position + size > self->capacity) {
        size = (uint32)(self->capacity - self->position);
    }
    memcpy(self->data + self->position, src, size);
    self->position += size;
    if (written) {
        *written = (uint32)size;
    }
    if (size == 0) {
        return BUFFER_UNDERFLOW;
    }
    return BUFFER_SUCCESS;
}

static BufferError MemoryBuffer__get_size(const MemoryBuffer *self, uint64 *size) {
    *size = self->size;
    return BUFFER_SUCCESS;
}

static BufferError MemoryBuffer__close(MemoryBuffer *self) {
    if (self->data) {
        mp_free(self->data);
        self->data = NULL;
    }
    self->size = 0;
    self->capacity = 0;
    self->position = 0;
    if (self->heap_allocated) {
        mp_free(self);
    }
    return BUFFER_SUCCESS;
}

BufferError MemoryBuffer__init(MemoryBuffer* self) {
    Buffer_init((Buffer*)self);
    self->data = NULL;
    self->size = 0;
    self->capacity = 0;
    self->position = 0;

    self->set_position = (BufferSetPositionFn) MemoryBuffer__set_position;
    self->get_position = (BufferGetPositionFn) MemoryBuffer__get_position;
    self->read = (BufferReadFn) MemoryBuffer__read;
    self->write = (BufferWriteFn) MemoryBuffer__write;
    self->getsize = (BufferGetSizeFn) MemoryBuffer__get_size;
    self->close = (BufferCloseFn) MemoryBuffer__close;
    return BUFFER_SUCCESS;
}

MemoryBuffer * MemoryBuffer_new() {
    MemoryBuffer *mb = mp_malloc(sizeof(MemoryBuffer));
    if (!mb) {
        GLog_Error("Out of memory");
        exit(1);
    }
    memset(mb, 0, sizeof(MemoryBuffer));
    MemoryBuffer__init(mb);
    mb->heap_allocated = 1;
    return mb;
}

BufferError MemoryBuffer_allocate(MemoryBuffer *self, const int64 size) {
    TracyCZoneN(ctx, "MemoryBuffer_allocate", 1);
    if (self->data) {
        mp_free(self->data);
    }
    MemoryBuffer__init(self);
    self->data = (uint8 *) mp_malloc(size);
    memset(self->data, 0, size);
    if (!self->data) {
        self->size = 0;
        self->capacity = 0;
        self->position = 0;
        TracyCZoneEnd(ctx);
        return BUFFER_FAILED;
    }
    self->size = size;
    self->capacity = size;
    self->position = 0;
    TracyCZoneEnd(ctx);
    return BUFFER_SUCCESS;
}

BufferError MemoryBuffer_from_data(MemoryBuffer *self, const char *data, uint32 data_size) {
    TracyCZoneN(ctx, "MemoryBuffer_from_data", 1);
    if (self->data) {
       MemoryBuffer__close(self);
    }
    MemoryBuffer_allocate(self, data_size);
    memcpy(self->data, data, data_size);
    self->size = data_size;
    self->capacity = data_size;
    self->position = 0;
    TracyCZoneEnd(ctx);
    return BUFFER_SUCCESS;
}
