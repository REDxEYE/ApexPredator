#include "modules/archive_mount.h"

#include "apex/package/tab_archive.h"
#include "platform/app_state.h"

#include <filesystem>
#include <memory>
#include <utility>

void mount_game_archives(ApexAppState &app_state) {
    auto manager = std::make_shared<ApexArchiveManager>();
    auto root = app_state.game_root().lexically_normal();
    if (root.filename().empty()) root = root.parent_path();
    if (root.filename() == "archives_win32") root = root.parent_path();
    TabArchive::mount_folder(*manager, root / "archives_win32");
    TabArchive::mount_folder(*manager, root / "DLC");
    app_state.set_archive_manager(std::move(manager));
}
