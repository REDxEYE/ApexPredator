//
// Created by red_eye on 10/7/26.
//

#pragma once
#include <glm/glm.hpp>

#include "int_def.h"
#include "apex/rbmdl/render_block.hpp"
#include "platform/app_state.h"
#include "redscore/platform/file/file.h"
#include "redscore/platform/model/model.hpp"

constexpr uint8 RBMDL_MAGIC[] = "RBMDL";

namespace Apex {

    class RBMdlFile {
    public:
        struct Header {
            uint32 major_version;
            uint32 minor_version;
            uint32 reserved;
            glm::vec3 bbox_min;
            glm::vec3 bbox_max;
        };

        RBMdlFile(IO::File &file);

        ~RBMdlFile() = default;

        [[nodiscard]] const std::vector<std::unique_ptr<RBMdl::RenderBlock> > &blocks() const {
            return m_blocks;
        }

    private:
        std::vector<std::unique_ptr<RBMdl::RenderBlock> > m_blocks;
    };

    VM::NodePtr export_rbmdl(ApexAppState &app_state, uint64 path_hash, const std::unique_ptr<IO::File> &&buffer);
}
