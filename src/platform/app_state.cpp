#include "platform/app_state.h"
#include "apex/package/tab_archive.h"

#include "games.hpp"

void ApexAppState::mount_archives() {
    auto manager = std::make_shared<ApexArchiveManager>();
    auto root = m_game_root;
#if GAME==GAME_JUST_CAUSE_2
    TabArchive::mount_folder(*manager, root / "archives_win32");
    TabArchive::mount_folder(*manager, root / "DLC");
#else
    if (std::filesystem::is_directory(root / "archives_win64")) root /= "archives_win64";
    TabArchive::mount_folder_optional(*manager, root / "supplemental");
    TabArchive::mount_folder_optional(*manager, root / "optional");
    TabArchive::mount_folder(*manager, root / "initial");
#endif
    m_archive_manager = std::move(manager);
}
