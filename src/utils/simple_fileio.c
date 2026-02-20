// Created by RED on 18.02.2026.

#include "../../include/utils/simple_fileio.h"

#include "platform/logger.h"


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#else
#include <stdio.h>
#endif

#ifdef _WIN32
bool write_file(const char *path, const uint8 *data, const int64 size) {
    HANDLE file_handle = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        GLog_Error("Failed to open file \"%s\" for writing. Error code: %lu", path, GetLastError());
        return false;
    }

    DWORD bytes_written;
    const BOOL result = WriteFile(file_handle, data, (DWORD)size, &bytes_written, NULL);
    CloseHandle(file_handle);
    if (!result) {
        GLog_Error("Failed to write to file \"%s\". Error code: %lu", path, GetLastError());
        return false;
    }

    if (bytes_written != size) {
        GLog_Error("Failed to write to file \"%s\". Expected to write %lld bytes, but wrote %lu bytes", path, size, bytes_written);
        return false;
    }

    return true;
}
#else
bool write_file(const char *path, const uint8 *data, int64 size) {
    FILE *file = fopen(path, "wb");
    if (!file) {
        GLog_Error("Failed to open file \"%s\" for writing", path);
        return false;
    }

    const size_t bytes_written = fwrite(data, 1, (size_t)size, file);
    fclose(file);
    if (bytes_written!=size) {
        GLog_Error("Failed to write to file \"%s\". Expected to write %lld bytes, but wrote %zu bytes", path, size, bytes_written);
        return false;
    }

    return true;
}

#endif