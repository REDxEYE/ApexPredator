// Created by RED on 17.09.2025.

#ifndef APEXPREDATOR_STRING_H
#define APEXPREDATOR_STRING_H
#include "int_def.h"

enum {
    STRING_EMBEDDED_SIZE = 32 - 8 - 2,
    STRING_EMBEDDED_CAPACITY = 32 - 8,
};

typedef struct String {
    union {
        char *buffer;
        char embeddedBuffer[STRING_EMBEDDED_CAPACITY];
    };

    uint32_t size;
    uint32_t capacity;
} String;

void String_free(String *string);

void String_init(String *string, uint32 size);

void String_from_cstr(String *string, const char *str);

char *String_data(String *string);

void String_append_cstr(String *string, char *str, uint32 size);

void String_append_str(String *string, String *other);

void String_resize(String *string, uint32 size);

void String_trim_zeros(String *string);

void String_sub_string(String *string, uint32 start, int32 size, String *out);

int32 String_find_chr(String *string, char chr);

void String_copy_from(String *string, String *other);
#endif //APEXPREDATOR_STRING_H
