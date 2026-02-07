// Created by RED on 03.10.2025.

#ifndef APEXPREDATOR_HASH_HELPER_H
#define APEXPREDATOR_HASH_HELPER_H

#include "string.h"
#include "string_view.h"

uint32 hash_string(const String *str);

uint32 hash_cstring(const char *str);

uint32 hash_vstring(StringView sv);

#endif //APEXPREDATOR_HASH_HELPER_H
