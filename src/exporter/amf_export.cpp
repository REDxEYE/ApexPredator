#include "exporter/amf_export.h"
#include "exporter/apex/amf_policy.h"

#include <filesystem>
#include <format>

#include "apex/hashes.h"
#include "exporter/adf_export.h"
#include "redscore/platform/logger.h"
#include "tracy/Tracy.hpp"

VM::NodePtr export_amf_model(ApexAppState &app_state, const ADFTypes::AmfModel *amf_model,
                             const uint64 path_hash) {
    ZoneScoped
    auto &helper = app_state.models();
    auto model_root_node = helper.create_node();
    const std::filesystem::path model_path = find_name(path_hash).value_or(std::format("model_{:08X}", path_hash));
    model_root_node->name = model_path.filename().string();

    GLog_Info("Exporting {} model", model_path.string());

    for (const auto &amf_material: amf_model->Materials) {
        const std::string material_name = find_name(amf_material.Name).value_or(
            std::format("material_{:08X}", amf_material.Name.storage));
        const std::string render_block_id = find_name(amf_material.RenderBlockId).value_or(
            std::format("renderblock_{:08X}", amf_material.RenderBlockId.storage));
        export_amf_material(app_state, model_root_node, amf_material, material_name, render_block_id);
    }

    auto [mesh_hash, mb] = open_amf_model_mesh(app_state, *amf_model);
    if (!mb) {
        GLog_Error("Mesh file not found: {:08X}", amf_model->Mesh.storage);
        return model_root_node;
    }

    const auto mesh_root_node = export_adf_file_from_buffer(app_state, mesh_hash, std::move(mb));
    if (mesh_root_node) helper.set_parent(model_root_node, mesh_root_node);

    return model_root_node;
}
