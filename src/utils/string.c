// Created by RED on 17.09.2025.

#include "utils/string.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int String__is_heap(const String *s) {
    return s->capacity > STRING_EMBEDDED_CAPACITY;
}

void String_free(String *string) {
    if (string->capacity > STRING_EMBEDDED_CAPACITY) {
        free(string->buffer);
        string->buffer = NULL;
    } else
        string->embeddedBuffer[0] = '\0';

    string->size = 0;
    string->capacity = 0;
}

void String_init(String *string, uint32 size) {
    if (string->capacity != 0) {
        if (size + 1 > string->capacity) {
            String_free(string);
            string->buffer = malloc(size + 1);
            assert(string->buffer && "Out of memory");
            memset(string->buffer, 0, size + 1);
            string->size = 0;
            string->capacity = size + 1;
            return;
        }
        string->size = 0;
        memset(String_data(string), 0, string->capacity);
        return;
    }
    string->size = 0;
    if (size + 1 >= STRING_EMBEDDED_CAPACITY) {
        String_free(string);
        string->buffer = (char *) malloc(size + 1);
        string->capacity = size + 1;
        memset(string->buffer, 0, string->capacity);
    } else {
        string->capacity = STRING_EMBEDDED_CAPACITY;
        string->embeddedBuffer[0] = '\0';
    }
}

void String_from_cstr(String *string, const char *str) {
    const size_t len = strlen(str);
    assert(len<UINT32_MAX && "Input CStr is to long");
    String_init(string, len);
    if (len + 1 > STRING_EMBEDDED_CAPACITY) {
        assert(string->buffer!=NULL&& "Failed to allocate string buffer.");
    }

    memcpy(String_data(string), str, len);
    String_data(string)[len] = '\0';
    string->size = len;
}

char *String_data(String *string) {
    return String__is_heap(string) ? string->buffer : string->embeddedBuffer;
}

void String_resize(String *s, uint32_t size) {
    uint32_t new_capacity = size + 1;

    if (s->capacity == 0) {
        s->size = 0;
        s->capacity = STRING_EMBEDDED_CAPACITY;
        s->embeddedBuffer[0] = '\0';
    }

    if (new_capacity > s->capacity) {
        if (String__is_heap(s)) {
            char *p = realloc(s->buffer, new_capacity);
            assert(p && "Out of memory");
            s->buffer = p;
            s->capacity = new_capacity;
        } else {
            char *p = malloc(new_capacity);
            assert(p && "Out of memory");
            if (s->size) memcpy(p, s->embeddedBuffer, s->size);
            memset(p + s->size, 0, (size_t) new_capacity - s->size);
            s->buffer = p;
            s->capacity = new_capacity;
        }
    } else {
        if (new_capacity <= STRING_EMBEDDED_CAPACITY) {
            if (String__is_heap(s)) {
                size_t to_copy = size < s->size ? size : s->size;
                if (to_copy) {
                    char *src = s->buffer;
                    memcpy(s->embeddedBuffer, src, to_copy);
                    free(src);
                }
                s->capacity = STRING_EMBEDDED_CAPACITY;
            }
        } else {
            if (String__is_heap(s)) {
                char *p = (char *) realloc(s->buffer, new_capacity);
                assert(p && "Out of memory");
                s->buffer = p;
                s->capacity = new_capacity;
            }
            /* staying in heap; nothing else needed */
        }
    }
    String_data(s)[size] = '\0';
}

void String_trim_zeros(String *string) {
    uint32 actual_len = strlen(String_data(string));
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
    memcpy(String_data(out), String_data(string) + start, length);
    out->size = length;
    String_data(out)[length] = '\0';
}

int32 String_find_chr(String *string, char chr) {
    char *found = strchr(String_data(string), chr);
    return found ? (int32) (found - String_data(string)) : -1;
}

void String_copy_from(String *string, String *other) {
    String_init(string, other->size);
    memcpy(String_data(string), String_data(other), other->size);
    string->size = other->size;
    String_data(string)[string->size] = '\0';
}

void String_append_cstr(String *string, char *str, uint32 size) {
    if (string->size + size >= string->capacity) {
        String_resize(string, string->size + size);
    }

    memcpy(String_data(string) + string->size, str, size);
    string->size += size;
    String_data(string)[string->size] = '\0';
}

void String_append_str(String *string, String *other) {
    String_append_cstr(string, String_data(other), other->size);
}
