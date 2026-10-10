//
// Created by red_eye on 10/9/26.
//

#include "apex/rbmdl/general.hpp"

#include <glm/glm.hpp>

#include "apex/rbmdl/common.hpp"


Apex::RBMdl::GeneralMaterial Apex::RBMdl::GeneralMaterial::read_v2(IO::File &file) {
    auto material = (GeneralMaterial){
        .channel_texture_mask = {file.read_f32(), file.read_f32(), file.read_f32(), file.read_f32()},
        .channel_ao_mask = {file.read_f32(), file.read_f32(), file.read_f32()},
        .channel_ao_grayscale = file.read_f32(),
        .depth_offset = file.read_f32(),
        .specular_power = file.read_f32(),
        .vertex_format = file.read_u32(),
        .position_scale = file.read_f32(),
        .primary_uv_scale = {file.read_f32(), file.read_f32()},
        .secondary_uv_scale = {0, 0},
        .unknown_parameter = file.read_f32(),
        .packed_parameters = {file.read_u8(), file.read_u8(), file.read_u8(), file.read_u8()},
        .flags = file.read_u32(),
    };
    material.secondary_uv_scale = material.primary_uv_scale;
    return material;
}

Apex::RBMdl::GeneralMaterial Apex::RBMdl::GeneralMaterial::read_v3(IO::File &file) {
    return file.read_pod<GeneralMaterial>();
}

Apex::RBMdl::GeneralRenderBlock::GeneralRenderBlock(IO::File &file) {
    const uint8 version = file.read_u8();
    if (version != 2 && version != 3) {
        throw std::runtime_error(std::format("Unsupported version: {}", version));
    }
    m_material = version == 2 ? GeneralMaterial::read_v2(file) : GeneralMaterial::read_v3(file);
    for (auto &texture: m_textures) {
        texture = read_rstring(file);
    }
    if (const uint32 mode = file.read_u32(); mode != 3) {
        throw std::runtime_error(std::format("Unsupported mode: {}", mode));
    }
    const uint32 vertex_count = file.read_u32();
    if (m_material.vertex_format == 0) {
        m_vertex_data.resize(vertex_count * 40);
        file.read(m_vertex_data.data(), vertex_count * 40);
    } else {
        m_vertex_data.resize(vertex_count * 28);
        file.read(m_vertex_data.data(), vertex_count * 28);
    }
    const uint32 index_count = file.read_u32();
    m_indices.resize(index_count);
    file.read(m_indices.data(), index_count * 2);
}

void Apex::RBMdl::GeneralRenderBlock::to_primitive(ApexArchiveManager &manager, VM::SceneBuilder &builder,
                                                   VM::Primitive &primitive) {
    primitive.index_count = m_indices.size();
    primitive.index_type = VM::IndexType::U16;
    primitive.indices = VM::data_buffer_from_<uint16>(m_indices);
    const auto material = std::make_shared<VM::Material>(m_textures[0]);
    if (m_material.vertex_format == 1) {
        const uint32 vertex_count = m_vertex_data.size() / sizeof(GeneralVertex28);
        const auto vertices = vertices28();
        primitive.attributes.emplace_back(VM::ElementUsage::Position, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec3, false, vertex_count,
                                          from_packed(vertices, [this](const GeneralVertex28 &v) {
                                              const float32 max = std::numeric_limits<int16>::max();
                                              return glm::vec3(v.position[0] / max, v.position[1] / max,
                                                               v.position[2] / max) *
                                                     this->m_material.position_scale;
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::TexCoord, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec2, false, vertex_count,
                                          from_packed(vertices, [this](const GeneralVertex28 &v) {
                                              return glm::vec2(
                                                  snorm16(v.texcoord0[0]) * this->m_material.primary_uv_scale[0],
                                                  snorm16(v.texcoord0[1]) * this->m_material.primary_uv_scale[1]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::TexCoord, 1, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec2, false, vertex_count,
                                          from_packed(vertices, [this](const GeneralVertex28 &v) {
                                              return glm::vec2(
                                                  snorm16(v.texcoord0[2]) * this->m_material.secondary_uv_scale[0],
                                                  snorm16(v.texcoord0[3]) * this->m_material.secondary_uv_scale[1]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::Normal, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec3, false, vertex_count,
                                          from_packed(vertices, [](const GeneralVertex28 &v) {
                                              return unpack_direction(v.texcoord1[0]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::Tangent, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec4, false, vertex_count,
                                          from_packed(vertices, [](const GeneralVertex28 &v) {
                                              return unpack_tangent(v.texcoord1[1]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::Color, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec3, false, vertex_count,
                                          from_packed(vertices, [](const GeneralVertex28 &v) {
                                              return unpack_color(v.texcoord1[2]);
                                          }));
    } else {
        const uint32 vertex_count = m_vertex_data.size() / sizeof(GeneralVertex40);
        const auto vertices = vertices40();
        primitive.attributes.emplace_back(VM::ElementUsage::Position, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec3, false, vertex_count,
                                          from_packed(vertices, [](const GeneralVertex40 &v) {
                                              return glm::vec3(v.position[0], v.position[1], v.position[2]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::TexCoord, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec2, false, vertex_count,
                                          from_packed(vertices, [](const GeneralVertex40 &v) {
                                              return glm::vec2(v.texcoord0[0], v.texcoord0[1]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::TexCoord, 1, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec2, false, vertex_count,
                                          from_packed(vertices, [](const GeneralVertex40 &v) {
                                              return glm::vec2(v.texcoord0[2], v.texcoord0[3]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::Normal, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec3, false, vertex_count,
                                          from_packed(vertices, [](const GeneralVertex40 &v) {
                                              return unpack_direction(v.texcoord1[0]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::Tangent, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec4, false, vertex_count,
                                          from_packed(vertices, [](const GeneralVertex40 &v) {
                                              return unpack_tangent(v.texcoord1[1]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::Color, 0, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec3, false, vertex_count,
                                          from_packed(vertices, [](const GeneralVertex40 &v) {
                                              return unpack_color(v.texcoord1[2]);
                                          }));
    }
    apply_depth_offset(primitive, m_material.depth_offset, m_material.position_scale);
    if (!m_textures[0].empty()) {
        if (m_material.flags & 0x20) {
            const auto weights = m_material.channel_texture_mask;
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
    material->normal = VM::png_texture(m_textures[1], dds_to_png(manager, m_textures[1]));

    material->metallic_roughness = VM::lazy_png_texture(m_textures[2], [&manager, texture_name=m_textures[2]] {
        const auto &texture = load_dds(manager, texture_name);
        TextureOps::swap_channels(texture.get(), 1, 2);
        TextureOps::fill_channel(texture.get(), 1, 0.7f);
        return texture->save_to_memory(MemoryFormat::PNG);
    });

    primitive.material = material;
    if (m_material.flags & 0x1) {
        material->double_sided = true;
    }
    if (m_material.flags & 0x2) {
        material->alpha_mode = ISR::AlphaMode::Blend;
    }
}
