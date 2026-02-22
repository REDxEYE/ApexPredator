// Created by RED on 17.09.2025.

#ifndef APEXPREDATOR_FILE_BUFFER_H
#define APEXPREDATOR_FILE_BUFFER_H

#include "utils/buffer/buffer.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

struct FileBuffer : Buffer {
    String path;
    HANDLE hFile;
};


#else
#include <stdio.h>

struct FileBuffer:Buffer {
    String path;
    FILE *file;
} ;

#endif

BufferError FileBuffer_open_read(FileBuffer *fb, const char *path);

BufferError FileBuffer_open_write(FileBuffer *fb, const char *path);
#endif //APEXPREDATOR_FILE_BUFFER_H
