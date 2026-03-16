// Created by RED on 02.10.2025.

#include "platform/archive_manager.h"

#include "apex/hashes.h"
#include "platform/logger.h"
#include "tracy/Tracy.hpp"
#include "utils/hash_helper.h"

#include <ranges>
#include <algorithm>

bool ArchiveManager::is_mounted(const uint32 archive_hash) const {
    for (const auto &archive: m_archives | std::views::values) {
        if (archive->hash() == archive_hash) return true;
    }
    return false;
}

void ArchiveManager::mount(std::unique_ptr<Archive> archive) {
    m_archives.emplace(archive->hash(), std::move(archive));
}

void ArchiveManager::unmount(const uint32 archive_hash) {
    forget_dynamic_mount(archive_hash);
    m_archives.erase(archive_hash);
}

void ArchiveManager::unmount(const std::string_view name) {
    unmount(hash_string(name));
}

bool ArchiveManager::has_file(const uint32 hash) const {
    ZoneScoped;
    for (const auto &archive: m_archives | std::views::values) {
        if (archive->has_file(hash)) return true;
    }
    return false;
}

bool ArchiveManager::has_file(const std::string_view name) const {
    return has_file(hash_string(name));
}

std::unique_ptr<IO::File> ArchiveManager::get_file(const uint32 hash) {
    ZoneScoped
    ensure_parent_loaded(hash);

    for (const auto &archive: m_archives | std::views::values) {
        if (auto file = archive->get_file(hash); file) {
            return std::move(file);
        }
    }
    GLog_Error("File with hash 0x{:08X} not found in any archive", hash);
    return nullptr;
}

std::unique_ptr<IO::File> ArchiveManager::get_file(const std::string_view name) {
    return get_file(hash_string(name));
}

void ArchiveManager::all_entries(std::vector<ArchiveEntry> &entries) const {
    ZoneScoped;
    for (const auto &archive: m_archives | std::views::values) {
        archive->all_entries(entries);
    }
}

std::pair<bool, uint32> ArchiveManager::ensure_parent_loaded(const uint32 hash) {
    auto parent_opt = get_file_parent(hash);
    if (!parent_opt || *parent_opt == 0) return {false, 0};

    const auto parent_hash = *parent_opt;
    const auto was_mounted = is_mounted(parent_hash);
    const auto mounted = m_load_archive(*this, parent_hash);

    if (mounted.first) {
        touch_dynamic_mount(mounted.second != 0 ? mounted.second : parent_hash);
        evict_dynamic_mounts();
    }
    else if (was_mounted && m_dynamic_mount_set.contains(parent_hash)) {
        touch_dynamic_mount(parent_hash);
    }

    return mounted;
}

void ArchiveManager::touch_dynamic_mount(const uint32 hash) {
    if (!m_dynamic_mount_set.insert(hash).second) {
        const auto it = std::ranges::find(m_dynamic_mount_order, hash);
        if (it != m_dynamic_mount_order.end()) {
            m_dynamic_mount_order.erase(it);
        }
    }
    m_dynamic_mount_order.push_back(hash);
}

void ArchiveManager::evict_dynamic_mounts() {
    while (m_dynamic_mount_order.size() > MAX_DYNAMIC_MOUNTS) {
        const auto oldest_hash = m_dynamic_mount_order.front();
        m_dynamic_mount_order.pop_front();
        m_dynamic_mount_set.erase(oldest_hash);

        for (auto it = m_archives.begin(); it != m_archives.end();) {
            if (it->second->hash() == oldest_hash) {
                GLog_Info("Evicting {}", it->second->get_name());
                it = m_archives.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

void ArchiveManager::forget_dynamic_mount(const uint32 hash) {
    if (!m_dynamic_mount_set.erase(hash)) {
        return;
    }

    const auto it = std::ranges::find(m_dynamic_mount_order, hash);
    if (it != m_dynamic_mount_order.end()) {
        m_dynamic_mount_order.erase(it);
    }
}
