#pragma once

#include "platform/archive_manager.h"
#include "redscore/platform/model/model.hpp"

// Shared application state. Modules initialize their own archive backends.
class ApexAppState {
public:
    explicit ApexAppState(std::filesystem::path game_root) : m_game_root(std::move(game_root)) {}

    const std::filesystem::path &game_root() const { return m_game_root; }
    const std::filesystem::path &export_path() const { return m_export_path; }
    void export_path(const std::filesystem::path &path) { m_export_path = path; }

    ApexArchiveManager &manager() {
        if (!m_archive_manager) throw std::runtime_error("Archive manager has not been initialized");
        return *m_archive_manager;
    }
    // Used by the Generation Zero module and its standalone tools.
    void mount_archives();
    VM::SceneBuilder &models() { return m_models; }

    std::filesystem::path database_path;
    bool skip_textures = false;
    bool extract_raw = false;
    bool root_motion = false;

private:
    std::filesystem::path m_game_root;
    std::filesystem::path m_export_path = "extracted";
    std::shared_ptr<ApexArchiveManager> m_archive_manager;
    VM::SceneBuilder m_models;
};
