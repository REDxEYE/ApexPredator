// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_MEMORY_BUFFER_H
#define APEXPREDATOR_MEMORY_BUFFER_H

#include "utils/buffer/buffer.h"

struct MemoryBuffer : Buffer {
    uint8 *data;
    int64 size;
    int64 capacity;
    int64 position;
    uint32 heap_allocated: 1;
    uint32 owns_data: 1;
    uint32 read_only: 1;
};


MemoryBuffer *MemoryBuffer_new();

BufferError MemoryBuffer_allocate(MemoryBuffer *self, int64 size);

// BufferError MemoryBuffer_copy(MemoryBuffer* self, const Buffer* other);
BufferError MemoryBuffer_from_data(MemoryBuffer *self, const char *data, uint32 data_size);

BufferError MemoryBuffer_make_sub_buffer(MemoryBuffer *self, const MemoryBuffer *parent, int64 offset, int64 size);

#endif //APEXPREDATOR_MEMORY_BUFFER_H
