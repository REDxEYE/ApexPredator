// Created by RED on 16.02.2026.

#include "../commands.h"

#include "tiny_gltf.h"
#include "apex/asset_db.h"
#include "exporter/common_export.h"
#include "platform/app_state.h"
#include "redscore/platform/logger.h"
#include "redscore/utils/common.h"
#include "redscore/utils/simple_fileio.h"
#include "tracy/Tracy.hpp"
#include "utils/hash_helper.h"

void raw_export(ApexAppState &app_state, const uint32 asset_hash) {
    ZoneScoped
    const auto asset_path = find_name(asset_hash)
            .or_else([&] { return std::optional{std::format("{:08X}.bin", asset_hash)}; })
            .value();


    const std::filesystem::path save_path = app_state.export_path() / asset_path;
    std::filesystem::create_directories(save_path.parent_path());

    const auto mb = app_state.manager().get_file(asset_hash);
    if (!mb) {
        GLog_Error("File not found: {}", asset_hash);
        return;
    }

    write_file(save_path, mb->cbuffer());
    GLog_Info("File \"{}\" extracted to \"{}\"", asset_hash, save_path.string());
}

void normal_export(ApexAppState &app_state, const uint32 asset_hash) {
    ZoneScoped
    const auto asset_path = find_name(asset_hash)
            .or_else([&] { return std::optional{std::format("{:08X}.bin", asset_hash)}; })
            .value();

    std::filesystem::path save_path = app_state.export_path() / asset_path;
    save_path.replace_extension("gltf");

    auto &gltf_helper = app_state.helper();
    const auto node = export_file(app_state, asset_hash);
    if (node.is_valid()) {
        gltf_helper.add_to_scene(node);
    }
    if (!gltf_helper.model().scenes.empty() && !gltf_helper.model().nodes.empty()) {
        std::filesystem::create_directories(save_path.parent_path());
        tinygltf::TinyGLTF gltf_exporter;
        if (gltf_exporter.WriteGltfSceneToFile(&gltf_helper.model(), save_path.string(), false, true, true, false)) {
            GLog_Info("Written GLTF file: {}", save_path.string());
        }else {
            GLog_Error("Failed to write GLTF file: {}", save_path.string());
        }

    }
}

void ExtractCommand::handle() {
    convert_to_wsl(m_game_root);
    convert_to_wsl(m_export_path);


    AssetDB db(m_db_path);
    AssetDB::set_instance(&db);

    ApexAppState app_state(m_game_root);
    app_state.skip_textures = m_skip_textures;
    app_state.export_path(m_export_path);

    for (const auto &asset: m_assets) {
        uint32 file_hash;
        if (is_hex(asset.c_str())) {
            file_hash = parse_hex_u32(asset.c_str());
        }
        else if (is_digits(asset.c_str())) {
            file_hash = parse_digits_u32(asset.c_str());
        }
        else {
            file_hash = hash_string(std::filesystem::path(asset));
        }

        if (m_extract_raw) {
            raw_export(app_state, file_hash);
        }
        else {
            normal_export(app_state, file_hash);
        }
    }

    AssetDB::set_instance(nullptr);
}
