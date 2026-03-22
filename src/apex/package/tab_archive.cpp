// Created by RED on 18.09.2025.

#include "apex/package/tab_archive.h"
#include "redscore/platform/file/file_buffer.h"
#include "redscore/platform/logger.h"

#include "tracy/Tracy.hpp"
#include "utils/hash_helper.h"

bool TabArchive::has_file(std::string_view path) {
    const uint32 hash = hash_string(path);
    return m_entries.contains(hash);
}

bool TabArchive::has_file(const uint32 hash) {
    return m_entries.contains(hash);
}

std::unique_ptr<IO::File> TabArchive::get_file(const std::string_view path) {
    ZoneScoped
    const uint32 hash = hash_string(path);
    return get_file(hash);
}

std::unique_ptr<IO::File> TabArchive::get_file(const uint32 hash) {
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

void TabArchive::all_entries(std::vector<ArchiveEntry> &entries) const {
    entries.reserve(entries.size() + m_entries.size());
    for (const auto &[hash, tab_entry] : m_entries) {
        entries.emplace_back(hash,tab_entry.size);
    }
}

std::string TabArchive::get_name() const {
    const auto base = m_tab_path.parent_path().parent_path();
    const auto relative_path = std::filesystem::relative(m_tab_path, base);
    return relative_path.string();
}

uint32 TabArchive::hash() {
    return hash_string(get_name());
}

void TabArchive::mount_folder(ArchiveManager &manager, const std::filesystem::path &path) {
    for (std::filesystem::directory_iterator iterator(path); const auto &entry: iterator) {
        if (entry.path().extension() == ".tab") {
            try {
                auto tab_archive = std::make_unique<TabArchive>(entry.path());
                manager.mount(std::move(tab_archive));
            }
            catch (const std::exception &e) {
                GLog_Error("Failed to mount tab archive \"%s\": %s", entry.path().string().c_str(), e.what());
            }
        }
    }
}

void TabArchive::initialize() {
    ZoneScoped
    GLog_Info("Opening tab archive: {}", m_tab_path.string().c_str());

    IO::NativeFile tab_buffer(m_tab_path, std::ios::in | std::ios::binary);
    if (!tab_buffer.stream().is_open()) {
        throw std::runtime_error("Failed to open tab archive file: " + m_tab_path.string());
    }
    const auto header = tab_buffer.read_pod<TabHeader>();

    if (memcmp(header.dwMagic, "TAB\0", 4)!=0) {
        throw std::runtime_error("Invalid TAB archive magic");
    }

    const uint32 entry_count = tab_buffer.remaining() / sizeof(TabEntry);
    m_entries.reserve(entry_count);
    for (uint32 i = 0; i < entry_count; ++i) {
        const TabEntry entry = tab_buffer.read_pod<TabEntry>();
        m_entries[entry.hash] = entry;
    }

}
