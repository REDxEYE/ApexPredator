// Created by RED on 01.02.2026.

#ifndef APEXPREDATOR_APP_STATE_H
#define APEXPREDATOR_APP_STATE_H
#include "apex/package/tab_archive.h"
#include "platform/archive_manager.h"

#include "redscore/platform/app_state.h"


class ApexAppState: public AppState {
public:
    explicit ApexAppState(const std::filesystem::path &game_root) : AppState(game_root)  {
        TabArchive::mount_folder(m_archive_manager, m_game_root / "initial");
        TabArchive::mount_folder(m_archive_manager, m_game_root / "optional");
        TabArchive::mount_folder(m_archive_manager, m_game_root / "supplemental");
    }

    ApexArchiveManager &manager();

    [[nodiscard]] const std::filesystem::path &export_path() const;

    void export_path(const std::filesystem::path &path);

    bool skip_textures{false};

private:
    ApexArchiveManager m_archive_manager;
};

#endif //APEXPREDATOR_APP_STATE_H
