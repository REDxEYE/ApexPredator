// Created by RED on 12.01.2026.

#include "exporter/ddsc_export.h"

#include "apex/avtx.h"
#include "apex/hashes.h"
#include "redscore/platform/logger.h"
#include "redscore/platform/texture/texture.h"

#include "tracy/Tracy.hpp"

std::unique_ptr<Texture> convert_ddsc(ApexAppState &app_state, const uint32 hash) {
    ZoneScoped
    auto mb = app_state.manager().get(hash);
    if (!mb) {
        GLog_Error("File not found");
        return {};
    }
    return AVTX::from_buffer(std::move(mb), hash, app_state.manager());
}

void export_ddsc(ApexAppState &app_state, const uint32 hash, std::unique_ptr<IO::File> &&mb) {
    ZoneScoped
    const auto tex = AVTX::from_buffer(std::move(mb), hash, app_state.manager());
    if (!tex) {
        GLog_Error("Failed to convert AVTX texture with hash {:08X}", hash);
        return;
    }
    const auto export_path = app_state.export_path() / find_name(hash).value_or(std::format("texture_{:08X}", hash));
    std::filesystem::create_directories(export_path.parent_path());
    tex->save(export_path);
}
