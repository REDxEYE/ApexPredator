// Created by RED on 24.09.2025.

#ifndef APEXPREDATOR_COMMON_H
#define APEXPREDATOR_COMMON_H

#include <filesystem>
#include <stdbool.h>
#include "int_def.h"

bool compare_hashes(const uint32 *a, const uint32 *b);
bool compare_hashes64(const uint64 *a, const uint64 *b);

bool is_hex(const char * str);

bool is_digits(const char * str);

uint32 parse_hex_u32(const char * str);

uint32 parse_digits_u32(const char * str);

#define ALIGN_UP(value, alignment) (((value) + (alignment - 1)) & ~(alignment - 1))

void convert_to_wsl(std::filesystem::path &path);

namespace path_utils {

    inline std::string_view filename(std::string_view p) {
        const size_t pos = p.find_last_of("/\\");
        return pos == std::string_view::npos ? p : p.substr(pos + 1);
    }

    inline std::string_view extension(std::string_view p) {
        const auto name = filename(p);
        const size_t pos = name.find_last_of('.');
        return pos == std::string_view::npos ? std::string_view{} : name.substr(pos);
    }

    inline std::string_view stem(std::string_view p) {
        const auto name = filename(p);
        const size_t pos = name.find_last_of('.');
        return pos == std::string_view::npos ? name : name.substr(0, pos);
    }

}

#endif //APEXPREDATOR_COMMON_H
