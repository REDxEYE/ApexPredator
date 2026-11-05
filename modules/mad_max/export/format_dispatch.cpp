#include "exporter/format_dispatch.h"

#include "exporter/havok_export.h"
#include "havok/legacy/sdk2014/packfile.hpp"
#include "redscore/platform/logger.h"

#include <cstring>
#include <utility>

std::optional<VM::NodePtr> dispatch_file(ApexAppState &app_state, const uint64,
                                         std::unique_ptr<IO::File> &buffer, const std::string_view path) {
    const auto &bytes = buffer->cbuffer();
    if (bytes.size() >= sizeof(HAVOK2014_MAGIC) &&
        std::memcmp(bytes.data(), HAVOK2014_MAGIC, sizeof(HAVOK2014_MAGIC)) == 0) {
        GLog_Info("Detected Havok file");
        return export_havok_file(app_state, std::move(buffer), path);
    }
    return std::nullopt;
}
