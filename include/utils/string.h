// Created by RED on 17.09.2025.

#ifndef APEXPREDATOR_STRING_H
#define APEXPREDATOR_STRING_H
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "int_def.h"

typedef struct String {
    char *buffer;
    uint32 size;
    uint32 capacity;
    uint32 can_be_moved: 1;
} String;

void String_free(String *string);

void String_init(String *string, uint32 size);

String *String_from_cstr(String *string, const char *str);

const char *String_data(const String *string);

void String_append_cstr(String *string, char *str);

void String_append_cstr2(String *string, char *str, uint32 size);

void String_append_str(String *string, String *other);

void String_resize(String *string, uint32 size);

void String_trim_zeros(String *string);

void String_sub_string(const String *string, uint32 start, int32 size, String *out);

int32 String_find_chr(const String *string, char chr);

void String_copy_from(String *string, String *other);

void String_format(String *string, const char *fmt, ...);

void String_append_format(String *string, const char *fmt, ...);

bool String_equals(const String *string, const String *other);

static  inline String *String_move(String *string) {
    string->can_be_moved = 1;
    return string;
}

static  inline String *String_steal(String* string, String* other) {
    if (!other->can_be_moved) {
        printf("Error: trying to steal from string that is not marked movable\n");
        exit(1);
    }
    if (string->can_be_moved) {
        printf("Error: trying to steal into string that is marked movable\n");
        exit(1);
    }

    char* buffer = other->buffer;
    other->buffer = NULL;
    if (string->buffer!=NULL) {
        free(string->buffer);
    }
    string->buffer = buffer;
    string->size = other->size;
    string->capacity = other->capacity;
    other->size = 0;
    other->capacity = 0;
    return string;
}

static inline char* String_detach(String* string) {
    if (!string->can_be_moved) {
        printf("Error: trying to detach from string that is not marked movable\n");
        exit(1);
    }
    char* buffer = string->buffer;
    string->buffer = NULL;
    string->size = 0;
    string->capacity = 0;
    return buffer;
}

static inline uint32 String_find_subcstring(String* string, const char* sub) {
    char* found = strstr(string->buffer, sub);
    return found ? (uint32) (found - string->buffer) : UINT32_MAX;
}

#endif //APEXPREDATOR_STRING_H
