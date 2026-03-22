// Created by RED on 01.02.2026.

#ifndef APEXPREDATOR_APP_STATE_H
#define APEXPREDATOR_APP_STATE_H
#include <format>
#include "tiny_gltf.h"
#include "apex/hashes.h"
#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "apex/package/tab_archive.h"
#include "platform/archive_manager.h"
#include "redscore/utils/common.h"
#include "redscore/platform/gltf_helper.h"
#include "redscore/platform/logger.h"

std::pair<bool, uint32 > inline mount_archive(ArchiveManager &manager, const uint32 hash) {
    if (manager.is_mounted(hash)) {
        return {false, 0};
    }
    auto buffer = manager.get_file(hash);
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

        manager.mount(std::make_unique<SArchive>(hash, std::move(section_buffer)));
        return {true, hash};
    }
    return {false, 0};
}

class AppState {
public:
    AppState(const std::filesystem::path &game_root) : AppState() {
        m_game_root = game_root;
        convert_to_wsl(m_game_root);

        TabArchive::mount_folder(m_archive_manager, m_game_root / "initial");
        TabArchive::mount_folder(m_archive_manager, m_game_root / "optional");
        TabArchive::mount_folder(m_archive_manager, m_game_root / "supplemental");
    }

    AppState() : m_archive_manager(mount_archive) {
    }

    ArchiveManager &manager();

    [[nodiscard]] const std::filesystem::path &export_path() const;

    void export_path(const std::filesystem::path &path);

    GltfHelper &helper() {
        return m_gltf_helper;
    }

    bool skip_textures{false};

private:
    GltfHelper m_gltf_helper{};
    ArchiveManager m_archive_manager;
    std::filesystem::path m_game_root;
    std::filesystem::path m_export_path;

};

#endif //APEXPREDATOR_APP_STATE_H
