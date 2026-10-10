//
// Created by red_eye on 10/9/26.
//

#pragma once

#include <glm/glm.hpp>

#include "redscore/platform/archive_manager.h"
#include "redscore/platform/file/file.h"
#include "redscore/platform/texture/readers/dds_reader.hpp"
#include "redscore/platform/model/model.hpp"
#include "utils/hash_helper.h"


std::string read_rstring(IO::File &file);

float snorm16(int16_t value);

glm::vec3 unpack_direction(float packed);

glm::vec4 unpack_tangent(float packed);

glm::vec3 unpack_color(float packed);

VM::DataBuffer dds_to_png(ArchiveManager<u64> &manager, const std::string &name);

std::shared_ptr<Texture> load_dds(ArchiveManager<u64> &manager, const std::string &name);
// Match the General/Facade shader's RGBA dot into grayscale RGB; keep source alpha.
void apply_channel_texture_mask(Texture &texture, const glm::vec4 &weights);
// Bake a view-independent approximation of the game's clip-space depth bias.
void apply_depth_offset(VM::Primitive &primitive, float depth_offset, float position_scale);

template<typename T, typename F>
VM::DataBuffer from_packed(IO::BufferView<const T> data, F &&extractor) {
    using O = std::remove_cvref_t<std::invoke_result_t<F, const T &> >;
    static_assert(std::is_trivially_copyable_v<O>);
    VM::DataBuffer out_buffer(data.size() * sizeof(O));
    auto *destination = out_buffer.data();
    for (const auto &vertex: data) {
        const O decoded = std::invoke(extractor, vertex);
        std::memcpy(destination, &decoded, sizeof(O));
        destination += sizeof(O);
    }
    return out_buffer;
}
