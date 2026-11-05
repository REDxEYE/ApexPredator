#include "exporter/apex/amf_policy.h"

#include <cstring>
#include <format>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "apex/hashes.h"
#include "apex/adf/adf.h"
#include "exporter/amf_attribute_decode.h"
#include "redscore/platform/logger.h"
#include "redscore/utils/common.h"
#include "tracy/Tracy.hpp"
#include "utils/hash_helper.h"

namespace {
void export_amf_lod(VM::SceneBuilder &helper, const std::string_view mesh_name,
                    const VM::NodePtr &mesh_root_node, const ADFTypes::AmfLodGroup &lod_group,
                    const int32 lod_id,
                    const std::vector<ADFTypes::AmfBuffer> &all_index_buffer,
                    const std::vector<ADFTypes::AmfBuffer> &all_vertex_buffer) {
    for (const auto [mesh_id, mesh]: lod_group.Meshes | std::views::enumerate) {
        auto lod_name = std::format("{}_lod_{}_mesh_{}", path_utils::stem(mesh_name), lod_id, mesh_id);
        auto node = helper.create_node();
        node->name = lod_name;
        helper.set_parent(mesh_root_node, node);

        uint32 vertex_count = mesh.VertexCount;
        uint32 index_buffer_index = mesh.IndexBufferIndex;
        if (index_buffer_index >= all_index_buffer.size()) continue;

        uint32 index_buffer_stride = mesh.IndexBufferStride;
        uint32 index_buffer_offset = mesh.IndexBufferOffset;
        auto &vertex_buffer_indices = mesh.VertexBufferIndices;
        auto &vertex_buffer_strides = mesh.VertexStreamStrides;
        auto &vertex_buffer_offsets = mesh.VertexStreamOffsets;
        auto &bone_lookup = mesh.BoneIndexLookup;
        auto &amf_attributes = mesh.StreamAttributes;
        auto &used_index_buffer = all_index_buffer[index_buffer_index];

        auto mesh_type_name = find_name(mesh.MeshTypeId).value_or(std::format("mesh_{:08X}", mesh.MeshTypeId.storage));
        node->model = std::make_shared<VM::Model>();
        auto &submodel = node->model->submodels.emplace_back();
        auto &model_mesh = submodel.meshes.emplace_back();
        model_mesh.name = mesh_type_name;
        model_mesh.primitives.resize(mesh.SubMeshes.size());

        for (const auto &[sub_mesh_id, sub_mesh]: mesh.SubMeshes | std::views::enumerate) {
            const auto material_name = find_name(sub_mesh.SubMeshId).value_or(
                std::format("material_{:08X}", sub_mesh.SubMeshId.storage));
            auto material = helper.find_material(material_name);
            auto &primitive = model_mesh.primitives[sub_mesh_id];
            if (material) primitive.material = material;
            auto *index_data = used_index_buffer.Data.data() + index_buffer_offset + sub_mesh.IndexStreamOffset;
            if (index_buffer_stride != 2 && index_buffer_stride != 4)
                throw std::runtime_error("Unsupported AMF index width");
            primitive.set_indices(index_data, sub_mesh.IndexCount * index_buffer_stride,
                                  index_buffer_stride == 2 ? VM::IndexType::U16 : VM::IndexType::U32,
                                  sub_mesh.IndexCount);

            uint32 uv_count = 0;
            uint32 joint_set = 0, weight_set = 0;
            for (const auto &amf_attribute: amf_attributes) {
                const size_t stream = amf_attribute.StreamIndex;
                if (stream >= vertex_buffer_indices.size() || stream >= vertex_buffer_strides.size() ||
                    stream >= vertex_buffer_offsets.size())
                    throw std::runtime_error("Invalid AMF vertex stream index");
                const size_t buffer_index = vertex_buffer_indices[stream];
                if (buffer_index >= all_vertex_buffer.size())
                    throw std::runtime_error("Invalid AMF vertex buffer index");
                const size_t offset = size_t(vertex_buffer_offsets[stream]) + amf_attribute.StreamOffset;
                const AMF::AttributeInput input{
                    amf_attribute, all_vertex_buffer[buffer_index].Data, offset, vertex_buffer_strides[stream],
                    vertex_count, bone_lookup, uv_count, true
                };
                if (amf_attribute.Usage == ADFTypes::AmfUsage::AmfUsage_TangentSpace) {
                    if (auto frame = AMF::decode_tangent_space(input)) {
                        primitive.attributes.emplace_back(std::move(frame->normal));
                        primitive.attributes.emplace_back(std::move(frame->tangent));
                    }
                    continue;
                }
                auto decoded = AMF::decode_attribute(input);
                if (!decoded) continue;
                if (decoded->usage == VM::ElementUsage::TexCoord) ++uv_count;
                if (decoded->usage == VM::ElementUsage::Joints) decoded->set = joint_set++;
                if (decoded->usage == VM::ElementUsage::Weights) decoded->set = weight_set++;
                primitive.attributes.emplace_back(std::move(*decoded));
            }
            normalize_amf_weight_sets(primitive.attributes);
        }
        if (const auto constants = ADF::as<ADFTypes::GeneralMeshConstants>(mesh.MeshProperties)) {
            if (constants->IsSkinnedMesh) {
                const auto skin = helper.current_skin();
                if (skin) node->skin = skin;
            }
        }
    }
}
}

VM::NodePtr export_amf_mesh(ApexAppState &app_state, uint64 path_hash,
                            const ADFTypes::AmfMeshHeader *header,
                            const ADFTypes::AmfMeshBuffers *mesh_buffers) {
    ZoneScoped
    auto &helper = app_state.models();
    std::string mesh_name = find_lookup3_name(path_hash).value_or(std::format("mesh_{:08X}", path_hash));
    auto mesh_root_node = helper.create_node();
    mesh_root_node->name = mesh_name;
    std::vector<ADFTypes::AmfBuffer> all_vertex_buffer = {};
    std::vector<ADFTypes::AmfBuffer> all_index_buffer = {};
    all_vertex_buffer.reserve(mesh_buffers->VertexBuffers.size());
    all_index_buffer.reserve(mesh_buffers->IndexBuffers.size());
    for (const auto &buffer: mesh_buffers->VertexBuffers) all_vertex_buffer.emplace_back(buffer);
    for (const auto &buffer: mesh_buffers->IndexBuffers) all_index_buffer.emplace_back(buffer);

    // Legacy high-resolution buffers follow the base buffers in their respective streams.
    if (const auto hi_res_path_full_tmp = find_asset_name(header->HighLodPath)) {
        const auto hi_res_path_full = hi_res_path_full_tmp.value();
        if (hi_res_path_full.contains("intermediate/")) {
            auto hi_res_path = hi_res_path_full.substr(strlen("intermediate/"));
            if (auto hi_res_buffer = app_state.manager().get(asset_path_hash(hi_res_path))) {
                ADF::ADFFile hi_res_adf = ADF::ADFFile::from_buffer(std::move(hi_res_buffer));
                const auto hi_res_buffers = std::move(hi_res_adf.read_instance<ADFTypes::AmfMeshBuffers>(0));
                if (!hi_res_buffers)
                    GLog_Warning("Unexpected hi-res mesh buffers type: %08X", hi_res_adf.instances()[0].type_hash);
                for (const auto &buffer: hi_res_buffers->VertexBuffers) all_vertex_buffer.emplace_back(buffer);
                for (const auto &buffer: hi_res_buffers->IndexBuffers) all_index_buffer.emplace_back(buffer);
            }
        }
    }
    export_amf_lod(helper, mesh_name, mesh_root_node, header->LodGroups.back(), 0,
                   all_index_buffer, all_vertex_buffer);
    return mesh_root_node;
}

std::pair<uint64, std::unique_ptr<IO::File>> open_amf_model_mesh(
    ApexAppState &app_state, const ADFTypes::AmfModel &model) {
    const uint64 mesh_hash = model.Mesh;
    auto mb = app_state.manager().get(model.Mesh);
    return {mesh_hash, std::move(mb)};
}
