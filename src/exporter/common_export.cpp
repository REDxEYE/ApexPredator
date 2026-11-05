// Created by RED on 12.01.2026.

#include "exporter/common_export.h"

#include "apex/hashes.h"
#include "exporter/format_dispatch.h"
#include "redscore/platform/logger.h"
#include "redscore/utils/simple_fileio.h"
#include "tracy/Tracy.hpp"

#include <exception>
#include <filesystem>
#include <format>
#include <utility>

VM::NodePtr export_file(ApexAppState &app_state, const uint64 hash) {
    const auto path = find_asset_name(hash).value_or(std::format("unnamed/unknown_{:08X}", hash));
    return export_file(app_state, hash, app_state.manager().get(hash), path);
}

VM::NodePtr export_file(ApexAppState &app_state, const uint64 hash, std::unique_ptr<IO::File>&& buffer,
                        const std::string_view path) {
    ZoneScoped
    GLog_Info("Exporting file: {}", path);
    if (!buffer) {
        GLog_Error("File \"{}\" not found", path);
        return {};
    }
    const auto &mb = buffer->cbuffer();

    if (auto result = dispatch_file(app_state, hash, buffer, path)) return std::move(*result);
    const std::filesystem::path &unk_file_export_path = get_export_path(app_state.export_path(), hash, ".bin");
    std::filesystem::create_directories(unk_file_export_path.parent_path());
    try {
        write_file(unk_file_export_path, mb);
        GLog_Info("Unknown file \"{}\" has been written to file: \"{}\"", path, unk_file_export_path.string());
    } catch (const std::exception &e) {
        GLog_Error("Failed to write unknown file \"{}\" to path: \"{}\". Error: {}", path,
                   unk_file_export_path.string(), e.what());
    }
    return {};
}
