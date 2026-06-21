// Created by RED on 02.10.2025.

#ifndef APEXPREDATOR_ARCHIVE_MANAGER_H
#define APEXPREDATOR_ARCHIVE_MANAGER_H

#include <utility>
#include <deque>

#include "apex/hashes.h"
#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "redscore/platform/archive_manager.h"

class ApexArchiveManager : public ArchiveManager<u64> {
public:
    ApexArchiveManager() = default;

    [[nodiscard]] bool has(const u64& hash) override;

    [[nodiscard]] bool has(std::string_view name);

    std::unique_ptr<IO::File> get(const u64& hash) override;

    std::unique_ptr<IO::File> get(std::string_view name);

    bool foreach_file(const std::function<bool(const ArchiveEntry &)> &callback) override;

protected:
    std::pair<bool, u64> load_child_archive(const u64 &hash) override {
        if (is_mounted(hash)) {
            return {false, 0};
        }
        auto buffer = get(hash);
        if (!buffer) {
            const auto name = find_name(hash);
            if (name)
                GLog_Error("Failed to load archive \"%s\"", name->data());
            else
                GLog_Error("Failed to load archive with hash 0x%08X", hash);
            return {false, 0};
        }

        std::vector<uint8> first_bytes(16);
        buffer->read_exact(first_bytes);
        buffer->set_position(0);


        if (memcmp(first_bytes.data(), AAF_MAGIC, 4) == 0) {
            const auto name = find_name(hash);
            if (name) {
                GLog_Info("Mounting AAF archive \"{}\"", name->data());
            }
            else {
                GLog_Info("Mounting AAF archive with hash 0x{:08X}", hash);
            }
            AAFArchive aaf_archive(std::move(buffer));

            std::unique_ptr<IO::File> section_buffer = aaf_archive.get_data();

            mount(std::make_unique<SArchive>(hash, std::move(section_buffer)));
            return {true, hash};
        }
        return {false, 0};
    }

private:
    std::pair<bool, uint64> ensure_parent_loaded(uint64 hash) ;
};

#endif //APEXPREDATOR_ARCHIVE_MANAGER_H
