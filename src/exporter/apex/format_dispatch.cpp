#include "exporter/format_dispatch.h"

#include "apex/adf/adf.h"
#include "apex/avtx.h"
#include "apex/hashes.h"
#include "apex/rtpc.h"
#include "exporter/adf_export.h"
#include "exporter/ddsc_export.h"
#include "exporter/fmod_export.h"
#include "exporter/havok_export.h"
#include "exporter/rtpc_export.h"
#include "havok/legacy/sdk2014/packfile.hpp"
#include "redscore/platform/logger.h"
#include "redscore/utils/simple_fileio.h"

#include <cstring>
#include <filesystem>
#include <utility>

namespace {
constexpr char mkv_magic[] = "\x1A\x45\xDF\xA3";
}

std::optional<VM::NodePtr> dispatch_file(ApexAppState &app_state, const uint64 hash,
                                         std::unique_ptr<IO::File> &buffer, const std::string_view path) {
    const auto &bytes = buffer->cbuffer();
    if (bytes.size() >= 4) {
        if (std::memcmp(bytes.data(), ADF_MAGIC, 4) == 0 ||
            std::memcmp(bytes.data(), ADF_SMALL_MAGIC, 4) == 0) {
            GLog_Info("Detected ADF file");
            return export_adf_file_from_buffer(app_state, hash, std::move(buffer));
        }
        if (std::memcmp(bytes.data(), AVTX_MAGIC, 4) == 0) {
            GLog_Info("Detected AVTX file");
            export_ddsc(app_state, hash, std::move(buffer));
            return VM::NodePtr{};
        }
        if (std::memcmp(bytes.data(), RIFF_MAGIC, 4) == 0) {
            GLog_Info("Detected FMOD bank/RIFF file");
            export_fmod_bank(app_state, hash, std::move(buffer));
            return VM::NodePtr{};
        }
        if (std::memcmp(bytes.data(), RTPC_MAGIC, 4) == 0) {
            GLog_Info("Detected RTPC file");
            return export_rtpc(app_state, std::move(buffer), hash);
        }
    }
    if (bytes.size() >= sizeof(HAVOK2014_MAGIC) &&
        std::memcmp(bytes.data(), HAVOK2014_MAGIC, sizeof(HAVOK2014_MAGIC)) == 0) {
        GLog_Info("Detected Havok file");
        return export_havok_file(app_state, std::move(buffer), path);
    }
    if (bytes.size() >= 8 && std::memcmp(bytes.data() + 4, HAVOK_MAGIC, 4) == 0) {
        GLog_Info("Detected Havok file");
        return export_havok_file(app_state, std::move(buffer), path);
    }
    if (bytes.size() >= 4 && std::memcmp(bytes.data(), mkv_magic, 4) == 0) {
        GLog_Info("Detected MKV file");
        const auto export_path = get_export_path(app_state.export_path(), hash, ".mkv");
        std::filesystem::create_directories(export_path.parent_path());
        try {
            write_file(export_path, bytes);
            GLog_Info("MKV file \"{}\" been written to file: \"{}\"", path, export_path.string());
        } catch (const std::exception &error) {
            GLog_Error("Failed to write MKV file \"{}\" to path: \"{}\". Error: {}",
                       path, export_path.string(), error.what());
        }
        return VM::NodePtr{};
    }
    return std::nullopt;
}
