#include "modules/archive_mount.h"

#include "apex/package/tab_archive.h"
#include "platform/app_state.h"

#include <filesystem>
#include <memory>
#include <utility>

void mount_game_archives(ApexAppState &app_state) {
    auto manager = std::make_shared<ApexArchiveManager>();
    auto root = app_state.game_root().lexically_normal();
    if (root.filename() == "archives_win64" || root.filename() == "patch_win64" ||
        root.filename() == "feral_overrides" || root.filename() == "dlc_win64")
        root = root.parent_path();
    if (std::filesystem::is_directory(root / "share/data/archives_win64")) root /= "share/data";
    TabArchive::mount_folder_optional(*manager, root / "feral_overrides");
    TabArchive::mount_folder_optional(*manager, root / "patch_win64");
    const auto dlc = root / "dlc_win64";
    if (std::filesystem::is_directory(dlc))
        for (const auto &entry : std::filesystem::directory_iterator(dlc))
            if (entry.is_directory()) TabArchive::mount_folder(*manager, entry.path());
    TabArchive::mount_folder(*manager, root / "archives_win64");
    app_state.set_archive_manager(std::move(manager));
}
