// Created by RED on 12.01.2026.

#include "exporter/common_export.h"

#include "apex/avtx.h"
#include "apex/hashes.h"
#include "apex/rtpc.h"
#include "apex/adf/adf.h"
#include "exporter/adf_export.h"

#include "redscore/platform/logger.h"
#include "redscore/utils/simple_fileio.h"

#include "tiny_gltf.h"
#include "exporter/ddsc_export.h"
#include "exporter/fmod_export.h"
#include "exporter/havok_export.h"
#include "exporter/rtpc_export.h"
#include "tracy/Tracy.hpp"

#define MVK_MAGIC "\x1A\x45\xDF\xA3"


GltfHelper::Handle<tinygltf::Node> export_file(AppState &app_state, const uint32 hash) {
    ZoneScoped
    auto& manager = app_state.manager();
    auto path = find_name(hash).value_or(std::format("unknown({:08X})", hash));
    GLog_Info("Exporting file: {}", path);
    auto buffer = manager.get_file(hash);
    if (!buffer) {
        GLog_Error("File \"{}\" not found", path);
        return {};
    }
    const auto &mb = buffer->cbuffer();
    if (memcmp(mb.data(), ADF_MAGIC, 4) == 0) {
        return export_adf_file_from_buffer(app_state, hash, std::move(buffer));
    }
    if (memcmp(mb.data(), AVTX_MAGIC, 4) == 0) {
        export_ddsc(app_state, hash, std::move(buffer));
        return {};
    }
    if (memcmp(mb.data(), RIFF_MAGIC, 4) == 0) {
        export_fmod_bank(app_state, hash, std::move(buffer));
        return {};
    }
    if (memcmp(mb.data(), RTPC_MAGIC, 4) == 0) {
        return export_rtpc(app_state, std::move(buffer), hash);
    }
    if (memcmp(mb.data() + 4, HAVOK_MAGIC, 4) == 0) {
        return export_havok_file(app_state, std::move(buffer), path);
    }
    if (memcmp(mb.data(), MVK_MAGIC, 4) == 0) {
        const std::filesystem::path export_path = get_export_path(app_state.export_path(), hash, ".mkv");

        std::filesystem::create_directories(export_path.parent_path());

        try {
            write_file(export_path, mb);
            GLog_Info("MKV file \"{}\" been written to file: \"{}\"", path, export_path.string());
        } catch (const std::exception &e) {
            // format exception to log
            GLog_Error("Failed to write MKV file \"{}\" to path: \"{}\". Error: {}", path, export_path.string(),
                       e.what());
        }
    }
    else {
        const std::filesystem::path &unk_file_export_path = get_export_path(app_state.export_path(), hash, ".bin");
        std::filesystem::create_directories(unk_file_export_path.parent_path());
        try {
            write_file(unk_file_export_path, mb);
            GLog_Info("Unknown file \"{}\" has been written to file: \"{}\"", path, unk_file_export_path.string());
        } catch (const std::exception &e) {
            GLog_Error("Failed to write unknown file \"{}\" to path: \"{}\". Error: {}", path,
                       unk_file_export_path.string(), e.what());
        }
    }
    return {};
}
