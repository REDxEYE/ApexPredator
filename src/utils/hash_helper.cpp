// Created by RED on 04.01.2026.
#include "utils/hash_helper.h"
#include "games.hpp"

#include <cstring>

#include "utils/lookup3.h"
#include "utils/murmur3.h"


#if GAME==GAME_GENERATION_ZERO
uint64 asset_path_hash(const char *str, const uint32 len) {
    return hashlittle(str, len, 0);
}
#elif GAME==GAME_RAGE2
uint64 asset_path_hash(const char *str, const uint32 len) {
    return MurmurHash3_x64_128(str, len, 0);
}
#endif

uint64 asset_path_hash(const std::string_view sv) {
    return asset_path_hash(sv.data(), sv.size());
}

uint64 hash_string(const std::string &str) {
    return hashlittle(str.c_str(), str.size(), 0);
}

uint64 hash_string(const std::filesystem::path &str) {
    std::string tmp = str.string();
    if constexpr  (std::filesystem::path::preferred_separator == '\\') {
        for (char &c: tmp) {
            if (c == '\\') c = '/';
        }
    }
    return hash_string(tmp);
}

uint64 hash_string(const char *str) {
    return hashlittle(str, std::strlen(str), 0);
}

uint64 hash_string(const std::string_view sv) {
    return hashlittle(sv.data(), sv.size(), 0);
}

StringHashes string_hashes(std::string_view value) {
    StringHashes result{hashlittle(value.data(), value.size(), 0), 0};
#if GAME==GAME_RAGE2
    result.murmur = MurmurHash3_x64_128(value.data(), value.size(), 0);
#endif
    return result;
}
