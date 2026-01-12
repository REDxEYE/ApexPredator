// Created by RED on 04.01.2026.
#include "utils/hash_helper.h"
#include "utils/lookup3.h"

uint32 hash_string(const String *str) {
    return hashlittle(String_data(str), str->size, 0);
}

uint32 hash_cstring(const char *str) {
    return hashlittle(str, strlen(str), 0);
}
