#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "apex/adf/generated/adf_types.h"
#include "redscore/platform/model/model.hpp"

namespace AMF {
    // Callers resolve game-specific vertex buffers and stream offsets first.
    struct AttributeInput {
        const ADFTypes::AmfStreamAttribute &descriptor;
        std::span<const uint8_t> buffer;
        size_t offset;
        size_t stride;
        size_t vertex_count;
        std::span<const int16_t> bone_lookup;
        uint32_t texcoord_set;
        bool color_as_custom = false;
    };

    // Skips unknown usages or invalid buffer data; throws for unsupported formats of known usages.
    std::optional<VM::VertexAttribute> decode_attribute(const AttributeInput &input);
}
