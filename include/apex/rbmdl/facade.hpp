#pragma once

#include "apex/rbmdl/render_block.hpp"
#include "redscore/platform/file/file.h"

namespace Apex::RBMdl {
#pragma pack(push, 1)
    struct FacadeMaterial {
        glm::vec4 base_channel_mask;       // Base RGBA dot weights when flags & 0x20
        glm::vec3 secondary_channel_mask;  // Slot-3 RGB dot weights when flags & 0x10
        glm::vec3 lighting_color_target;
        float depth_offset;
        float specular_power;
        uint32 vertex_format;              // 0: float position, 1: SNORM16 position
        float position_scale;
        uint32 flags;
    };
    static_assert(sizeof(FacadeMaterial) == 60);

    struct FacadeVertex44 {
        float position[3];
        float texcoord0[4]; // Two UV pairs
        float texcoord1[4]; // Packed normal, tangent, color, scene lookup
    };
    static_assert(sizeof(FacadeVertex44) == 44);

    struct FacadeVertex40 {
        int16 position[4];
        float texcoord0[4];
        float texcoord1[4];
    };
    static_assert(sizeof(FacadeVertex40) == 40);
#pragma pack(pop)

    class FacadeRenderBlock : public RenderBlock {
    public:
        static constexpr uint32 block_type_hash = 0xCE39D7BF;

        explicit FacadeRenderBlock(IO::File &file);

        void to_primitive(ApexArchiveManager &manager, VM::SceneBuilder &builder, VM::Primitive &primitive) override;

    private:
        FacadeMaterial m_material{};
        std::string m_textures[8];
        IO::Buffer m_vertex_data;
        std::vector<uint16> m_indices;
    };
}
