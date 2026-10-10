//
// Created by red_eye on 10/10/26.
//

#pragma once
#include "apex/rbmdl/common.hpp"
#include "apex/rbmdl/rbmdl_file.hpp"
#include "apex/rbmdl/render_block.hpp"
#include "redscore/platform/file/file.h"

namespace Apex::RBMdl {
#pragma pack(push, 1)
    struct HaloVertex {
        float32 position[3];
        uint32 unk;
        uint8 color[4];
    };

    static_assert(sizeof(HaloVertex) == 20);
#pragma pack(pop)


    class HaloRenderBlock : public RenderBlock {
    public:
        static constexpr uint32 block_type_hash = 0x65D9B5B2;

        explicit HaloRenderBlock(IO::File &file);

        [[nodiscard]] IO::BufferView<const HaloVertex> vertices() const {
            return m_vertex_data.readonly_view_as<HaloVertex>();
        }

        void to_primitive(ApexArchiveManager &manager, VM::SceneBuilder &builder, VM::Primitive &primitive) override;

    private:
        std::string m_textures[8];
        IO::Buffer m_vertex_data;
        std::vector<uint16> m_indices;
    };
}