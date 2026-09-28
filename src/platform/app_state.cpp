#include "platform/app_state.h"
#include "apex/package/tab_archive.h"

void ApexAppState::mount_archives() {
    auto manager = std::make_shared<ApexArchiveManager>();
    auto root = m_game_root;
    if (std::filesystem::is_directory(root / "archives_win64")) root /= "archives_win64";
    TabArchive::mount_folder_optional(*manager, root / "supplemental");
    TabArchive::mount_folder_optional(*manager, root / "optional");
    TabArchive::mount_folder(*manager, root / "initial");
    m_archive_manager = std::move(manager);
}
