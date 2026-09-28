// Created by RED on 03.10.2025.

#ifndef APEXPREDATOR_HASH_HELPER_H
#define APEXPREDATOR_HASH_HELPER_H
#include <string>
#include <string_view>
#include <filesystem>

#include "int_def.h"


struct StringHashes {
    uint32 lookup3;
    uint64 murmur;
};

uint64 asset_path_hash(const char *str, uint32 len);

uint64 asset_path_hash(std::string_view sv);

StringHashes string_hashes(std::string_view value);

uint64 hash_string(const std::string &str);

uint64 hash_string(const std::filesystem::path &str);

uint64 hash_string(const char *str);

uint64 hash_string(std::string_view sv);

#endif //APEXPREDATOR_HASH_HELPER_H
