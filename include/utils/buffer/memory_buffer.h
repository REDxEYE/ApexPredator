// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_MEMORY_BUFFER_H
#define APEXPREDATOR_MEMORY_BUFFER_H

#include "utils/buffer/buffer.h"

typedef struct {
    struct Buffer_s;
    uint8* data;
    uint64 size;
    uint64 capacity;
    uint64 position;
}MemoryBuffer;

BufferError MemoryBuffer_allocate(MemoryBuffer* mb, uint64 size);


#endif //APEXPREDATOR_MEMORY_BUFFER_H