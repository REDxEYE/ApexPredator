// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_MEMORY_BUFFER_H
#define APEXPREDATOR_MEMORY_BUFFER_H

#include "utils/buffer/buffer.h"

typedef struct {
    struct Buffer_s;
    uint8* data;
    int64 size;
    int64 capacity;
    int64 position;
    uint32 heap_allocated:1;
}MemoryBuffer;

MemoryBuffer* MemoryBuffer_new();
BufferError MemoryBuffer_allocate(MemoryBuffer* mb, int64 size);


#endif //APEXPREDATOR_MEMORY_BUFFER_H