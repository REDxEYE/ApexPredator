// Created by RED on 02.10.2025.

#include "platform/archive_manager.h"

#include "apex/hashes.h"
#include "redscore/platform/logger.h"
#include "tracy/Tracy.hpp"
#include "utils/hash_helper.h"

#include <ranges>
#include <algorithm>

bool ApexArchiveManager::has(const uint64& hash) {
    ZoneScoped;
    ensure_parent_loaded(hash);
    for (const auto &archive: m_archives | std::views::values) {
        if (archive->has(hash)) return true;
    }
    return false;
}

bool ApexArchiveManager::has(const std::string_view name) {
    return has(hash_string(name));
}


std::unique_ptr<IO::File> ApexArchiveManager::get(const uint64& hash) {
    // ZoneScoped
    ensure_parent_loaded(hash);

    for (const auto &archive: m_archives | std::views::values) {
        if (auto file = archive->get(hash); file) {
            return std::move(file);
        }
    }
    GLog_Error("File with hash 0x{:08X} not found in any archive", hash);
    return nullptr;
}

std::unique_ptr<IO::File> ApexArchiveManager::get(const std::string_view name) {
    return get(hash_string(name));
}

bool ApexArchiveManager::foreach_file(const std::function<bool(const ArchiveEntry &)> &callback) {
    for (const auto &archive: m_archives | std::views::values) {
        if (!archive->foreach_file(callback)) {
            return false;
        }
    }
    return true;
}

std::pair<bool, uint64> ApexArchiveManager::ensure_parent_loaded(const uint64 hash){
    auto parent_opt = get_file_parent(hash);
    if (!parent_opt || *parent_opt == 0) return {false, 0};

    const auto parent_hash = *parent_opt;
    const auto was_mounted = is_mounted(parent_hash);
    const auto mounted = load_child_archive(parent_hash);

    if (mounted.first) {
        touch_dynamic_mount(mounted.second != 0 ? mounted.second : parent_hash);
        evict_dynamic_mounts();
    }
    else if (was_mounted && m_dynamic_mount_set.contains(parent_hash)) {
        touch_dynamic_mount(parent_hash);
    }

    return mounted;
}
