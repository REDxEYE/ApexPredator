#include "utils/hash_helper.h"

#include <stdexcept>

uint64 asset_path_hash(const char *str, const uint32 len) {
    return hashlittle(str, len, 0);
}

uint64 asset_path_hash(const std::string_view path) {
    const auto last = path.find_last_of("/\\");
    const auto name = path.substr(last == std::string_view::npos ? 0 : last + 1);
    if (name.empty()) throw std::invalid_argument("Asset name is empty");
    return asset_path_hash(name.data(), name.size());
}

LegacyHashDomain legacy_hash_domain() noexcept {
    return LegacyHashDomain::Lookup3;
}

std::string_view game_files_table() noexcept {
    return "files_mad_max";
}
