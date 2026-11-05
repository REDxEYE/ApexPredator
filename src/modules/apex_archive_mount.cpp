#include "modules/archive_mount.h"

#include "apex/package/tab_archive.h"
#include "platform/app_state.h"

#include <filesystem>
#include <memory>
#include <utility>

void mount_game_archives(ApexAppState &app_state) {
    auto manager = std::make_shared<ApexArchiveManager>();
    auto root = app_state.game_root();
    if (std::filesystem::is_directory(root / "archives_win64")) root /= "archives_win64";
    TabArchive::mount_folder_optional(*manager, root / "supplemental");
    TabArchive::mount_folder_optional(*manager, root / "optional");
    TabArchive::mount_folder(*manager, root / "initial");
    app_state.set_archive_manager(std::move(manager));
}
