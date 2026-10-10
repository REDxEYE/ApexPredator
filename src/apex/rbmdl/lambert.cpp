//
// Created by red_eye on 10/10/26.
//

#include "apex/rbmdl/lambert.hpp"

Apex::RBMdl::LambertMaterialV4 Apex::RBMdl::LambertMaterialV4::read_v0(IO::File &file) {
    LambertMaterialV4 mat{};
    mat.flags = file.read_u32() | 0x40;
    return mat;
}

Apex::RBMdl::LambertMaterialV4 Apex::RBMdl::LambertMaterialV4::read_v2(IO::File &file) {
    LambertMaterialV4 mat{};
    mat.flags = file.read_u32();
    mat.depth_offset = file.read_f32();
    return mat;
}

Apex::RBMdl::LambertMaterialV4 Apex::RBMdl::LambertMaterialV4::read_v3(IO::File &file) {
    LambertMaterialV4 mat{};
    mat.flags = file.read_u32();
    mat.depth_offset = file.read_f32();
    mat.vertex_format = file.read_u32();
    mat.position_scale = file.read_f32();
    mat.uv_scale = {file.read_f32(), file.read_f32(), file.read_f32(), file.read_f32()};
    mat.unknown_float = file.read_f32();
    for (auto &byte: mat.packed)
        byte = file.read_u8();
    return mat;
}

Apex::RBMdl::LambertMaterialV4 Apex::RBMdl::LambertMaterialV4::read_v4(IO::File &file) {
    return file.read_pod<LambertMaterialV4>();
}

Apex::RBMdl::LambertRenderBlock::LambertRenderBlock(IO::File &file) {
    switch (file.read_u8()) {
        case(0):
            m_material = LambertMaterialV4::read_v0(file);
            break;
        case 2:
            m_material = LambertMaterialV4::read_v2(file);
            break;
        case 3:
            m_material = LambertMaterialV4::read_v3(file);
            break;
        case 4:
            m_material = LambertMaterialV4::read_v4(file);
            break;
        default:
            throw std::runtime_error("Unknown LambertMaterial version");
    }
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

void Apex::RBMdl::LambertRenderBlock::to_primitive(ApexArchiveManager &manager, VM::SceneBuilder &builder,
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
                                                  snorm16(v.texcoord0[0]) * this->m_material.uv_scale[0],
                                                  snorm16(v.texcoord0[1]) * this->m_material.uv_scale[1]);
                                          }));
        primitive.attributes.emplace_back(VM::ElementUsage::TexCoord, 1, "", VM::ElementFormat::F32,
                                          VM::ElementType::Vec2, false, vertex_count,
                                          from_packed(vertices, [this](const GeneralVertex28 &v) {
                                              return glm::vec2(
                                                  snorm16(v.texcoord0[2]) * this->m_material.uv_scale[2],
                                                  snorm16(v.texcoord0[3]) * this->m_material.uv_scale[3]);
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
    material->albedo = VM::png_texture(m_textures[0], dds_to_png(manager, m_textures[0]));
    material->normal = VM::png_texture(m_textures[1], dds_to_png(manager, m_textures[1]));
    VM::png_texture(m_textures[1], dds_to_png(manager, m_textures[1]));

    if (m_material.flags&0x01) {
        material->alpha_mode = ISR::AlphaMode::Blend;
    }
    if (m_material.flags & 0x02) {
        material->alpha_mode = ISR::AlphaMode::Mask;
        material->alpha_cutoff = 0.5f;
    }

    if (m_material.flags & 0x04) {
        material->double_sided = true;
    }
}
