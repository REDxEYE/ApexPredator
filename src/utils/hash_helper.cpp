// Created by RED on 04.01.2026.
#include "utils/hash_helper.h"

#include <cstring>

#include "utils/lookup3.h"

uint32 hash_string(const std::string &str) {
    return hashlittle(str.c_str(), str.size(), 0);
}

uint32 hash_string(const std::filesystem::path &str) {
    std::string tmp = str.string();
    if constexpr (std::filesystem::path::preferred_separator=='\\') {
        for (char &c : tmp) {
            if (c == '\\') c = '/';
        }
    }
    return hash_string(tmp);
}

uint32 hash_string(const char *str) {
    return hashlittle(str, std::strlen(str), 0);
}

uint32 hash_string(const std::string_view sv) {
    return hashlittle(sv.data(), sv.size(), 0);
}
