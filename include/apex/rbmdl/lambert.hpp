//
// Created by red_eye on 10/10/26.
//

#pragma once
#include "general.hpp"
#include "apex/rbmdl/common.hpp"
#include "apex/rbmdl/rbmdl_file.hpp"
#include "apex/rbmdl/render_block.hpp"
#include "redscore/platform/file/file.h"

namespace Apex::RBMdl {
    struct LambertMaterialV4 {
        uint32_t vertex_format = 0; // +0x00: 0 => 40-byte, 1 => 28-byte vertices
        float position_scale = 1.0f; // +0x04: uniform packed-position scale
        glm::vec4 uv_scale{1.0f, 1.0f, 1.0f, 1.0f}; // +0x08: primary XY, secondary XY
        float unknown_float = 1.0f; // +0x18
        uint8_t packed[4]{0xFF, 0xFF, 0xFF, 0xFF}; // +0x1C: purpose unknown; not endian-swapped
        uint32_t flags = 0x40; // +0x20
        float depth_offset = 0.0f; // +0x24: clip-space z bias per clip-space w
        uint8_t channel_selectors[2]{}; // +0x28: base/secondary channel selectors
        uint8_t trailing_unknown[2]{}; // +0x2A: not endian-swapped

        static LambertMaterialV4 read_v0(IO::File &file);

        static LambertMaterialV4 read_v2(IO::File &file);

        static LambertMaterialV4 read_v3(IO::File &file);

        static LambertMaterialV4 read_v4(IO::File &file);
    };

    static_assert(sizeof(LambertMaterialV4) == 44);

    class LambertRenderBlock : public RenderBlock {
    public:
        static constexpr uint32 block_type_hash = 0xD5D78AE0;

        explicit LambertRenderBlock(IO::File &file);

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
        LambertMaterialV4 m_material;
        std::string m_textures[8];
        IO::Buffer m_vertex_data;
        std::vector<uint16> m_indices;
    };
}
