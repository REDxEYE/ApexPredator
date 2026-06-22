//
// Created by red_eye on 4/1/26.
//

#include "../commands.h"
#include "exporter/common_export.h"
#include "apex/asset_db.h"

void ExtractEverythingCommand::handle() {
    convert_to_wsl(m_game_root);
    convert_to_wsl(m_export_path);


    AssetDB db(m_db_path);
    AssetDB::set_instance(&db);

    ApexAppState app_state(m_game_root);
    app_state.skip_textures = m_skip_textures;
    app_state.export_path(m_export_path);

    // TODO:
    // std::vector<ArchiveEntry> all_files;
    // app_state.manager().all_entries(all_files);
    // for (const auto &[path_hash, size] : all_files) {
    //     export_file(app_state, path_hash);
    // }
}
