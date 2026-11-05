#include "exporter/gltf_export.h"

#include "apex/hashes.h"
#include "exporter/common_export.h"
#include "platform/app_state.h"
#include "redscore/platform/logger.h"
#include "tracy/Tracy.hpp"

#include <filesystem>
#include <format>
#include <optional>
#include <stdexcept>

void normal_export(ApexAppState &app_state, const std::uint64_t asset_hash) {
    ZoneScoped
    const auto asset_path = find_asset_name(asset_hash)
            .or_else([&] { return std::optional{std::format("{:08X}.bin", asset_hash)}; })
            .value();

    std::filesystem::path save_path = app_state.export_path() / asset_path;
    save_path.replace_extension("gltf");

    auto &builder = app_state.models();
    builder.reset();
    const auto node = export_file(app_state, asset_hash);
    if (node) builder.add_to_scene(node);
    if (!builder.scene.roots.empty()) {
        std::filesystem::create_directories(save_path.parent_path());
        if (VM::save_gltf(builder.scene, save_path)) {
            GLog_Info("Written GLTF file: {}", save_path.string());
        } else {
            throw std::runtime_error("Failed to write GLTF file: " + save_path.string());
        }
    }
}
