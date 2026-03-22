// Created by RED on 12.01.2026.

#include "exporter/amf_export.h"

#include <algorithm>
#include <ranges>

#include "apex/hashes.h"
#include "apex/adf/adf.h"
#include "apex/adf/generated/adf_types_fwd.h"
#include "exporter/adf_export.h"
#include "exporter/ddsc_export.h"
#include "redscore/platform/logger.h"
#include "redscore/platform/texture_ops.h"
#include "redscore/utils/common.h"
#include "tracy/Tracy.hpp"
#include "utils/hash_helper.h"


void export_amf_lod(GltfHelper &helper, const std::string_view mesh_name,
                    GltfHelper::Handle<tinygltf::Node> mesh_root_node, const ADFTypes::AmfLodGroup &lod_group,
                    const int32 lod_id,
                    const std::vector<ADFTypes::AmfBuffer> &all_index_buffer,
                    const std::vector<ADFTypes::AmfBuffer> &all_vertex_buffer
) {
    for (const auto [mesh_id, mesh]: lod_group.Meshes | std::views::enumerate) {
        auto lod_name = std::format("{}_lod_{}_mesh_{}", path_utils::stem(mesh_name), lod_id, mesh_id);
        auto node = helper.make<tinygltf::Node>();
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
        if (index_buffer_index > all_index_buffer.size()) {
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

        auto gl_mesh_name = find_name(mesh.MeshTypeId).value_or(std::format("mesh_{:08X}", mesh.MeshTypeId.storage));
        auto gl_mesh = helper.make<tinygltf::Mesh>();
        gl_mesh->name = gl_mesh_name;
        gl_mesh->primitives.resize(mesh.SubMeshes.size());
        node->mesh = gl_mesh.index();

        for (const auto &[sub_mesh_id, sub_mesh]: mesh.SubMeshes | std::views::enumerate) {
            const auto material_name = find_name(sub_mesh.SubMeshId).value_or(
                std::format("material_{:08X}", sub_mesh.SubMeshId.storage));

            auto material = helper.find<tinygltf::Material>(material_name);
            auto &primitive = gl_mesh->primitives[sub_mesh_id];

            if (material) {
                primitive.material = material.index();
            }
            primitive.mode = TINYGLTF_MODE_TRIANGLES;
            auto *index_data = used_index_buffer.Data.data() + index_buffer_offset + sub_mesh.IndexStreamOffset;
            helper.set_primitive_indices_from_u8(gl_mesh.index(), sub_mesh_id, index_data,
                                                 sub_mesh.IndexCount * index_buffer_stride,
                                                 index_buffer_stride == 2
                                                     ? TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT
                                                     : TINYGLTF_COMPONENT_TYPE_INT,
                                                 sub_mesh.IndexCount, 0, 0, "Indices"
            );

            uint32 uv_count = 0;
            for (const auto &amf_attribute: amf_attributes) {
                uint32 stream_index = vertex_buffer_indices[amf_attribute.StreamIndex];
                uint32 stream_offset = amf_attribute.StreamOffset;
                uint32 stream_stride = vertex_buffer_strides[amf_attribute.StreamIndex];

                int32 data_type;
                int32 comp_type;
                bool normalized;
                String attr_name = "_INVALID_ATTRIBUTE";
                Buffer attribute_data = Buffer();
                auto vertex_buffer = Buffer(all_vertex_buffer[stream_index].Data);
                auto raw_vertex_buffer_data = vertex_buffer.view(
                    vertex_buffer_offsets[amf_attribute.StreamIndex] + stream_offset, stream_stride * vertex_count);
                float32 bbox_min[3] = {0, 0, 0};
                float32 bbox_max[3] = {0, 0, 0};
                bool use_bbox = false;
                uint32 stride;
                switch (amf_attribute.Usage) {
                    case ADFTypes::AmfUsage::AmfUsage_Position: {
                        data_type = TINYGLTF_TYPE_VEC3;
                        comp_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
                        normalized = false;
                        use_bbox = true;
                        stride = 3 * sizeof(float32);
                        attr_name = "POSITION";
                        auto packing_data = reinterpret_cast<const float *>(amf_attribute.PackingData.data());;
                        attribute_data.resize(vertex_count * stride);
                        auto output_data = attribute_data.writable_view_as<float32>();
                        switch (amf_attribute.Format) {
                            case ADFTypes::AmfFormat::AmfFormat_R16G16B16_SNORM: {
                                for (int i = 0; i < vertex_count; ++i) {
                                    auto input_data = reinterpret_cast<int16 *>(
                                        raw_vertex_buffer_data.data() + i * stream_stride);
                                    float32 x = *packing_data * input_data[0] / 32767.0f;
                                    float32 y = *packing_data * input_data[1] / 32767.0f;
                                    float32 z = *packing_data * input_data[2] / 32767.0f;
                                    // Update min/max
                                    if (i == 0) {
                                        bbox_min[0] = x;
                                        bbox_min[1] = y;
                                        bbox_min[2] = z;
                                        bbox_max[0] = x;
                                        bbox_max[1] = y;
                                        bbox_max[2] = z;
                                    }
                                    else {
                                        if (x < bbox_min[0]) bbox_min[0] = x;
                                        if (y < bbox_min[1]) bbox_min[1] = y;
                                        if (z < bbox_min[2]) bbox_min[2] = z;
                                        if (x > bbox_max[0]) bbox_max[0] = x;
                                        if (y > bbox_max[1]) bbox_max[1] = y;
                                        if (z > bbox_max[2]) bbox_max[2] = z;
                                    }
                                    output_data[i * 3 + 0] = x;
                                    output_data[i * 3 + 1] = y;
                                    output_data[i * 3 + 2] = z;
                                }
                                break;
                            }
                            default: {
                                GLog_Error("Unsupported position attribute format: {}",
                                           to_string(amf_attribute.Format));
                                abort();
                            }
                        }
                        break;
                    }
                    case ADFTypes::AmfUsage::AmfUsage_TextureCoordinate: {
                        data_type = TINYGLTF_TYPE_VEC2;
                        comp_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
                        normalized = false;
                        stride = 2 * sizeof(float32);
                        attr_name = "TEXCOORD_" + std::to_string(uv_count);
                        uv_count++;
                        auto packing_data = reinterpret_cast<const float *>(amf_attribute.PackingData.data());;
                        attribute_data.resize(vertex_count * stride);
                        auto output_data = attribute_data.writable_view_as<float32>();
                        switch (amf_attribute.Format) {
                            case ADFTypes::AmfFormat::AmfFormat_R16G16_SNORM: {
                                for (int i = 0; i < vertex_count; ++i) {
                                    int16 *input_data = reinterpret_cast<int16 *>(
                                        raw_vertex_buffer_data.data() + i * stream_stride);
                                    output_data[i * 2 + 0] = input_data[0] / 32767.0f * packing_data[0];
                                    output_data[i * 2 + 1] = input_data[1] / 32767.0f * packing_data[1];
                                }
                                break;
                            }
                            default: {
                                GLog_Error("Unsupported texcoord attribute format: {}",
                                           to_string(amf_attribute.Format));
                                abort();
                            }
                        }
                        break;
                    }
                    case ADFTypes::AmfUsage::AmfUsage_Normal: {
                        data_type = TINYGLTF_TYPE_VEC3;
                        comp_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
                        normalized = false;
                        stride = 3 * sizeof(float32);
                        attr_name = "NORMAL";

                        attribute_data.resize(vertex_count * stride);
                        auto output_data = attribute_data.writable_view_as<float32>();
                        switch (amf_attribute.Format) {
                            case ADFTypes::AmfFormat::AmfFormat_R32_UNIT_VEC_AS_FLOAT: {
                                for (int i = 0; i < vertex_count; ++i) {
                                    float32 input_data = *reinterpret_cast<float32 *>(
                                        raw_vertex_buffer_data.data() + i * stream_stride);
                                    float32 x = input_data;
                                    float32 y = input_data * (1.0f / 256.f);
                                    float32 z = input_data * (1.0f / (256.f * 256.f));
                                    float32 bogus;
                                    x = -1.f + 2.0f * modff(x, &bogus);
                                    y = -1.f + 2.0f * modff(y, &bogus);
                                    z = -1.f + 2.0f * modff(z, &bogus);

                                    float32 length = sqrtf(x * x + y * y + z * z);
                                    if (length > 0.0f) {
                                        x /= length;
                                        y /= length;
                                        z /= length;
                                    }

                                    output_data[i * 3 + 0] = x;
                                    output_data[i * 3 + 1] = y;
                                    output_data[i * 3 + 2] = z;
                                }
                                break;
                            }
                            default: {
                                GLog_Error("Unsupported normal attribute format: {}",
                                           to_string(amf_attribute.Format));
                                abort();
                            }
                        }
                        break;
                    }
                    case ADFTypes::AmfUsage::AmfUsage_Color: {
                        data_type = TINYGLTF_TYPE_VEC4;
                        comp_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
                        normalized = true;
                        stride = 4 * sizeof(uint8);
                        attr_name = "_COLOR_0";
                        attribute_data.resize(vertex_count * stride);
                        auto output_data = attribute_data.writable_view_as<uint8>();
                        switch (amf_attribute.Format) {
                            case ADFTypes::AmfFormat::AmfFormat_R32_R8G8B8A8_UNORM_AS_FLOAT: {
                                for (int i = 0; i < vertex_count; ++i) {
                                    float32 vertex_data = *reinterpret_cast<int16 *>(
                                        raw_vertex_buffer_data.data() + i * stream_stride);
                                    float32 r = vertex_data;
                                    float32 g = vertex_data * (1.0f / 256.f);
                                    float32 b = vertex_data * (1.0f / 256.f * 256.f);
                                    float32 a = vertex_data * (1.0f / 256.f * 256.f * 256.f);
                                    float32 bogus;
                                    r = modff(r, &bogus);
                                    g = modff(g, &bogus);
                                    b = modff(b, &bogus);
                                    a = modff(a, &bogus);

                                    output_data[i * 4 + 0] = static_cast<uint8>(r * 255.f);
                                    output_data[i * 4 + 1] = static_cast<uint8>(g * 255.f);
                                    output_data[i * 4 + 2] = static_cast<uint8>(b * 255.f);
                                    output_data[i * 4 + 3] = static_cast<uint8>(a * 255.f);
                                }
                                break;
                            }
                            default: {
                                GLog_Error("Unsupported color attribute format: {}",
                                           to_string(amf_attribute.Format));
                                abort();
                            }
                        }
                        break;
                    }
                    case ADFTypes::AmfUsage::AmfUsage_BoneIndex: {
                        data_type = TINYGLTF_TYPE_VEC4;
                        comp_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
                        normalized = false;
                        stride = 4 * sizeof(uint16);
                        attr_name = "JOINTS_0";
                        attribute_data.resize(vertex_count * stride);
                        auto output_data = attribute_data.writable_view_as<uint16>();
                        switch (amf_attribute.Format) {
                            case ADFTypes::AmfFormat::AmfFormat_R8G8B8A8_UINT: {
                                for (int i = 0; i < vertex_count; ++i) {
                                    uint8 *input_data = raw_vertex_buffer_data.data() + i * stream_stride;
                                    for (int j = 0; j < 4; ++j) {
                                        int16 bone_index = bone_lookup[input_data[j]];
                                        output_data[i * 4 + j] = bone_index;
                                    }
                                }
                                break;
                            }
                            default: {
                                GLog_Error("Unsupported bone index attribute format: {}",
                                           to_string(amf_attribute.Format));
                                abort();
                            }
                        }
                        break;
                    }
                    case ADFTypes::AmfUsage::AmfUsage_BoneWeight: {
                        data_type = TINYGLTF_TYPE_VEC4;
                        comp_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
                        normalized = false;
                        stride = 4 * sizeof(float32);
                        attr_name = "WEIGHTS_0";
                        attribute_data.resize(vertex_count * stride);
                        auto output_data = attribute_data.writable_view_as<float32>();
                        switch (amf_attribute.Format) {
                            case ADFTypes::AmfFormat::AmfFormat_R8G8B8A8_UNORM: {
                                for (int i = 0; i < vertex_count; ++i) {
                                    uint8 *input_data = raw_vertex_buffer_data.data() + i * stream_stride;
                                    float32 weight_sum = 0.0f;
                                    for (int j = 0; j < 4; ++j) {
                                        output_data[i * 4 + j] = static_cast<float32>(input_data[j]) / 255.0f;
                                        weight_sum += output_data[i * 4 + j];
                                    }
                                    if (weight_sum > 0.0f) {
                                        for (int j = 0; j < 4; ++j) {
                                            output_data[i * 4 + j] /= weight_sum;
                                        }
                                    }
                                }
                                break;
                            }
                            case ADFTypes::AmfFormat::AmfFormat_R32G32B32A32_FLOAT: {
                                for (int i = 0; i < vertex_count; ++i) {
                                    auto input_data = reinterpret_cast<float32 *>(
                                        raw_vertex_buffer_data.data() + i * stream_stride);
                                    float32 weight_sum = 0.0f;
                                    for (int j = 0; j < 4; ++j) {
                                        output_data[i * 4 + j] = input_data[j];
                                        weight_sum += input_data[j];
                                    }
                                    // Normalize weights
                                    if (weight_sum > 0.0f) {
                                        for (int j = 0; j < 4; ++j) {
                                            output_data[i * 4 + j] /= weight_sum;
                                        }
                                    }
                                }
                                break;
                            }
                            default: {
                                throw std::runtime_error(std::format("Unsupported bone weight attribute format:{}",
                                                                     to_string(amf_attribute.Format)));
                            }
                        }
                        break;
                    }
                    default: {
                        // GLog_Warning("Unsupported attribute usage: {}", std::to_underlying(amf_attribute.Usage));
                        continue;
                    }
                }

                auto attribute_accessor = helper.set_primitive_attribute_from_u8(
                    gl_mesh.index(), sub_mesh_id, attr_name,
                    attribute_data.data(), attribute_data.size(), comp_type, data_type, vertex_count, normalized,
                    stride, 0, std::format("{}_{}", mesh_name,attr_name));

                auto &attribute = helper.model().accessors[attribute_accessor.index()];
                if (use_bbox) {
                    attribute.minValues = {bbox_min[0], bbox_min[1], bbox_min[2]};
                    attribute.maxValues = {bbox_max[0], bbox_max[1], bbox_max[2]};
                }
            }
        }
        if (const auto constants = ADF::as<ADFTypes::GeneralMeshConstants>(mesh.MeshProperties)) {
            if (constants->IsSkinnedMesh) {
                const auto skin = helper.current_skin();
                if (skin.is_valid()) {
                    node->skin = skin.index();
                }
            }
        }
    }
}

GltfHelper::Handle<tinygltf::Node> export_amf_mesh(AppState &app_state, uint32 path_hash,
                                                   const ADFTypes::AmfMeshHeader *header,
                                                   const ADFTypes::AmfMeshBuffers *mesh_buffers) {
    ZoneScoped
    auto &helper = app_state.helper();

    std::string mesh_name = find_name(path_hash).value_or(std::format("mesh_{:08X}", path_hash));
    auto mesh_root_node = helper.make<tinygltf::Node>();
    mesh_root_node->name = mesh_name;

    std::vector<ADFTypes::AmfBuffer> all_vertex_buffer = {};
    all_vertex_buffer.reserve(mesh_buffers->VertexBuffers.size());

    std::vector<ADFTypes::AmfBuffer> all_index_buffer = {};
    all_index_buffer.reserve(mesh_buffers->IndexBuffers.size());

    for (const auto &buffer: mesh_buffers->VertexBuffers) {
        all_vertex_buffer.emplace_back(buffer);
    }

    for (const auto &amf_buffer: mesh_buffers->IndexBuffers) {
        all_index_buffer.emplace_back(amf_buffer);
    }


    // hires fix
    if (const auto hi_res_path_full_tmp = find_name(header->HighLodPath)) {
        const auto hi_res_path_full = hi_res_path_full_tmp.value();
        if (hi_res_path_full.contains("intermediate/")) {
            auto hi_res_path = hi_res_path_full.substr(strlen("intermediate/"));

            if (auto hi_res_buffer = app_state.manager().get_file(hash_string(hi_res_path))) {
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

    // for (const auto [lod_id, lod_group]: header->LodGroups | std::views::enumerate) {
    //     export_amf_lod(helper, mesh_name, mesh_root_node, lod_group, lod_id, all_index_buffer, all_vertex_buffer);
    // }

    return mesh_root_node;
}

GltfHelper::Handle<tinygltf::Node> export_amf_model(AppState &app_state, const ADFTypes::AmfModel *amf_model,
                                                    const uint32 path_hash) {
    ZoneScoped
    auto &helper = app_state.helper();
    auto model_root_node = helper.make<tinygltf::Node>();
    const std::filesystem::path model_path = find_name(path_hash).value_or(std::format("model_{:08X}", path_hash));
    model_root_node->name = model_path.filename().string();

    for (const auto &amf_material: amf_model->Materials) {
        std::string material_name = find_name(amf_material.Name).value_or(
            std::format("material_{:08X}", amf_material.Name.storage));

        auto material = helper.find<tinygltf::Material>(material_name);

        if (!material) {
            auto new_material = helper.make<tinygltf::Material>();
            new_material->name = material_name;

            std::string render_block_id = find_name(amf_material.RenderBlockId).value_or(
                std::format("renderblock_{:08X}", amf_material.RenderBlockId.storage));

            GLog_Info("Material {} -> {}", material_name, render_block_id);
            for (const auto [tex_id, texture]: amf_material.Textures | std::views::enumerate) {
                auto tex_path = find_name(texture.storage);
                if (!tex_path) {
                    continue;
                }
                GLog_Info("\tSlot {} -> {}", tex_id, tex_path.value());
            }

            if (render_block_id == "GeneralR2") {
                if (!app_state.skip_textures) {
                    const auto constants = ADF::as<ADFTypes::GeneralR2Constants>(amf_material.Attributes);
                    if (!constants) {
                        const auto &type = typeid(*constants);
                        GLog_Warning("Unsupported GeneralR2 material attribute type: {}", type.name());
                        continue;
                    }
                    new_material->pbrMetallicRoughness.baseColorFactor[0] = 1.0f;
                    new_material->pbrMetallicRoughness.baseColorFactor[1] = 1.0f;
                    new_material->pbrMetallicRoughness.baseColorFactor[2] = 1.0f;
                    new_material->pbrMetallicRoughness.baseColorFactor[3] = 1.0f;
                    new_material->pbrMetallicRoughness.metallicFactor = 1.0f;
                    new_material->pbrMetallicRoughness.roughnessFactor = 1.0f;

                    if (amf_material.Textures.size() >= 3) {
                        std::unique_ptr<Texture> diffuse_texture = nullptr;

                        if (const auto diffuse_path = find_name(amf_material.Textures[0])) {
                            diffuse_texture = convert_ddsc(app_state, amf_material.Textures[0]);
                            auto image = helper.create_texture_png_data(
                                diffuse_texture->save_to_memory(MemoryFormat::PNG),
                                std::format("{}_{:08X}.png", path_utils::filename(diffuse_path.value()),
                                            hash_string(diffuse_path.value())));
                            new_material->pbrMetallicRoughness.baseColorTexture.index = image.index();
                        }
                        if (auto normal_path = find_name(amf_material.Textures[1])) {
                            auto normal_texture = convert_ddsc(app_state, amf_material.Textures[1]);
                            auto image = helper.create_texture_png_data(
                                normal_texture->save_to_memory(MemoryFormat::PNG),
                                std::format("{}_{:08X}.png", path_utils::filename(normal_path.value()),
                                            hash_string(normal_path.value())));
                            new_material->normalTexture.index = image.index();
                            new_material->normalTexture.scale = 1.0f;
                        }

                        if (auto orm_path = find_name(amf_material.Textures[2])) {
                            auto orm_texture = convert_ddsc(app_state, amf_material.Textures[2]);
                            auto image = helper.create_texture_png_data(
                                orm_texture->save_to_memory(MemoryFormat::PNG),
                                std::format("{}_{:08X}.png", path_utils::filename(orm_path.value()),
                                            hash_string(orm_path.value())));
                            new_material->pbrMetallicRoughness.metallicRoughnessTexture.index = image.index();
                        }
                        if (constants->UseEmissive) {
                            if (auto emission_path = find_name(amf_material.Textures[4])) {
                                const uint32 hash = hash_string(emission_path.value());
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
                                auto image = helper.create_texture_png_data(
                                    emission_texture->save_to_memory(MemoryFormat::PNG), texture_name);
                                new_material->emissiveTexture.index = image.index();
                                new_material->emissiveFactor[0] = 1.0f;
                                new_material->emissiveFactor[1] = 1.0f;
                                new_material->emissiveFactor[2] = 1.0f;
                            }
                        }

                        // if (constants->UseAlbedoDetail) {
                        //     StringView albedo_detail_path = find_name(amf_material.Textures.items[5]);
                        //     Texture *albedo_detail = convert_ddsc(app_state, amf_material.Textures.items[5]);
                        //     String *texture_save_path = GLTFContext_data_path(context);
                        //     const uint32 hash = hash_vstring(albedo_detail_path);
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
                        //     const uint32 hash = hash_vstring(normal_detail_path);
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
            }
            else {
                GLog_Warning("Unsupported material render block: 0x{:08X}", amf_material.RenderBlockId.storage);
                continue;
            }
        }
    }


    auto mb = app_state.manager().get_file(amf_model->Mesh);
    if (!mb) {
        GLog_Error("File not found");
        return model_root_node;
    }

    const auto mesh_root_node = export_adf_file_from_buffer(app_state, amf_model->Mesh, std::move(mb));
    helper.set_parent(model_root_node, mesh_root_node);

    return model_root_node;
}
