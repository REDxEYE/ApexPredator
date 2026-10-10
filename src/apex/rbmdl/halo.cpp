//
// Created by red_eye on 10/10/26.
//

#include "apex/rbmdl/halo.hpp"

#include "redscore/platform/texture/texture_ops.h"

using namespace Apex::RBMdl;

HaloRenderBlock::HaloRenderBlock(IO::File &file) {
    if (const uint8 version = file.read_u8(); version != 0) {
        throw std::runtime_error(std::format("Unsupported version: {}", version));
    }
    for (auto &texture: m_textures) {
        texture = read_rstring(file);
    }
    if (const uint32 mode = file.read_u32(); mode != 3) {
        throw std::runtime_error(std::format("Unsupported mode: {}", mode));
    }

    const uint32 vertex_count = file.read_u32();
    m_vertex_data.resize(vertex_count * 20);
    file.read(m_vertex_data.data(), vertex_count * 20);
    const uint32 index_count = file.read_u32();
    m_indices.resize(index_count);
    file.read(m_indices.data(), index_count * 2);
}

void HaloRenderBlock::to_primitive(ApexArchiveManager &manager, VM::SceneBuilder &builder, VM::Primitive &primitive) {
    primitive.index_count = m_indices.size();
    primitive.index_type = VM::IndexType::U16;
    primitive.indices = VM::data_buffer_from_<uint16>(m_indices);
    const auto &vertices = this->vertices();
    const uint32 vertex_count = vertices.size();
    primitive.attributes.emplace_back(VM::ElementUsage::Position, 0, "", VM::ElementFormat::F32,
                                      VM::ElementType::Vec3, false, vertex_count,
                                      from_packed(vertices, [](const HaloVertex &v) {
                                          return glm::vec3(v.position[0], v.position[1], v.position[2]);
                                      }));
    primitive.attributes.emplace_back(VM::ElementUsage::Color, 0, "", VM::ElementFormat::U8,
                                      VM::ElementType::Vec4, true, vertex_count,
                                      from_packed(vertices, [](const HaloVertex &v) {
                                          return glm::u8vec4(v.color[0], v.color[1], v.color[2], v.color[3]);
                                      }));

    auto material = std::make_shared<VM::Material>(m_textures[0]+"_HALO");
    material->albedo = VM::png_texture(m_textures[0], dds_to_png(manager, m_textures[0]));
    if (!m_textures[1].empty())
        material->normal = VM::png_texture(m_textures[1], dds_to_png(manager, m_textures[1]));

    if (!m_textures[2].empty()) {
        material->metallic_roughness = VM::lazy_png_texture(m_textures[2], [&manager, texture_name=m_textures[2]] {
            const auto &texture = load_dds(manager, texture_name);
            TextureOps::swap_channels(texture.get(), 1, 2);
            TextureOps::fill_channel(texture.get(), 1, 0.7f);
            return texture->save_to_memory(MemoryFormat::PNG);
        });
    }
    primitive.material = material;
}
