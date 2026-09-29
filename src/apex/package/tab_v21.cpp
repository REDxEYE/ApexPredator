//
// Created by red_eye on 9/24/26.
//

#include "tab_v21.hpp"

bool TabV21::TabV21::has(std::string_view path) {
    const uint64 hash = asset_path_hash(path);
    return m_entries.contains(hash);
}

bool TabV21::TabV21::has(const uint64& hash) {
    return m_entries.contains(hash);
}

std::unique_ptr<IO::File> TabV21::TabV21::get(const std::string_view path) {
    ZoneScoped
    const uint64 hash = asset_path_hash(path);
    return get(hash);
}

std::unique_ptr<IO::File> TabV21::TabV21::get(const uint64& hash) {
    ZoneScoped
    const auto it = m_entries.find(hash);
    if (it == m_entries.end()) {
        return nullptr;
    }
    const TabEntry &entry = it->second;
    arc_buffer.set_position(entry.offset, std::ios::beg);
    auto buffer = std::vector<uint8>(entry.size);
    arc_buffer.read_exact(buffer);
    return std::move(std::make_unique<IO::MemoryFile>(std::move(buffer)));
}


const u64 & TabV21::TabV21::key() const {
    return m_hash;
}

bool TabV21::TabV21::foreach_file(const std::function<bool(const Archive<u64>::ArchiveEntry &)> &callback) {
    for (const auto &[hash, entry] : m_entries)
        if (!callback({hash, 0, entry.size})) return false;
    return true;
}
