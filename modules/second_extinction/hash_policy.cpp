#include "utils/hash_helper.h"
#include "utils/murmur3.h"

uint64 asset_path_hash(const char *str, const uint32 len) {
    return MurmurHash3_x64_128(str, len, 0);
}

uint64 asset_path_hash(const std::string_view path) {
    return asset_path_hash(path.data(), path.size());
}

LegacyHashDomain legacy_hash_domain() noexcept {
    return LegacyHashDomain::Murmur;
}

std::string_view game_files_table() noexcept {
    return "files_second_extinction";
}
