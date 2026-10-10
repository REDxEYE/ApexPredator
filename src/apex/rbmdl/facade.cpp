#include "apex/rbmdl/facade.hpp"

#include <format>
#include <stdexcept>

#include "apex/rbmdl/common.hpp"
#include "redscore/platform/texture/texture_ops.h"

namespace {
    template<typename Vertex>
    void append_surface_attributes(IO::BufferView<const Vertex> vertices, VM::Primitive &primitive) {
        const auto count = vertices.size();
        primitive.attributes.emplace_back(VM::ElementUsage::TexCoord, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec2, false, count,
                                          from_packed(vertices, [](const Vertex &v) {
                                              return glm::vec2(v.texcoord0[0], v.texcoord0[1]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::TexCoord, 1, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec2, false, count,
                                          from_packed(vertices, [](const Vertex &v) {
                                              return glm::vec2(v.texcoord0[2], v.texcoord0[3]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::Normal, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec3, false, count,
                                          from_packed(vertices, [](const Vertex &v) {
                                              return unpack_direction(v.texcoord1[0]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::Tangent, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec4, false, count,
                                          from_packed(vertices, [](const Vertex &v) {
                                              return unpack_tangent(v.texcoord1[1]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::Color, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec3, false, count,
                                          from_packed(vertices, [](const Vertex &v) {
                                              return unpack_color(v.texcoord1[2]);
                                          }));
        // texcoord1[3] encodes a lookup into a renderer-owned scene texture, not a mesh UV.
    }
}

Apex::RBMdl::FacadeRenderBlock::FacadeRenderBlock(IO::File &file) {
    const uint8 version = file.read_u8();
    if (version != 1)
        throw std::runtime_error(std::format("Unsupported Facade version: {}", version));

    m_material = file.read_pod<FacadeMaterial>();
    if (m_material.vertex_format > 1)
        throw std::runtime_error(std::format("Unsupported Facade vertex format: {}", m_material.vertex_format));

    for (auto &texture : m_textures)
        texture = read_rstring(file);

    if (const uint32 mode = file.read_u32(); mode != 3)
        throw std::runtime_error(std::format("Unsupported Facade mode: {}", mode));

    const uint32 vertex_count = file.read_u32();
    const size_t stride = m_material.vertex_format == 0 ? sizeof(FacadeVertex44) : sizeof(FacadeVertex40);
    m_vertex_data.resize(static_cast<size_t>(vertex_count) * stride);
    file.read(m_vertex_data.data(), m_vertex_data.size());

    const uint32 index_count = file.read_u32();
    m_indices.resize(index_count);
    file.read(m_indices.data(), m_indices.size() * sizeof(uint16));
}

void Apex::RBMdl::FacadeRenderBlock::to_primitive(ApexArchiveManager &manager, VM::SceneBuilder &,
                                                  VM::Primitive &primitive) {
    primitive.index_count = m_indices.size();
    primitive.index_type = VM::IndexType::U16;
    primitive.indices = VM::data_buffer_from_<uint16>(m_indices);

    if (m_material.vertex_format == 0) {
        const auto vertices = m_vertex_data.readonly_view_as<FacadeVertex44>();
        primitive.attributes.emplace_back(VM::ElementUsage::Position, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec3, false, vertices.size(),
                                          from_packed(vertices, [](const FacadeVertex44 &v) {
                                              return glm::vec3(v.position[0], v.position[1], v.position[2]);
                                          }));
        append_surface_attributes(vertices, primitive);
    } else {
        const auto vertices = m_vertex_data.readonly_view_as<FacadeVertex40>();
        const float scale = m_material.position_scale;
        primitive.attributes.emplace_back(VM::ElementUsage::Position, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec3, false, vertices.size(),
                                          from_packed(vertices, [scale](const FacadeVertex40 &v) {
                                              return glm::vec3(snorm16(v.position[0]), snorm16(v.position[1]),
                                                               snorm16(v.position[2])) * scale;
                                          }));
        append_surface_attributes(vertices, primitive);
    }
    apply_depth_offset(primitive, m_material.depth_offset, m_material.position_scale);

    const auto material = std::make_shared<VM::Material>(m_textures[0]);
    if (!m_textures[0].empty()) {
        if (m_material.flags & 0x20) {
            const auto weights = m_material.base_channel_mask;
            material->albedo = VM::lazy_png_texture(m_textures[0], [&manager, name = m_textures[0], weights] {
                const auto texture = load_dds(manager, name);
                if (!texture)
                    return VM::DataBuffer{};
                apply_channel_texture_mask(*texture, weights);
                return texture->save_to_memory(MemoryFormat::PNG);
            });
        } else {
            material->albedo = VM::png_texture(m_textures[0], dds_to_png(manager, m_textures[0]));
        }
    }
    if (!m_textures[1].empty())
        material->normal = VM::png_texture(m_textures[1], dds_to_png(manager, m_textures[1]));
    if (!m_textures[2].empty()) {
        // Approximate the surface map with the same glTF channel mapping as General.
        material->metallic_roughness = VM::lazy_png_texture(m_textures[2], [&manager, name = m_textures[2]] {
            const auto texture = load_dds(manager, name);
            if (!texture)
                return VM::DataBuffer{};
            TextureOps::swap_channels(texture.get(), 1, 2);
            TextureOps::fill_channel(texture.get(), 1, 0.7f);
            return texture->save_to_memory(MemoryFormat::PNG);
        });
    }
    material->double_sided = (m_material.flags & 0x1) != 0;
    primitive.material = material;
}
