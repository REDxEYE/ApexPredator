// Created by RED on 01.02.2026.

#include "platform/app_state.h"

ArchiveManager &AppState::manager() {
    return m_archive_manager;
}

const std::filesystem::path & AppState::export_path() const {
    return m_export_path;
}

void AppState::export_path(const std::filesystem::path &path) {
    m_export_path = path;
}
