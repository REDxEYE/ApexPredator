//
// Created by red_eye on 10/9/26.
//

#pragma once
#include "apex/rbmdl/rbmdl_file.hpp"
#include "apex/rbmdl/render_block.hpp"
#include "redscore/platform/file/file.h"
#include "redscore/platform/texture/texture_ops.h"

namespace Apex::RBMdl {
#pragma pack(push, 1)

    struct GeneralMaterial {
        glm::vec4 channel_texture_mask{}; // +0x00: RGBA dot weights for the base texture when flags & 0x20
        glm::vec3 channel_ao_mask{}; // +0x10: RGB dot weights for the secondary texture
        float channel_ao_grayscale{}; // +0x1C: select grayscale secondary texture when > 0.5
        float depth_offset{}; // +0x20: clip-space z offset per clip-space w
        float specular_power{}; // +0x24: specular exponent
        uint32_t vertex_format{}; // +0x28: 0 => 40 bytes; 1 => 28 bytes
        float position_scale{}; // +0x2C: uniform scale for packed positions
        glm::vec2 primary_uv_scale{}; // +0x30: primary UV X/Y scale
        glm::vec2 secondary_uv_scale{}; // +0x38: secondary UV X/Y scale
        float unknown_parameter{}; // +0x40: purpose unverified
        uint8_t packed_parameters[4]{}; // +0x44: four bytes, purpose unverified
        uint32_t flags{}; // +0x48

        static GeneralMaterial read_v2(IO::File &file);

        static GeneralMaterial read_v3(IO::File &file);
    };

    static_assert(sizeof(GeneralMaterial) == 76);

    struct GeneralVertex28 {
        int16_t texcoord0[4]; // DXGI_FORMAT_R16G16B16A16_SNORM
        float texcoord1[3]; // DXGI_FORMAT_R32G32B32_FLOAT
        int16_t position[4]; // DXGI_FORMAT_R16G16B16A16_SNORM
    };

    static_assert(sizeof(GeneralVertex28) == 28);

    struct GeneralVertex40 {
        float position[3]; // DXGI_FORMAT_R32G32B32_FLOAT
        float texcoord0[4]; // DXGI_FORMAT_R32G32B32A32_FLOAT
        float texcoord1[3]; // DXGI_FORMAT_R32G32B32_FLOAT
    };

    static_assert(sizeof(GeneralVertex40) == 40);


    class GeneralRenderBlock : public RenderBlock {
public:
    static constexpr uint32 block_type_hash = 0xA7583B2B;

    explicit GeneralRenderBlock(IO::File &file);;

    [[nodiscard]] IO::BufferView<const GeneralVertex28> vertices28() const {
        if (m_material.vertex_format == 0)
            return {};
        return m_vertex_data.readonly_view_as<GeneralVertex28>();
    }

    [[nodiscard]] IO::BufferView<const GeneralVertex40> vertices40() const {
        if (m_material.vertex_format != 0)
            return {};
        return m_vertex_data.readonly_view_as<GeneralVertex40>();
    }

    void to_primitive(ApexArchiveManager &manager, VM::SceneBuilder &builder, VM::Primitive &primitive) override;

private:
    GeneralMaterial m_material;
    std::string m_textures[8];
    IO::Buffer m_vertex_data;
    std::vector<uint16> m_indices;
};

#pragma pack(pop)
}
