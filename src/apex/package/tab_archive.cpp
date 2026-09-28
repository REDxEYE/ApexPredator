// Created by RED on 18.09.2025.

#include "apex/package/tab_archive.h"

#include "tab_v21.hpp"
#include "tab_v31.hpp"
#include "redscore/platform/file/native_file.h"
#include "redscore/platform/logger.h"

#include "tracy/Tracy.hpp"
#include "utils/hash_helper.h"

bool TabArchive::has(std::string_view path) {
    if (!_impl)
        return false;
    return _impl->has(path);
}

bool TabArchive::has(const uint64 &hash) {
    if (!_impl)
        return false;
    return _impl->has(hash);
}

std::unique_ptr<IO::File> TabArchive::get(const std::string_view path) {
    if (!_impl)
        return nullptr;
    return _impl->get(path);
}

std::unique_ptr<IO::File> TabArchive::get(const uint64 &hash) {
    if (!_impl)
        return nullptr;
    return _impl->get(hash);
}

// void TabArchive::all_entries(std::vector<ArchiveEntry> &entries) const {
//     entries.reserve(entries.size() + m_entries.size());
//     for (const auto &[hash, tab_entry] : m_entries) {
//         entries.emplace_back(hash,tab_entry.size);
//     }
// }

std::string_view TabArchive::name() const {
    return m_name;
}

const uint64 &TabArchive::key() const {
    if (!_impl) {
        static uint64 no_value = 0u;
        return no_value;
    }
    return _impl->key();
}

void TabArchive::mount_folder(ArchiveManager<u64> &manager, const std::filesystem::path &path) {
    for (std::filesystem::directory_iterator iterator(path); const auto &entry: iterator) {
        if (entry.path().extension() == ".tab") {
            try {
                auto tab_archive = std::make_unique<TabArchive>(entry.path());
                manager.mount(std::move(tab_archive));
            } catch (const std::exception &e) {
                GLog_Error("Failed to mount tab archive \"{}\": {}", entry.path().string().c_str(), e.what());
            }
        }
    }
}

void TabArchive::mount_folder_optional(ArchiveManager<u64> &manager, const std::filesystem::path &path) {
    if (std::filesystem::is_directory(path)) {
        mount_folder(manager, path);
    }
}

bool TabArchive::foreach_file(const std::function<bool(const ArchiveEntry &)> &callback) {
    if (!_impl) {
        return false;
    }
    return _impl->foreach_file(callback);
}

void TabArchive::initialize() {
    ZoneScoped
    GLog_Info("Opening tab archive: {}", m_tab_path.string().c_str());

    IO::NativeFile tab_buffer(m_tab_path, std::ios::in | std::ios::binary);
    if (!tab_buffer.stream().is_open()) {
        throw std::runtime_error("Failed to open tab archive file: " + m_tab_path.string());
    }
    const auto header = tab_buffer.read_pod<TabV21::TabHeader>();

    if (memcmp(header.dwMagic, "TAB\0", 4) != 0) {
        throw std::runtime_error("Invalid TAB archive magic");
    }
    tab_buffer.set_position(0, std::ios::beg);
    if (header.wMajorVersion == 2 && header.wMinorVersion == 1) {
        _impl = std::make_unique<TabV21::TabV21>(tab_buffer, m_tab_path);
    } else if (header.wMajorVersion == 3 && header.wMinorVersion == 1) {
        _impl = std::make_unique<TabV31::TabV31>(tab_buffer, m_tab_path);
    } else {
        throw std::runtime_error("Unsupported TAB archive version");
    }
}
