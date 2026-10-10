//
// Created by red_eye on 10/7/26.
//

#include "apex/rbmdl/rbmdl_file.hpp"

#include "redscore/platform/file/file.h"

#include <algorithm>

#include "apex/rbmdl/common.hpp"
#include "apex/rbmdl/facade.hpp"
#include "apex/rbmdl/general.hpp"
#include "apex/rbmdl/halo.hpp"
#include "apex/rbmdl/lambert.hpp"
#include "redscore/platform/texture/texture.h"

using namespace Apex::RBMdl;

Apex::RBMdlFile::RBMdlFile(IO::File &file) {
    const auto magic = read_rstring(file);
    const auto header = file.read_pod<Header>();
    if (header.major_version!=1 && header.minor_version!=13) {
        throw std::runtime_error(std::format("Unsupported RBMDL version: {}.{}", header.major_version, header.minor_version));
    }
    const uint32 block_count = file.read_u32();
    for (uint32 i = 0; i < block_count; i++) {
        const uint32 block_type_hash = file.read_u32();
        GLog_Info("Block type hash: 0x{:08X}", block_type_hash);
        std::unique_ptr<RenderBlock> block;
        switch (block_type_hash) {
            case GeneralRenderBlock::block_type_hash: {
                block = std::make_unique<GeneralRenderBlock>(file);
                break;
            }
            case FacadeRenderBlock::block_type_hash: {
                block = std::make_unique<FacadeRenderBlock>(file);
                break;
            }
            case HaloRenderBlock::block_type_hash: {
                block = std::make_unique<HaloRenderBlock>(file);
                break;
            }
            case LambertRenderBlock::block_type_hash: {
                block = std::make_unique<LambertRenderBlock>(file);
                break;
            }
            default: {
                GLog_Error("Unknown block type hash: 0x{:08X}. Please report this issue.", block_type_hash);
                throw std::runtime_error(std::format("Unknown block type hash: 0x{:08X}", block_type_hash));
            }
        }
        m_blocks.push_back(std::move(block));
        if (const uint32 end_marker = file.read_u32(); end_marker != 0x89ABCDEF) {
            throw std::runtime_error(std::format("Invalid end marker: 0x{:08X}", end_marker));
        }
    }
}

VM::NodePtr Apex::export_rbmdl(ApexAppState &app_state, uint64 path_hash, const std::unique_ptr<IO::File> &&buffer) {
    const RBMdlFile mdl(*buffer);
    auto &manager = app_state.manager();
    auto &builder = app_state.models();
    const auto name = find_asset_name(path_hash).value_or("Model");
    VM::NodePtr node = builder.create_node(name);

    node->model = std::make_shared<VM::Model>();
    auto &submodel = node->model->submodels.emplace_back();
    auto &model_mesh = submodel.meshes.emplace_back();
    model_mesh.primitives.reserve(mdl.blocks().size());
    for (const auto &render_block: mdl.blocks()) {
        auto &primitive = model_mesh.primitives.emplace_back();
        render_block->to_primitive(manager, builder, primitive);
    }

    return node;
}
