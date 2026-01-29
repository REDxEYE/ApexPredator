// Created by RED on 17.09.2025.

#ifndef APEXPREDATOR_STRING_H
#define APEXPREDATOR_STRING_H
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "int_def.h"
#include "platform/memory_profiling.h"

#define Z_MALLOC(sz)      mp_malloc(sz)
#define Z_CALLOC(n, sz)  mp_calloc((n), (sz))
#define Z_REALLOC(p, sz)  mp_realloc((p), (sz))
#define Z_FREE(p)         mp_free(p)

#include "zstr.h"


typedef struct String {
    zstr s;
    uint8 heap_allocated:1;
} String;

void String_free(String *string);
String* String_new(uint32 size);
String* String_new_from_cstr(const char *str);
String* String_new_from_cstr2(const char *str, uint32 len);
String* String_new_from_str(const String* other);
void String_init(String *string, uint32 size);

String *String_from_cstr(String *string, const char *str);
String *String_from_cstr2(String *string, const char *str, uint32 len);

const char *String_cstr(const String *string);
char *String_data(String *string);
uint32 String_size(const String *string);
void String_set_size(String *string, uint32 size);

void String_append_cstr(String *string, const char *str);
void String_append_cstr2(String *string, const char *str, uint32 size);
void String_append_str(String *string, const String *other);

void String_reserve(String *string, uint32 size);
void String_trim_zeros(String *string);
void String_fill(String *string, uint32 offset, uint32 size, char chr);

void String_sub_string(const String *string, uint32 start, int32 size, String *out);
int32 String_find_chr(const String *string, char chr);

void String_copy_from(String *dst, const String *src);
void String_move_from(String *dst, String *src);

void String_format(String *string, const char *fmt, ...);
void String_append_format(String *string, const char *fmt, ...);
void String_prepend_format(String *string, const char *fmt, ...);

bool String_equals(const String *string, const String *other);
bool String_cequals(const String *string, const char* other);
bool String_ends_with(const String *string, const String *suffix);
bool String_cends_with(const String *string, const char *suffix);
bool String_cstarts_with(const String *string, const char *prefix);

char *String_detach(String *string);

uint32 String_find_subcstring(const String *string, const char *sub);

void String_replace_char(String *string, const char *targets, char replacement);

#endif //APEXPREDATOR_STRING_H
