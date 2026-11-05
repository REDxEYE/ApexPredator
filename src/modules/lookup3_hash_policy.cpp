#include "utils/hash_helper.h"

uint64 asset_path_hash(const char *str, const uint32 len) {
    return hashlittle(str, len, 0);
}

uint64 asset_path_hash(const std::string_view path) {
    return asset_path_hash(path.data(), path.size());
}

LegacyHashDomain legacy_hash_domain() noexcept {
    return LegacyHashDomain::Lookup3;
}

std::string_view game_files_table() noexcept {
    return "files_generation_zero";
}
