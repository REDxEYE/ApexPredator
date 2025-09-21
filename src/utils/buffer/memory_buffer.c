// Created by RED on 18.09.2025.

#include "utils/buffer/memory_buffer.h"

#include <stdlib.h>
#include <string.h>

static BufferError MemoryBuffer__set_position(MemoryBuffer *fb, int64 position, BufferPositionOrigin origin) {
    uint64 new_position = 0;
    switch (origin) {
        case BUFFER_ORIGIN_START:
            if (position < 0) return BUFFER_FAILED;
            new_position = (uint64) position;
            break;
        case BUFFER_ORIGIN_CURRENT:
            if (position < 0 && (uint64)(-position) > fb->position) return BUFFER_FAILED;
            new_position = fb->position + position;
            break;
        case BUFFER_ORIGIN_END:
            if (position < 0 && (uint64)(-position) > fb->size) return BUFFER_FAILED;
            new_position = fb->size + position;
            break;
        default:
            return BUFFER_FAILED; // Invalid seek direction
    }
    if (new_position > fb->size) {
        return BUFFER_FAILED;
    }
    fb->position = new_position;
    return BUFFER_SUCCESS;
}

static BufferError MemoryBuffer__get_position(MemoryBuffer *fb, int64 *position) {
    *position = fb->position;
    return BUFFER_SUCCESS;
}

static BufferError MemoryBuffer__read(MemoryBuffer *fb, void *dst, uint32 size, uint32 *read) {
    if (fb->position + size > fb->size) {
        size = (uint32)(fb->size - fb->position);
    }
    memcpy(dst, fb->data + fb->position, size);
    fb->position += size;
    if (read) {
        *read = size;
    }
    if (size == 0) {
        return BUFFER_UNDERFLOW;
    }
    return BUFFER_SUCCESS;
}

static BufferError MemoryBuffer__write(MemoryBuffer *fb, const void *src, uint64 size, uint32 *written) {
    if (fb->position + size > fb->capacity) {
        size = (uint32)(fb->capacity - fb->position);
    }
    memcpy(fb->data + fb->position, src, size);
    fb->position += size;
    if (written) {
        *written = (uint32)size;
    }
    if (size == 0) {
        return BUFFER_UNDERFLOW;
    }
    return BUFFER_SUCCESS;
}

static BufferError MemoryBuffer__get_size(MemoryBuffer *fb, uint64 *size) {
    *size = fb->size;
    return BUFFER_SUCCESS;
}

static BufferError MemoryBuffer__close(MemoryBuffer *fb) {
    if (fb->data) {
        free(fb->data);
        fb->data = NULL;
    }
    fb->size = 0;
    fb->capacity = 0;
    fb->position = 0;
    return BUFFER_SUCCESS;
}

BufferError MemoryBuffer__init(MemoryBuffer* mb) {
    Buffer_init((Buffer*)mb);
    mb->data = NULL;
    mb->size = 0;
    mb->capacity = 0;
    mb->position = 0;

    mb->set_position = (BufferSetPositionFn) MemoryBuffer__set_position;
    mb->get_position = (BufferGetPositionFn) MemoryBuffer__get_position;
    mb->read = (BufferReadFn) MemoryBuffer__read;
    mb->write = (BufferWriteFn) MemoryBuffer__write;
    mb->getsize = (BufferGetSizeFn) MemoryBuffer__get_size;
    mb->close = (BufferCloseFn) MemoryBuffer__close;
    return BUFFER_SUCCESS;
}

BufferError MemoryBuffer_allocate(MemoryBuffer *mb, uint64 size) {
    if (mb->data) {
        free(mb->data);
    }
    MemoryBuffer__init(mb);
    mb->data = (uint8 *) malloc(size);
    memset(mb->data, 0, size);
    if (!mb->data) {
        mb->size = 0;
        mb->capacity = 0;
        mb->position = 0;
        return BUFFER_FAILED;
    }
    mb->size = size;
    mb->capacity = size;
    mb->position = 0;
    return BUFFER_SUCCESS;
}
