// Created by RED on 24.09.2025.

#ifndef APEXPREDATOR_COMMON_H
#define APEXPREDATOR_COMMON_H

#include <stdbool.h>
#include "int_def.h"

bool compare_hashes(const uint32 *a, const uint32 *b);
bool compare_hashes64(const uint64 *a, const uint64 *b);

bool is_hex(const char * str);

bool is_digits(const char * str);

uint32 parse_hex_u32(const char * str);

uint32 parse_digits_u32(const char * str);

#endif //APEXPREDATOR_COMMON_H