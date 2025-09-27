// Created by RED on 17.09.2025.

#include "utils/string.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

void String_free(String *string) {
    if (string->buffer!=NULL) {
        free(string->buffer);
        string->buffer = NULL;
    }
    string->size = 0;
    string->capacity = 0;
}

void String_init(String *string, uint32 size) {
    string->can_be_moved = 0;
    if (string->buffer!=NULL) {
        if (size + 1 > string->capacity) {
            String_resize(string, size);
            assert(string->buffer && "Out of memory");
            memset(string->buffer, 0, size + 1);
            string->size = 0;
            string->capacity = size + 1;
            return;
        }
        string->size = 0;
        memset(string->buffer, 0, string->capacity);
        return;
    }
    string->size = 0;
    string->buffer = (char *) malloc(size + 1);
    string->capacity = size + 1;
    memset(string->buffer, 0, string->capacity);
}

String* String_from_cstr(String *string, const char *str) {
    const size_t len = strlen(str);
    assert(len<UINT32_MAX && "Input CStr is to long");
    String_init(string, len);
    memcpy(string->buffer, str, len);
    string->buffer[len] = '\0';
    string->size = len;

    return string;
}

char *String_data(const String *string) {
    if (string->can_be_moved) {
        printf("Warning, using string that can be moved\n");
    }
    if (string->buffer==NULL) {
        printf("Error: trying to get data from uninitialized string\n");
        exit(1);
    }
    return string->buffer;
}

void String_append_cstr(String *string, char *str) {
    if (!str) return;
    uint32 str_len = (uint32) strlen(str);
    String_append_cstr2(string, str, str_len);
}

void String_resize(String *string, uint32_t size) {
    uint32_t new_capacity = size + 1;
    if (new_capacity > string->capacity) {
        char *p = realloc(string->buffer, new_capacity);
        assert(p && "Out of memory");
        string->buffer = p;
        string->capacity = new_capacity;
    }
    string->buffer[size] = '\0';
}

void String_trim_zeros(String *string) {
    uint32 actual_len = strlen(string->buffer);
    assert(actual_len<UINT32_MAX && "Invalid string.");
    String_resize(string, actual_len);
}

void String_sub_string(String *string, uint32 start, int32 size, String *out) {
    if (start >= string->size || size == 0) {
        String_init(out, 0);
        return;
    }

    uint32 available = string->size - start;
    uint32 length;

    if (size > 0) {
        length = (size < (int32) available) ? (uint32) size : available;
    } else {
        length = available;
    }

    String_init(out, length);
    memcpy(out->buffer, string->buffer + start, length);
    out->size = length;
    out->buffer[length] = '\0';
}

int32 String_find_chr(String *string, char chr) {
    char *found = strchr(string->buffer, chr);
    return found ? (int32) (found - string->buffer) : -1;
}

void String_copy_from(String *string, String *other) {
    if (other->can_be_moved) {
        String_steal(string, other);
        return;
    }

    String_init(string, other->size);
    memcpy(string->buffer, String_data(other), other->size);
    string->size = other->size;
    string->buffer[string->size] = '\0';
}

void String_format(String *string, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (needed < 0) {
        String_init(string, 0);
        return;
    }
    String_init(string, (uint32) needed);
    va_start(args, fmt);
    vsnprintf(string->buffer, string->capacity, fmt, args);
    va_end(args);
    string->size = strlen(string->buffer);
}

void String_append_format(String *string, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (needed < 0) {
        return;
    }
    if (string->size + needed + 1 > string->capacity) {
        String_resize(string, string->size + needed);
    }
    va_start(args, fmt);
    vsnprintf(string->buffer + string->size, string->capacity - string->size, fmt, args);
    va_end(args);
    string->size += needed;
}

bool String_equals(String *string, String *other) {
    if (string->size != other->size) {
        return false;
    }
    return memcmp(string->buffer, other->buffer, string->size) == 0;
}

void String_append_cstr2(String *string, char *str, uint32 size) {
    if (string->size + size >= string->capacity) {
        String_resize(string, string->size + size);
    }

    memcpy(string->buffer + string->size, str, size);
    string->size += size;
    string->buffer[string->size] = '\0';
}

void String_append_str(String *string, String *other) {
    String_append_cstr2(string, other->buffer, other->size);
}
