// Created by RED on 02.10.2025.

#ifndef APEXPREDATOR_ARCHIVE_MANAGER_H
#define APEXPREDATOR_ARCHIVE_MANAGER_H

#include <utility>
#include <deque>
#include "redscore/platform/archive_manager.h"

class ApexArchiveManager : public ArchiveManager {
    using load_archive_callback = std::function<std::pair<bool, uint32>(ArchiveManager &manager, uint32 hash)>;

public:
    explicit ApexArchiveManager(const load_archive_callback &load_archive)
        : ArchiveManager(load_archive) {
    }

    [[nodiscard]] bool has_file(uint32 hash) override;

    [[nodiscard]] bool has_file(std::string_view name) override;

    std::unique_ptr<IO::File> get_file(uint32 hash) override;

    std::unique_ptr<IO::File> get_file(std::string_view name) override;

private:
    std::pair<bool, uint32> ensure_parent_loaded(uint32 hash) ;
};

#endif //APEXPREDATOR_ARCHIVE_MANAGER_H
