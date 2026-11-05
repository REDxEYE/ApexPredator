#include "exporter/raw_export.h"

#include "apex/hashes.h"
#include "platform/app_state.h"
#include "redscore/platform/logger.h"
#include "redscore/utils/simple_fileio.h"
#include "tracy/Tracy.hpp"

#include <filesystem>
#include <format>
#include <optional>
#include <stdexcept>

void raw_export(ApexAppState &app_state, const std::uint64_t asset_hash) {
    ZoneScoped
    const auto asset_path = find_asset_name(asset_hash)
            .or_else([&] { return std::optional{std::format("{:08X}.bin", asset_hash)}; })
            .value();

    const std::filesystem::path save_path = app_state.export_path() / asset_path;
    std::filesystem::create_directories(save_path.parent_path());

    const auto mb = app_state.manager().get(asset_hash);
    if (!mb) {
        GLog_Error("File not found: {}", asset_hash);
        throw std::runtime_error("Asset not found");
    }

    write_file(save_path, mb->cbuffer());
    GLog_Info("File \"{}\" extracted to \"{}\"", asset_hash, save_path.string());
}
