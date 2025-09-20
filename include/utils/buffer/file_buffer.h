// Created by RED on 17.09.2025.

#ifndef APEXPREDATOR_FILE_BUFFER_H
#define APEXPREDATOR_FILE_BUFFER_H

#include "utils/buffer/buffer.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

typedef struct {
    struct Buffer_s;
    String path;
    HANDLE hFile;
} FileBuffer;


#else
#include <stdio.h>

typedef struct {
    struct Buffer_s;
    String path;
    FILE *file;
} FileBuffer;

#endif

BufferError FileBuffer_open(FileBuffer *fb, const char *path);

#endif //APEXPREDATOR_FILE_BUFFER_H
