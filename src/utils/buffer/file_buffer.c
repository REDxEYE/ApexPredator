// Created by RED on 17.09.2025.

#include "utils/buffer/file_buffer.h"

#include <assert.h>

#ifndef WIN32
#include <stdio.h>
#endif

static BufferError FileBuffer__set_position(FileBuffer *fb, int64 position, BufferPositionOrigin origin) {
#ifdef WIN32
    if (!fb->hFile) return 0;
    DWORD moveMethod;
    switch (origin) {
        case BUFFER_ORIGIN_START:
            moveMethod = FILE_BEGIN;
            break;
        case BUFFER_ORIGIN_CURRENT:
            moveMethod = FILE_CURRENT;
            break;
        case BUFFER_ORIGIN_END:
            moveMethod = FILE_END;
            break;
        default:
            return 0; // Invalid seek direction
    }
    LARGE_INTEGER distanceToMove;
    distanceToMove.QuadPart = position;
    LARGE_INTEGER newOffset;


    if (FAILED(SetFilePointerEx(fb->hFile, distanceToMove, &newOffset, moveMethod))) {
        return BUFFER_FAILED;
    }
#else
    if (!fb->file)return BUFFER_FAILED;
    int32 moveMethod;
    switch (origin) {
        case BUFFER_ORIGIN_START:
            moveMethod = SEEK_SET;
            break;
        case BUFFER_ORIGIN_CURRENT:
            moveMethod = SEEK_CUR;
            break;
        case BUFFER_ORIGIN_END:
            moveMethod = SEEK_END;
            break;
        default:
            return 0; // Invalid seek direction
    }

    fseek(fb->file, position, moveMethod);
#endif
    return BUFFER_SUCCESS;
}

static BufferError FileBuffer__get_position(FileBuffer *fb, int64 *position) {
#ifdef WIN32
    if (!fb->hFile) return BUFFER_FAILED;
    LARGE_INTEGER zero = {0};
    LARGE_INTEGER currentPos;
    if (FAILED(SetFilePointerEx(fb->hFile, zero, &currentPos, FILE_CURRENT))) {
        return BUFFER_FAILED;
    }
    *position = (uint64) currentPos.QuadPart;
#else
    if (!fb->file)return BUFFER_FAILED;
    *position = ftell(fb->file);
#endif
    return BUFFER_SUCCESS;
}

static BufferError FileBuffer__get_size(FileBuffer *fb, uint64 *size) {
#ifdef WIN32
    if (!fb->hFile) return BUFFER_FAILED;
    LARGE_INTEGER fileSize;
    if (FAILED(GetFileSizeEx(fb->hFile, &fileSize))) {
        return BUFFER_FAILED;
    }
    *size = (uint64) fileSize.QuadPart;
#else
    if (!fb->file)return BUFFER_FAILED;
    int64 o = ftell(fb->file);
    fseek(fb->file, 0,SEEK_END);
    *size = ftell(fb->file);
    fseek(fb->file, o, SEEK_SET);
#endif
    return BUFFER_SUCCESS;
}

static BufferError FileBuffer__read(FileBuffer *fb, void *dst, uint32 size, uint32 *read) {
#ifdef WIN32
    if (!fb->hFile) return BUFFER_FAILED;
    DWORD tmp;

    if (!ReadFile(fb->hFile, dst, size, &tmp, NULL)) {
        return BUFFER_FAILED;
    }
    if (read != NULL) {
        *read = tmp;
    }
    if (tmp < size) {
        return BUFFER_UNDERFLOW;
    }
#else
    if (!fb->file)return BUFFER_FAILED;
    uint64 read_res = fread(dst, 1, size, fb->file);
    if (read != NULL) {
        *read = (uint32) read_res;
    }
    if (read_res == 0) {
        return BUFFER_FAILED;
    }
    if (read_res < size) {
        return BUFFER_UNDERFLOW;
    }
#endif
    return BUFFER_SUCCESS;
}

static BufferError FileBuffer__write(FileBuffer *fb, void *src, uint32 size, uint32 *written) {
#ifdef WIN32
    if (!fb->hFile) return BUFFER_FAILED;
    DWORD tmp;

    if (!WriteFile(fb->hFile, src, size, &tmp, NULL)) {
        return BUFFER_FAILED;
    }
    if (written != NULL) {
        *written = tmp;
    }
    if (tmp < size) {
        return BUFFER_UNDERFLOW;
    }
#else
    if (!fb->file)return BUFFER_FAILED;
    uint64 write_res = fwrite(src, 1, size, fb->file);
    if (written != NULL) {
        *written = (uint32) write_res;
    }
    if (write_res == 0) {
        return BUFFER_FAILED;
    }
    if (write_res < size) {
        return BUFFER_UNDERFLOW;
    }
#endif
    return BUFFER_SUCCESS;
}

BufferError FileBuffer__close(FileBuffer *fb) {
#ifdef WIN32
    if (fb->hFile) {
        CloseHandle(fb->hFile);
        fb->hFile = NULL;
    }
#else
    if (fb->file) {
        fclose(fb->file);
        fb->file = NULL;
    }
#endif
    String_free(&fb->path);
    return BUFFER_SUCCESS;
}

void FileBuffer_init(FileBuffer *fb) {
    Buffer_init((Buffer *) fb);
    fb->set_position = (BufferSetPositionFn) FileBuffer__set_position;
    fb->get_position = (BufferGetPositionFn) FileBuffer__get_position;
    fb->getsize = (BufferGetSizeFn) FileBuffer__get_size;
    fb->read = (BufferReadFn) FileBuffer__read;
    fb->write = (BufferWriteFn) FileBuffer__write;
    fb->close = (BufferCloseFn) FileBuffer__close;
}

BufferError FileBuffer_open(FileBuffer *fb, const char *path) {
    FileBuffer_init(fb);
#ifdef WIN32
    if (fb->hFile != INVALID_HANDLE_VALUE) {
        fb->close(fb);
    }

    String_from_cstr(&fb->path, path);
    fb->hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fb->hFile == INVALID_HANDLE_VALUE) {
        // Handle error
        fb->hFile = NULL;
        return BUFFER_FAILED;
    }

#else
    if (fb->file) {
        fb->close(fb);
    }

    String_from_cstr(&fb->path, path);
    fb->file = fopen(path, "rb");
    if (!fb->file) {
        return BUFFER_FAILED;
    }
#endif
    return BUFFER_SUCCESS;
}
