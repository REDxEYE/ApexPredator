// Created by RED on 15.02.2026.

#ifndef APEXPREDATOR_PATTERNS_H
#define APEXPREDATOR_PATTERNS_H
#include "stdint.h"

typedef struct Pattern {
    uint8_t *bytes;
    uint8_t *mask; // 1 = must match, 0 = wildcard
    size_t len;
} Pattern;

void pattern_free(Pattern *p);

int pattern_compile(Pattern *out, const char *pattern_str);

const uint8_t *pattern_find_first(const void *data, size_t size, const Pattern *p);

typedef struct Region {
    uintptr_t base;
    uintptr_t end;
}Region;

Region get_module();
Region get_pe_section(Region module, const char* section);

#endif //APEXPREDATOR_PATTERNS_H
