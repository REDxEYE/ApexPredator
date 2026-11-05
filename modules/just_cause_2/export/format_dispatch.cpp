#include "exporter/format_dispatch.h"

#include "apex/pcbb/pcbb.hpp"
#include "apex/rbmdl/rbmdl_file.hpp"
#include "exporter/pcbb_export.h"

#include <cstring>
#include <utility>

std::optional<VM::NodePtr> dispatch_file(ApexAppState &app_state, const uint64 hash,
                                         std::unique_ptr<IO::File> &buffer, std::string_view) {
    const auto &bytes = buffer->cbuffer();
    if (bytes.size() >= 8 && std::memcmp(bytes.data() + 4, RBMDL_MAGIC, 4) == 0)
        return Apex::export_rbmdl(app_state, hash, std::move(buffer));
    if (bytes.size() >= 4 && std::memcmp(bytes.data(), PCBB_MAGIC, 4) == 0)
        return Apex::export_pcbb(app_state, hash, std::move(buffer));
    return std::nullopt;
}
