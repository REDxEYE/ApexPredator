#include "exporter/apex/amf_policy.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <format>
#include <optional>
#include <ranges>
#include <vector>

#include "apex/hashes.h"
#include "apex/adf/adf.h"
#include "exporter/amf_attribute_decode.h"
#include "redscore/platform/logger.h"
#include "redscore/utils/common.h"
#include "tracy/Tracy.hpp"
#include "utils/hash_helper.h"

namespace {
struct Rage2BufferSlice {
    const ADFTypes::AmfMeshBuffers *buffers;
    size_t index;
};

bool rage2_range(size_t start, size_t count, size_t size) {
    return start <= size && count <= size - start;
}

// A few game vertices encode the same direction for tangent and bitangent.
// Their cross product is zero; recover the normal from a referenced face.
void repair_degenerate_frame_normals(VM::VertexAttribute &normal,
                                     const VM::VertexAttribute &position,
                                     VM::VertexAttribute &tangent,
                                     const ADFTypes::AmfMesh &mesh,
                                     const uint8 *indices_start) {
    const auto vector_at = [](const VM::VertexAttribute &attribute, size_t vertex) {
        std::array<float, 3> value;
        std::memcpy(value.data(), attribute.data.data() + vertex * 3 * sizeof(float),
                    3 * sizeof(float));
        return value;
    };
    const auto missing = [&normal, &vector_at](size_t vertex) {
        return vector_at(normal, vertex) == std::array<float, 3>{0.f, 0.f, 0.f};
    };
    const auto index_at = [&mesh](const uint8 *data, size_t i) {
        uint32 index;
        if (mesh.IndexBufferStride == 2) {
            uint16 value;
            std::memcpy(&value, data + i * 2, 2);
            index = value;
        } else {
            std::memcpy(&index, data + i * 4, 4);
        }
        return index;
    };
    size_t cursor = 0;
    for (const auto &sub_mesh: mesh.SubMeshes) {
        const size_t count = sub_mesh.IndexCount;
        if (cursor > mesh.IndexCount || count > mesh.IndexCount - cursor) break;
        const uint8 *indices = indices_start + cursor * mesh.IndexBufferStride;
        cursor += count;
        for (size_t triangle = 0; triangle < count / 3; ++triangle) {
            const uint32 a = index_at(indices, triangle * 3);
            const uint32 b = index_at(indices, triangle * 3 + 1);
            const uint32 c = index_at(indices, triangle * 3 + 2);
            if (a >= normal.count || b >= normal.count || c >= normal.count ||
                !(missing(a) || missing(b) || missing(c))) continue;
            const auto p0 = vector_at(position, a);
            const auto p1 = vector_at(position, b);
            const auto p2 = vector_at(position, c);
            const std::array<float, 3> u{p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
            const std::array<float, 3> v{p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
            std::array<float, 3> face{
                u[1] * v[2] - u[2] * v[1],
                u[2] * v[0] - u[0] * v[2],
                u[0] * v[1] - u[1] * v[0],
            };
            const float length = std::sqrt(face[0] * face[0] + face[1] * face[1] +
                                           face[2] * face[2]);
            if (length <= 1e-6f) continue;
            for (float &component: face) component /= length;
            for (const uint32 vertex: {a, b, c}) {
                if (!missing(vertex)) continue;
                std::array<float, 3> t;
                std::memcpy(t.data(), tangent.data.data() + vertex * 4 * sizeof(float),
                            3 * sizeof(float));
                const float along_normal = t[0] * face[0] + t[1] * face[1] +
                                           t[2] * face[2];
                for (size_t component = 0; component < 3; ++component)
                    t[component] -= along_normal * face[component];
                float tangent_length = std::sqrt(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);
                if (tangent_length <= 1e-6f) {
                    t = std::abs(face[2]) < 0.9f
                            ? std::array<float, 3>{face[1], -face[0], 0.f}
                            : std::array<float, 3>{0.f, face[2], -face[1]};
                    tangent_length = std::sqrt(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);
                }
                for (float &component: t) component /= tangent_length;
                std::memcpy(tangent.data.data() + vertex * 4 * sizeof(float),
                            t.data(), 3 * sizeof(float));
                std::memcpy(normal.data.data() + vertex * 3 * sizeof(float),
                            face.data(), 3 * sizeof(float));
            }
        }
    }
    // Unreferenced vertices (or degenerate triangles) still need a unit normal.
    for (size_t vertex = 0; vertex < normal.count; ++vertex) {
        if (!missing(vertex)) continue;
        std::array<float, 3> t;
        std::memcpy(t.data(), tangent.data.data() + vertex * 4 * sizeof(float),
                    3 * sizeof(float));
        std::array<float, 3> fallback = std::abs(t[2]) < 0.9f
                                            ? std::array<float, 3>{t[1], -t[0], 0.f}
                                            : std::array<float, 3>{0.f, t[2], -t[1]};
        const float length = std::sqrt(fallback[0] * fallback[0] +
                                       fallback[1] * fallback[1] + fallback[2] * fallback[2]);
        for (float &component: fallback) component /= length;
        std::memcpy(normal.data.data() + vertex * 3 * sizeof(float),
                    fallback.data(), 3 * sizeof(float));
    }
}

bool export_amf_lod(VM::SceneBuilder &helper, const std::string_view mesh_name,
                    const VM::NodePtr &mesh_root_node, const ADFTypes::AmfLodGroup &lod_group,
                    const int32 lod_id, const std::vector<Rage2BufferSlice> &all_buffers) {
    bool emitted = false;
    for (const auto [mesh_id, mesh]: lod_group.Meshes | std::views::enumerate) {
        const size_t merged_index = mesh.MergedBufferIndex;
        if (merged_index >= all_buffers.size()) {
            GLog_Error("Skipping LOD {} mesh {}: merged buffer index out of bounds", lod_id, mesh_id);
            continue;
        }
        const Rage2BufferSlice& slice = all_buffers[merged_index];
        const ADFTypes::AmfMeshBuffers *buffers = slice.buffers;
        // MergedBufferIndex numbers the pairs across both files; IndexOffsets
        // count indices, whereas VertexOffsets and stream offsets count bytes.
        if (!buffers || slice.index >= buffers->IndexOffsets.size() ||
            slice.index >= buffers->VertexOffsets.size() ||
            mesh.IndexBufferStride != 2 && mesh.IndexBufferStride != 4 ||
            mesh.VertexCount == 0 || mesh.SubMeshes.empty()) {
            GLog_Warning("Skipping LOD {} mesh {}: missing merged buffer or unsupported indices", lod_id, mesh_id);
            continue;
        }
        const auto &bytes = buffers->MergedBuffer.Data;
        const size_t index_start = static_cast<size_t>(buffers->IndexOffsets[slice.index]) * mesh.IndexBufferStride;
        const size_t vertex_start = buffers->VertexOffsets[slice.index];
        const size_t index_size = static_cast<size_t>(mesh.IndexCount) * mesh.IndexBufferStride;
        if (!rage2_range(index_start, index_size, bytes.size()) || vertex_start > bytes.size()) {
            GLog_Warning("Skipping LOD {} mesh {}: merged buffer exceeds bounds", lod_id, mesh_id);
            continue;
        }

        std::vector<VM::VertexAttribute> attributes;
        std::optional<size_t> degenerate_normal_index;
        uint32 uv_set = 0;
        uint32 joint_set = 0, weight_set = 0;
        bool has_position = false;
        for (const auto &attr: mesh.StreamAttributes) {
            const size_t stream = attr.StreamIndex;
            if (stream >= mesh.VertexStreamStrides.size() || stream >= mesh.VertexStreamOffsets.size())
                continue;
            const size_t offset = vertex_start + static_cast<size_t>(mesh.VertexStreamOffsets[stream]) + attr.StreamOffset;
            const AMF::AttributeInput input{
                attr, bytes, offset, mesh.VertexStreamStrides[stream], mesh.VertexCount,
                mesh.BoneIndexLookup, uv_set
            };
            if (attr.Usage == ADFTypes::AmfUsage::AmfUsage_TangentSpace) {
                if (auto frame = AMF::decode_tangent_space(input)) {
                    if (frame->has_degenerate_normal) degenerate_normal_index = attributes.size();
                    attributes.emplace_back(std::move(frame->normal));
                    attributes.emplace_back(std::move(frame->tangent));
                }
                continue;
            }
            auto decoded = AMF::decode_attribute(input);
            if (!decoded) continue;
            if (decoded->usage == VM::ElementUsage::TexCoord) ++uv_set;
            if (decoded->usage == VM::ElementUsage::Position) has_position = true;
            if (decoded->usage == VM::ElementUsage::Joints) decoded->set = joint_set++;
            if (decoded->usage == VM::ElementUsage::Weights) decoded->set = weight_set++;
            attributes.emplace_back(std::move(*decoded));
        }
        normalize_amf_weight_sets(attributes);
        if (!has_position) {
            GLog_Warning("Skipping LOD {} mesh {} without supported positions", lod_id, mesh_id);
            continue;
        }
        if (degenerate_normal_index) {
            const auto position = std::find_if(attributes.begin(), attributes.end(),
                                               [](const VM::VertexAttribute &attribute) {
                                                   return attribute.usage == VM::ElementUsage::Position;
                                               });
            repair_degenerate_frame_normals(attributes[*degenerate_normal_index], *position,
                                            attributes[*degenerate_normal_index + 1], mesh,
                                            bytes.data() + index_start);
        }

        auto node = helper.create_node();
        node->name = std::format("{}_lod_{}_mesh_{}", path_utils::stem(mesh_name), lod_id, mesh_id);
        node->model = std::make_shared<VM::Model>();
        auto &model_mesh = node->model->submodels.emplace_back().meshes.emplace_back();
        model_mesh.name = find_name(mesh.MeshTypeId).value_or(
            std::format("mesh_{:08X}", mesh.MeshTypeId.storage));
        size_t index_cursor = 0;
        for (const auto &sub_mesh: mesh.SubMeshes) {
            const size_t index_count = sub_mesh.IndexCount;
            if (index_cursor > mesh.IndexCount || index_count > mesh.IndexCount - index_cursor) {
                GLog_Warning("Skipping invalid submesh index count in LOD {} mesh {}", lod_id, mesh_id);
                break;
            }
            const uint8 *indices = bytes.data() + index_start + index_cursor * mesh.IndexBufferStride;
            index_cursor += index_count;
            if (index_count == 0) continue;
            bool valid_indices = true;
            for (size_t i = 0; i < index_count; ++i) {
                uint32 value;
                if (mesh.IndexBufferStride == 2) {
                    uint16 short_index;
                    std::memcpy(&short_index, indices + i * 2, 2);
                    value = short_index;
                } else std::memcpy(&value, indices + i * 4, 4);
                if (value >= mesh.VertexCount) {
                    valid_indices = false;
                    break;
                }
            }
            if (!valid_indices) {
                GLog_Warning("Skipping out-of-range indices in LOD {} mesh {}", lod_id, mesh_id);
                continue;
            }
            auto &primitive = model_mesh.primitives.emplace_back();
            const auto material_name = find_name(sub_mesh.SubMeshId).value_or(
                std::format("material_{:08X}", sub_mesh.SubMeshId.storage));
            primitive.material = helper.find_material(material_name);
            primitive.set_indices(indices, index_count * mesh.IndexBufferStride,
                                  mesh.IndexBufferStride == 2 ? VM::IndexType::U16 : VM::IndexType::U32,
                                  index_count);
            for (const auto &attribute: attributes)
                primitive.set_attribute(attribute);
        }
        if (model_mesh.primitives.empty()) continue;
        // Some character meshes have no MeshProperties despite carrying skinning streams.
        const auto skin = helper.current_skin();
        if (skin &&
            std::ranges::any_of(attributes, [](const auto &attribute) {
                return attribute.usage == VM::ElementUsage::Joints;
            }) &&
            std::ranges::any_of(attributes, [](const auto &attribute) {
                return attribute.usage == VM::ElementUsage::Weights;
            }))
            node->skin = skin;
        helper.set_parent(mesh_root_node, node);
        emitted = true;
    }
    return emitted;
}
}

VM::NodePtr export_amf_mesh(ApexAppState &app_state, uint64 path_hash,
                            const ADFTypes::AmfMeshHeader *header,
                            const ADFTypes::AmfMeshBuffers *mesh_buffers) {
    ZoneScoped
    auto &helper = app_state.models();
    std::string mesh_name = find_asset_name(path_hash).value_or(std::format("mesh_{:08X}", path_hash));
    auto mesh_root_node = helper.create_node();
    mesh_root_node->name = mesh_name;
    std::unique_ptr<ADFTypes::AmfMeshBuffers> hi_res_buffers;
    if (const auto hi_res_name = find_lookup3_name(header->HighLodPath.storage)) {
        std::string_view path = *hi_res_name;
        if (path.starts_with("intermediate/")) path.remove_prefix(strlen("intermediate/"));
        if (auto hi_res_buffer = app_state.manager().get(asset_path_hash(path))) {
            try {
                auto hi_res_adf = ADF::ADFFile::from_buffer(std::move(hi_res_buffer));
                if (!hi_res_adf.instances().empty())
                    hi_res_buffers = hi_res_adf.read_instance<ADFTypes::AmfMeshBuffers>(0);
                if (!hi_res_buffers)
                    GLog_Warning("Unsupported high-resolution mesh buffers: {}", path);
            } catch (const std::exception &error) {
                GLog_Warning("Unable to read high-resolution mesh buffers {}: {}", path, error.what());
            }
        }
    }

    std::vector<Rage2BufferSlice> all_buffers;
    const auto add_buffers = [&all_buffers](const ADFTypes::AmfMeshBuffers *buffers) {
        if (!buffers) return;
        const size_t count = std::min(buffers->IndexOffsets.size(), buffers->VertexOffsets.size());
        for (size_t index = 0; index < count; ++index)
            all_buffers.push_back({buffers, index});
    };
    add_buffers(mesh_buffers);
    add_buffers(hi_res_buffers.get());

    // LODIndex 0 is the best detail; fall back to the first available coarser
    // group if its streamed high-resolution buffers are unavailable.
    std::vector<const ADFTypes::AmfLodGroup *> lods;
    lods.reserve(header->LodGroups.size());
    for (const auto &lod: header->LodGroups) lods.push_back(&lod);
    std::ranges::sort(lods, {}, &ADFTypes::AmfLodGroup::LODIndex);
    for (const auto *lod: lods)
        if (export_amf_lod(helper, mesh_name, mesh_root_node, *lod, lod->LODIndex, all_buffers)) break;
    return mesh_root_node;
}

std::pair<uint64, std::unique_ptr<IO::File>> open_amf_model_mesh(
    ApexAppState &app_state, const ADFTypes::AmfModel &model) {
    const auto mesh_name = find_lookup3_name(model.Mesh.storage);
    const uint64 mesh_hash = mesh_name ? asset_path_hash(*mesh_name) : model.Mesh.storage;
    auto mb = app_state.manager().get(mesh_hash);
    // GTOC archives may expose the lookup3 key even when their path is absent
    // from the asset-path table; never reinterpret it as a Murmur asset hash.
    if (!mb && mesh_name) mb = app_state.manager().get(model.Mesh.storage);
    return {mesh_hash, std::move(mb)};
}
