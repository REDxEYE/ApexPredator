// Created by RED on 02.10.2025.

#ifndef APEXPREDATOR_ARCHIVE_MANAGER_H
#define APEXPREDATOR_ARCHIVE_MANAGER_H

#include <utility>
#include <vector>
#include <memory>
#include <deque>
#include <unordered_set>
#include "platform/archive.h"

class ArchiveManager: public Archive{
    using load_archive_callback = std::function<std::pair<bool, uint32>(ArchiveManager &manager, uint32 hash)>;

public:

    explicit ArchiveManager(load_archive_callback load_archive)
        : m_load_archive(std::move(load_archive)) {
    }

    [[nodiscard]] bool is_mounted(uint32 archive_hash) const;

    void mount(std::unique_ptr<Archive> archive);

    void unmount(uint32 archive_hash);
    void unmount(std::string_view name);

    [[nodiscard]] bool has_file(uint32 hash) const override;
    [[nodiscard]] bool has_file(std::string_view name) const override;


    std::unique_ptr<IO::File> get_file(uint32 hash) override;
    std::unique_ptr<IO::File> get_file(std::string_view name) override;

    void all_entries(std::vector<ArchiveEntry> &entries) const override;

    [[nodiscard]] std::string get_name() const override {
        return "Root";
    }

    uint32 hash() override {
        return 0;
    }

    ArchiveManager() = delete;
    ArchiveManager(const ArchiveManager&) = delete;
    ArchiveManager& operator=(const ArchiveManager&) = delete;

    ArchiveManager(ArchiveManager&&) noexcept = default;
    ArchiveManager& operator=(ArchiveManager&&) noexcept = default;
private:
    static constexpr size_t MAX_DYNAMIC_MOUNTS = 16;

    std::pair<bool, uint32> ensure_parent_loaded(uint32 hash);
    void touch_dynamic_mount(uint32 hash);
    void evict_dynamic_mounts();
    void forget_dynamic_mount(uint32 hash);

    std::unordered_map<uint32, std::unique_ptr<Archive>> m_archives;
    std::deque<uint32> m_dynamic_mount_order;
    std::unordered_set<uint32> m_dynamic_mount_set;
    load_archive_callback m_load_archive;
};

#endif //APEXPREDATOR_ARCHIVE_MANAGER_H
