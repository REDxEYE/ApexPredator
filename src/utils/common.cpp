// Created by RED on 24.09.2025.

#include "utils/common.h"

#include <cstdlib>
#include <format>

bool compare_hashes(const uint32 *a, const uint32 *b) {
    return *a == *b;
}

bool compare_hashes64(const uint64 *a, const uint64 *b) {
    return *a == *b;
}

bool is_hex(const char * str) {
    if (str == NULL) {
        return false;
    }
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    // Validate string to match 0xAABBCCDD pattern
    if (len == 10 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        for (size_t i = 2; i < len; ++i) {
            if (!((str[i] >= '0' && str[i] <= '9')) &&
                !((str[i] >= 'a' && str[i] <= 'f')) &&
                !((str[i] >= 'A' && str[i] <= 'F'))) {
                return false;
                }
        }
        return true;
    }
    return false;
}

bool is_digits(const char *str) {
    if (str == NULL) {
        return false;
    }
    size_t len = 0;
    while (str[len] != '\0') {
        if (str[len] < '0' || str[len] > '9') {
            return false;
        }
        len++;
    }
    return len > 0;
}

uint32 parse_hex_u32(const char *str) {
    return strtoll(str, NULL, 16);
}

uint32 parse_digits_u32(const char *str) {
    return strtoll(str, NULL, 10);
}

void convert_to_wsl(std::filesystem::path &path) {
#ifndef WSL_ENV
    return;
#else
    if (path.string().empty()) {
        return;
    }
    const std::string tmp_path = path.string();

    if (tmp_path[0] == '.')return;
    const char drive_tmp[2] = {(char) (tmp_path[0] > 'A' ? tmp_path[0] + ' ' : tmp_path[0]), 0};
    const std::string drive = std::string(drive_tmp, 1);

    auto tmp = std::format("/mnt/{}{}", drive, tmp_path.c_str() + 2);
    for (char &c : tmp) {
        if (c == '\\') c = '/';
    }
    path = std::filesystem::path(tmp);

#endif
}
