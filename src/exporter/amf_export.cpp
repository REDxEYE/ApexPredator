// Created by RED on 12.01.2026.

#include "exporter/amf_export.h"

#include "games.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <optional>
#include <span>
#include <ranges>

#include "apex/hashes.h"
#include "apex/adf/adf.h"
#include "apex/adf/generated/adf_types_fwd.h"
#include "exporter/adf_export.h"
#include "exporter/amf_attribute_decode.h"
#include "exporter/ddsc_export.h"
#include "redscore/platform/logger.h"
#include "redscore/platform/texture/texture_ops.h"
#include "redscore/utils/common.h"
#include "tracy/Tracy.hpp"
#include "utils/hash_helper.h"

namespace {
    // A vertex's influences span every WEIGHTS_n stream, not each group of four independently.
    void normalize_weight_sets(std::vector<VM::VertexAttribute> &attributes) {
        std::vector<VM::VertexAttribute *> weights;
        for (auto &attribute: attributes)
            if (attribute.usage == VM::ElementUsage::Weights) weights.push_back(&attribute);
        if (weights.empty()) return;

        for (size_t vertex = 0; vertex < weights.front()->count; ++vertex) {
            float total = 0.f;
            for (const auto *attribute: weights) {
                float values[4];
                std::memcpy(values, attribute->data.data() + vertex * sizeof(values), sizeof(values));
                for (float value: values) total += value;
            }
            if (total <= 0.f) continue;
            for (auto *attribute: weights) {
                float values[4];
                auto *data = attribute->data.data() + vertex * sizeof(values);
                std::memcpy(values, data, sizeof(values));
                for (float &value: values) value /= total;
                std::memcpy(data, values, sizeof(values));
            }
        }
    }
}

#if GAME==GAME_GENERATION_ZERO
void export_amf_lod(VM::SceneBuilder &helper, const std::string_view mesh_name,
                    const VM::NodePtr &mesh_root_node, const ADFTypes::AmfLodGroup &lod_group,
                    const int32 lod_id,
                    const std::vector<ADFTypes::AmfBuffer> &all_index_buffer,
                    const std::vector<ADFTypes::AmfBuffer> &all_vertex_buffer
) {
    for (const auto [mesh_id, mesh]: lod_group.Meshes | std::views::enumerate) {
        auto lod_name = std::format("{}_lod_{}_mesh_{}", path_utils::stem(mesh_name), lod_id, mesh_id);
        auto node = helper.create_node();
        node->name = lod_name;

        helper.set_parent(mesh_root_node, node);

        // if (mesh.MeshProperties.type_hash == STI_TYPE_HASH_GeneralMeshConstants) {
        //     // GeneralMeshConstants* constants = (GeneralMeshConstants*)mesh->MeshProperties.data;
        // }
        // else {
        //     GLog_Warning("Unsupported mesh prop type: %08X", mesh->MeshProperties.type_hash);
        // }

        uint32 vertex_count = mesh.VertexCount;
        uint32 index_buffer_index = mesh.IndexBufferIndex;
        if (index_buffer_index >= all_index_buffer.size()) {
            continue;
        }

        uint32 index_buffer_stride = mesh.IndexBufferStride;
        uint32 index_buffer_offset = mesh.IndexBufferOffset;
        auto &vertex_buffer_indices = mesh.VertexBufferIndices;
        auto &vertex_buffer_strides = mesh.VertexStreamStrides;
        auto &vertex_buffer_offsets = mesh.VertexStreamOffsets;
        auto &bone_lookup = mesh.BoneIndexLookup;
        auto &amf_attributes = mesh.StreamAttributes;

        auto &used_index_buffer = all_index_buffer[index_buffer_index];

        // auto mesh_type_name = find_name(mesh.MeshTypeId);

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

            if (material) {
                primitive.material = material;
            }
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
            normalize_weight_sets(primitive.attributes);
        }
        if (const auto constants = ADF::as<ADFTypes::GeneralMeshConstants>(mesh.MeshProperties)) {
            if (constants->IsSkinnedMesh) {
                const auto skin = helper.current_skin();
                if (skin) {
                    node->skin = skin;
                }
            }
        }
    }
}
#elif GAME==GAME_RAGE2
namespace {
    struct Rage2BufferSlice {
        const ADFTypes::AmfMeshBuffers *buffers;
        size_t index;
    };

    bool rage2_range(size_t start, size_t count, size_t size) {
        return start <= size && count <= size - start;
    }


    // Rage2 BC5 normals keep X/Y in two channels, written as LA by the PNG writer.
    // Expand them to the RGB tangent-space normals expected by glTF.
    std::unique_ptr<Texture> rage2_unpack_normal(std::unique_ptr<Texture> texture) {
        const size_t channels = texture->channel_count();
        const size_t count = size_t(texture->width()) * texture->height();
        if (texture->bpc() != 1 || texture->is_float() || texture->depth() != 1 ||
            (channels != 2 && channels != 4) || texture->data().size() != count * channels)
            throw std::runtime_error("Unsupported Rage2 normal texture layout");

        const auto blue = [](uint8 red, uint8 green) -> uint8 {
            const float x = red / 127.5f - 1.f;
            const float y = green / 127.5f - 1.f;
            const float z = std::sqrt(std::max(0.f, 1.f - x * x - y * y));
            return static_cast<uint8>(std::lround((z * 0.5f + 0.5f) * 255.f));
        };
        if (channels == 2) {
            std::vector<uint8> rgba(count * 4);
            const auto &src = texture->data();
            for (size_t i = 0; i < count; ++i) {
                const uint8 red = src[i * 2], green = src[i * 2 + 1];
                rgba[i * 4] = red;
                rgba[i * 4 + 1] = green;
                rgba[i * 4 + 2] = blue(red, green);
                rgba[i * 4 + 3] = 255;
            }
            return std::make_unique<Texture>(texture->width(), texture->height(), 1, 1, 4, false,
                                             std::move(rgba));
        }
        auto &rgba = texture->data();
        for (size_t i = 0; i < count; ++i) {
            uint8 *pixel = rgba.data() + i * 4;
            pixel[1] = pixel[3];
            pixel[2] = blue(pixel[0], pixel[1]);
            pixel[3] = 255;
        }
        return texture;
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
}

bool export_amf_lod(VM::SceneBuilder &helper, const std::string_view mesh_name,
                    const VM::NodePtr &mesh_root_node, const ADFTypes::AmfLodGroup &lod_group,
                    const int32 lod_id,
                    const std::vector<Rage2BufferSlice> &all_buffers
) {
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
        normalize_weight_sets(attributes);
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
#else
#error "Unsupported game"
#endif
VM::NodePtr export_amf_mesh(ApexAppState &app_state, uint64 path_hash,
                            const ADFTypes::AmfMeshHeader *header,
                            const ADFTypes::AmfMeshBuffers *mesh_buffers) {
    ZoneScoped
    auto &helper = app_state.models();

#if GAME==GAME_RAGE2
    std::string mesh_name = find_asset_name(path_hash).value_or(std::format("mesh_{:08X}", path_hash));
#else
    std::string mesh_name = find_lookup3_name(path_hash).value_or(std::format("mesh_{:08X}", path_hash));
#endif
    auto mesh_root_node = helper.create_node();
    mesh_root_node->name = mesh_name;
#if GAME==GAME_GENERATION_ZERO
    std::vector<ADFTypes::AmfBuffer> all_vertex_buffer = {};
    std::vector<ADFTypes::AmfBuffer> all_index_buffer = {};
    all_vertex_buffer.reserve(mesh_buffers->VertexBuffers.size());

    all_index_buffer.reserve(mesh_buffers->IndexBuffers.size());

    for (const auto &buffer: mesh_buffers->VertexBuffers) {
        all_vertex_buffer.emplace_back(buffer);
    }

    for (const auto &amf_buffer: mesh_buffers->IndexBuffers) {
        all_index_buffer.emplace_back(amf_buffer);
    }


    // hires fix
    if (const auto hi_res_path_full_tmp = find_asset_name(header->HighLodPath)) {
        const auto hi_res_path_full = hi_res_path_full_tmp.value();
        if (hi_res_path_full.contains("intermediate/")) {
            auto hi_res_path = hi_res_path_full.substr(strlen("intermediate/"));

            if (auto hi_res_buffer = app_state.manager().get(asset_path_hash(hi_res_path))) {
                ADF::ADFFile hi_res_adf = ADF::ADFFile::from_buffer(std::move(hi_res_buffer));
                const auto hi_res_buffers = std::move(hi_res_adf.read_instance<ADFTypes::AmfMeshBuffers>(0));
                if (!hi_res_buffers) {
                    GLog_Warning("Unexpected hi-res mesh buffers type: %08X", hi_res_adf.instances()[0].type_hash);
                }

                for (const auto &buffer: hi_res_buffers->VertexBuffers) {
                    all_vertex_buffer.emplace_back(buffer);
                }

                for (const auto &amf_buffer: hi_res_buffers->IndexBuffers) {
                    all_index_buffer.emplace_back(amf_buffer);
                }
            }
        }
    }

    export_amf_lod(helper, mesh_name, mesh_root_node, header->LodGroups.back(), 0, all_index_buffer, all_vertex_buffer);
#elif GAME==GAME_RAGE2
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
#else
#error "Unsupported game"
#endif


    return mesh_root_node;
}

VM::NodePtr export_amf_model(ApexAppState &app_state, const ADFTypes::AmfModel *amf_model,
                             const uint64 path_hash) {
    ZoneScoped
    auto &helper = app_state.models();
    auto model_root_node = helper.create_node();
    const std::filesystem::path model_path = find_name(path_hash).value_or(std::format("model_{:08X}", path_hash));
    model_root_node->name = model_path.filename().string();

    GLog_Info("Exporting {} model", model_path.string());

    for (const auto &amf_material: amf_model->Materials) {
        std::string material_name = find_name(amf_material.Name).value_or(
            std::format("material_{:08X}", amf_material.Name.storage));
        std::string render_block_id = find_name(amf_material.RenderBlockId).value_or(
            std::format("renderblock_{:08X}", amf_material.RenderBlockId.storage));

        auto material = helper.find_material(material_name);

        if (!material) {
            auto new_material = helper.material(material_name);
            new_material->name = material_name;


            GLog_Info("Material {} -> {}", material_name, render_block_id);
            for (const auto [tex_id, texture]: amf_material.Textures | std::views::enumerate) {
                auto tex_path = find_name(texture.storage);
                if (!tex_path) {
                    continue;
                }
                GLog_Info("\tSlot {} -> {}", tex_id, tex_path.value());
            }
#if GAME==GAME_GENERATION_ZERO
            if (render_block_id == "GeneralR2") {
                if (!app_state.skip_textures) {
                    const auto constants = ADF::as<ADFTypes::GeneralR2Constants>(amf_material.Attributes);
                    if (!constants) {
                        GLog_Warning("Unsupported GeneralR2 material attribute type");
                        continue;
                    }
                    new_material->base_color[0] = 1.0f;
                    new_material->base_color[1] = 1.0f;
                    new_material->base_color[2] = 1.0f;
                    new_material->base_color[3] = 1.0f;
                    new_material->metallic_factor = 1.0f;
                    new_material->roughness_factor = 1.0f;

                    if (amf_material.Textures.size() >= 3) {
                        std::unique_ptr<Texture> diffuse_texture = nullptr;

                        if (const auto diffuse_path = find_name(amf_material.Textures[0])) {
                            diffuse_texture = convert_ddsc(app_state, amf_material.Textures[0]);
                            auto image = VM::png_texture(
                                std::format("{}_{:08X}.png", path_utils::filename(diffuse_path.value()),
                                            asset_path_hash(diffuse_path.value())),
                                diffuse_texture->save_to_memory(MemoryFormat::PNG));
                            new_material->albedo = image;
                        }
                        if (auto normal_path = find_name(amf_material.Textures[1])) {
                            auto normal_texture = convert_ddsc(app_state, amf_material.Textures[1]);
                            auto image = VM::png_texture(
                                std::format("{}_{:08X}.png", path_utils::filename(normal_path.value()),
                                            asset_path_hash(normal_path.value())),
                                normal_texture->save_to_memory(MemoryFormat::PNG));
                            new_material->normal = image;
                            new_material->normal_scale = 1.0f;
                        }

                        if (auto orm_path = find_name(amf_material.Textures[2])) {
                            auto orm_texture = convert_ddsc(app_state, amf_material.Textures[2]);
                            auto image = VM::png_texture(
                                std::format("{}_{:08X}.png", path_utils::filename(orm_path.value()),
                                            asset_path_hash(orm_path.value())),
                                orm_texture->save_to_memory(MemoryFormat::PNG));
                            new_material->metallic_roughness = image;
                        }
                        if (constants->UseEmissive && amf_material.Textures.size() > 4) {
                            if (auto emission_path = find_name(amf_material.Textures[4])) {
                                const uint64 hash = asset_path_hash(emission_path.value());
                                auto texture_name = std::format("{}_{:08X}",
                                                                path_utils::filename(emission_path.value()), hash);
                                auto emission_texture = convert_ddsc(app_state, amf_material.Textures[4]);
                                if (!constants->EmissiveTextureHasColor && diffuse_texture) {
                                    auto new_emission_texture = TextureOps::multiply(
                                        emission_texture.get(), diffuse_texture.get());

                                    auto texture_save_path = app_state.export_path();
                                    texture_save_path /= texture_name;

                                    if (new_emission_texture) {
                                        new_emission_texture->save(texture_save_path);
                                        emission_texture = std::move(new_emission_texture);
                                    }
                                }
                                auto image = VM::png_texture(texture_name,
                                                             emission_texture->save_to_memory(MemoryFormat::PNG));
                                new_material->emissive = image;
                                new_material->emissive_factor[0] = 1.0f;
                                new_material->emissive_factor[1] = 1.0f;
                                new_material->emissive_factor[2] = 1.0f;
                            }
                        }

                        // if (constants->UseAlbedoDetail) {
                        //     StringView albedo_detail_path = find_name(amf_material.Textures.items[5]);
                        //     Texture *albedo_detail = convert_ddsc(app_state, amf_material.Textures.items[5]);
                        //     String *texture_save_path = GLTFContext_data_path(context);
                        //     const uint64 hash = hash_vstring(albedo_detail_path);
                        //     String tex_name = {};
                        //     Path_filename_sv(albedo_detail_path, &tex_name);
                        //     String_append_format(texture_save_path, "/%s_%08X", String_cstr(&tex_name), hash);
                        //     Texture_save(albedo_detail, texture_save_path);
                        //     Texture_free(albedo_detail);
                        //     String_free(&tex_name);
                        //     String_free(texture_save_path);
                        // }
                        // if (constants->UseNormalDetail) {
                        //     StringView normal_detail_path = find_name(amf_material.Textures.items[6]);
                        //     Texture *normal_detail = convert_ddsc(app_state, amf_material.Textures.items[6]);
                        //     String *texture_save_path = GLTFContext_data_path(context);
                        //     const uint64 hash = hash_vstring(normal_detail_path);
                        //     String tex_name = {};
                        //     Path_filename_sv(normal_detail_path, &tex_name);
                        //     String_append_format(texture_save_path, "/%s_%08X", String_cstr(&tex_name), hash);
                        //     Texture_save(normal_detail, texture_save_path);
                        //     Texture_free(normal_detail);
                        //     String_free(&tex_name);
                        //     String_free(texture_save_path);
                        // }
                    }
                }
            } else {
                GLog_Warning("Unsupported material render block: 0x{:08X}", amf_material.RenderBlockId.storage);
                continue;
            }

#elif GAME==GAME_RAGE2
            // GeneralR2 and Character share diffuse, normal and MPM in their
            // first three slots. The remaining Character slots are shader-specific.
            // RAGE2 has no generated material constants for these render blocks,
            // so do not interpret their deferred attributes.
            if ((render_block_id == "GeneralR2" || render_block_id == "Character") && !app_state.skip_textures) {
                const auto load_slot = [&](size_t slot) -> VM::TexturePtr {
                    if (slot >= amf_material.Textures.size()) return {};
                    const auto texture_name = find_lookup3_name(amf_material.Textures[slot].storage);
                    if (!texture_name) return {};
                    try {
                        const auto hash = asset_path_hash(*texture_name);
                        auto texture = convert_ddsc(app_state, hash);
                        if (!texture) return {};
                        if (slot == 1) texture = rage2_unpack_normal(std::move(texture));
                        return VM::png_texture(
                            std::format("{}_{:08X}.png", path_utils::filename(*texture_name), hash),
                            texture->save_to_memory(MemoryFormat::PNG));
                    } catch (const std::exception &error) {
                        GLog_Warning("Unable to decode AMF texture {}: {}", *texture_name, error.what());
                        return {};
                    }
                };
                new_material->albedo = load_slot(0);
                new_material->normal = load_slot(1);
                new_material->metallic_roughness = load_slot(2);
            }
#else
#error "Unsupported game"
#endif
        }
#if GAME==GAME_RAGE2
        // These Character slots are auxiliary blood/gib maps, not the material's
        // default PBR channels. Export them separately and retain their slot IDs.
        if (render_block_id == "Character" && !app_state.skip_textures) {
            for (size_t slot: {8, 9}) {
                if (slot >= amf_material.Textures.size()) continue;
                const auto texture_name = find_lookup3_name(amf_material.Textures[slot].storage);
                if (!texture_name) continue;
                try {
                    const auto hash = asset_path_hash(*texture_name);
                    const auto source_path = std::filesystem::path(*texture_name);
                    const auto destination = app_state.export_path() / source_path;
                    auto image_path = source_path;
                    image_path += ".png";
                    if (!std::filesystem::exists(app_state.export_path() / image_path)) {
                        auto texture = convert_ddsc(app_state, hash);
                        if (!texture) continue;
                        if (texture->is_float()) image_path.replace_extension(".dds");
                        if (!texture->is_float() || !std::filesystem::exists(app_state.export_path() / image_path)) {
                            std::filesystem::create_directories(destination.parent_path());
                            texture->save(destination);
                        }
                    }
                    model_root_node->extras["rage2CharacterTextures"][material_name][std::to_string(slot)] =
                            image_path.generic_string();
                } catch (const std::exception &error) {
                    GLog_Warning("Unable to export Character texture {}: {}", *texture_name, error.what());
                }
            }
        }
#endif
    }

#if GAME==GAME_RAGE2
    const auto mesh_name = find_lookup3_name(amf_model->Mesh.storage);
    const uint64 mesh_hash = mesh_name ? asset_path_hash(*mesh_name) : amf_model->Mesh.storage;
    auto mb = app_state.manager().get(mesh_hash);
    // GTOC archives may expose the lookup3 key even when their path is absent
    // from the asset-path table; never reinterpret it as a Murmur asset hash.
    if (!mb && mesh_name) mb = app_state.manager().get(amf_model->Mesh.storage);
#else
    const uint64 mesh_hash = amf_model->Mesh;
    auto mb = app_state.manager().get(amf_model->Mesh);
#endif
    if (!mb) {
        GLog_Error("Mesh file not found: {:08X}", amf_model->Mesh.storage);
        return model_root_node;
    }

    const auto mesh_root_node = export_adf_file_from_buffer(app_state, mesh_hash, std::move(mb));
    if (mesh_root_node) helper.set_parent(model_root_node, mesh_root_node);

    return model_root_node;
}
