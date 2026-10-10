//
// Created by red_eye on 10/9/26.
//

#include "apex/rbmdl/common.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

std::string read_rstring(IO::File &file) {
    const uint32 length = file.read_u32();
    std::string result(length, '\0');
    file.read_string(length, result);
    return result;
}

float snorm16(const int16_t value) {
    return std::max(-1.f, static_cast<float>(value) / 32767.f);
}

glm::vec3 unpack_direction(const float packed) {
    const glm::vec3 components(packed, packed / 256.f, packed / 65536.f);
    return glm::normalize(2.f * glm::fract(components) - 1.f);
}

glm::vec4 unpack_tangent(const float packed) {
    const float handedness = packed < 0.f ? 1.f : packed > 0.f ? -1.f : 0.f;
    return {unpack_direction(std::abs(packed)), handedness};
}

glm::vec3 unpack_color(const float packed) {
    return glm::fract(glm::vec3(packed, packed / 64.f, packed / 4096.f));
}

VM::DataBuffer dds_to_png(ArchiveManager<u64> &manager, const std::string &name) {
    if (name.empty()) {
        return {};
    }
    const auto &file = manager.get(asset_path_hash(name));
    if (!file) {
        return {};
    }
    return DDS::read_texture(file).save_to_memory(MemoryFormat::PNG);
}

std::shared_ptr<Texture> load_dds(ArchiveManager<u64> &manager, const std::string &name) {
    if (name.empty()) {
        return {};
    }
    const auto &file = manager.get(asset_path_hash(name));
    if (!file) {
        return {};
    }
    return std::make_shared<Texture>(std::move(DDS::read_texture(file)));
}

void apply_channel_texture_mask(Texture &texture, const glm::vec4 &weights) {
    const size_t channels = texture.channel_count();
    if (texture.bpc() != 1 || texture.is_float() || (channels != 3 && channels != 4))
        throw std::runtime_error("Channel-masked base texture requires 8-bit RGB or RGBA pixels");

    int selected_channel = -1;
    for (int channel = 0; channel < 4; ++channel) {
        if (weights[channel] == 0.f)
            continue;
        if (weights[channel] != 1.f || selected_channel != -1) {
            selected_channel = -1;
            break;
        }
        selected_channel = channel;
    }

    auto &pixels = texture.data();
    for (size_t i = 0; i < pixels.size(); i += channels) {
        uint8 value;
        if (selected_channel >= 0) {
            value = static_cast<size_t>(selected_channel) < channels ? pixels[i + selected_channel] : 255;
        } else {
            const float alpha = channels == 4 ? pixels[i + 3] : 255.f;
            const float gray = pixels[i] * weights[0] + pixels[i + 1] * weights[1]
                               + pixels[i + 2] * weights[2] + alpha * weights[3];
            value = static_cast<uint8>(std::lround(std::clamp(gray, 0.f, 255.f)));
        }
        pixels[i] = pixels[i + 1] = pixels[i + 2] = value;
    }
}

void apply_depth_offset(VM::Primitive &primitive, const float depth_offset, const float position_scale) {
    if (depth_offset == 0.f)
        return;

    VM::VertexAttribute *positions = nullptr;
    const VM::VertexAttribute *normals = nullptr;
    for (auto &attribute : primitive.attributes) {
        if (attribute.usage == VM::ElementUsage::Position && attribute.set == 0)
            positions = &attribute;
        if (attribute.usage == VM::ElementUsage::Normal && attribute.set == 0)
            normals = &attribute;
    }
    if (!positions || !normals || positions->count != normals->count ||
        positions->data.size() != positions->count * sizeof(glm::vec3) ||
        normals->data.size() != normals->count * sizeof(glm::vec3))
        throw std::runtime_error("Depth-offset geometry requires matching float3 positions and normals");

    // JC2 adds depth_offset * clip_w to clip z. glTF has no portable depth bias;
    // this small local-normal extrusion separates coplanar decals at ordinary view distances.
    const float distance = depth_offset * std::max(1.f, std::abs(position_scale)) * 25.f;
    for (size_t i = 0; i < positions->count; ++i) {
        glm::vec3 point, normal;
        auto *destination = positions->data.data() + i * sizeof(point);
        std::memcpy(&point, destination, sizeof(point));
        std::memcpy(&normal, normals->data.data() + i * sizeof(normal), sizeof(normal));
        point += normal * distance;
        std::memcpy(destination, &point, sizeof(point));
    }
}
