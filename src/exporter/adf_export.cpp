// Created by RED on 12.01.2026.

#include "apex/adf/adf.h"
#include "apex/adf/generated/adf_types.h"
#include "apex/adf/sti_shared.h"
#include "apex/hashes.h"
#include "exporter/adf_export.h"
#include "exporter/amf_export.h"
#include "redscore/platform/logger.h"
#include "redscore/platform/texture/texture.h"
#include "redscore/utils/simple_fileio.h"
#include "utils/hash_helper.h"

#include "zstd.h"
#include "redscore/gltf/tiny_gltf.h"
#include "tracy/Tracy.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/glm.hpp"

#pragma pack(push, 1)

struct VertexID_UV {
    uint32 vertex_id;
    int16 UV[2];
};


static_assert(sizeof(VertexID_UV) == 8, "VertexID_UV size mismatch");

struct VertexPosNorm {
    uint16 pos[4];
    int8 norm[3];
    uint8 mask_d;
};

static_assert(sizeof(VertexPosNorm) == 12, "VertexPosNorm size mismatch");
#pragma pack(pop)

using namespace ADFTypes;

std::optional<IO::Buffer> decompress_data(const CompressedData &data) {
    ZoneScoped
    IO::Buffer decompressed_data(data.UncompressedSize);

    const auto compressed_header = reinterpret_cast<const CompressedHeader *>(data.Data.data());
    const uint8 *compressed_data = data.Data.data() + sizeof(CompressedHeader);

    switch (compressed_header->comp_type) {
        case CompType::zstd: {
            const size_t zstd_res = ZSTD_decompress(decompressed_data.data(), decompressed_data.size(),
                                                    compressed_data, data.Data.size() - sizeof(CompressedHeader));
            const auto error = static_cast<ZSTD_ErrorCode>(ZSTD_isError(zstd_res));
            if (error != ZSTD_error_no_error) {
                GLog_Error("ZSTD decompression error: {}", ZSTD_getErrorName(error));
                return {};
            }
            assert(zstd_res == compressed_header->decomp_size && "ZSTD decompression size mismatch");
        }
        break;
        default:
            GLog_Error("Unsupported compression type: {}", compressed_header->comp_type);
            return {};
    }

    return std::move(decompressed_data);
}

std::unique_ptr<Texture> export_terrain_texture(const TerrainTexture &terrain_texture, uint32 expected_channels) {
    ZoneScoped
    if (terrain_texture.Width == 0 || terrain_texture.Height == 0 || terrain_texture.Data.UncompressedSize == 0) {
        return nullptr;
    }

    const float32 pixel_size = static_cast<float32>(terrain_texture.Data.UncompressedSize) /
                               (static_cast<float32>(terrain_texture.Width) * terrain_texture.Height);


    const auto decompressed_data = decompress_data(terrain_texture.Data);
    if (!decompressed_data) {
        GLog_Error("Failed to decompress terrain texture");
        return nullptr;
    }

    auto fmt = DDSDXGIFormat::DXGI_FORMAT_B8G8R8A8_UNORM;

    switch (terrain_texture.BlockCompressionType) {
        case BlockCompressionType::E_BLOCKCOMPRESSIONTYPE_BC7:
            fmt = DDSDXGIFormat::DXGI_FORMAT_BC7_UNORM;
            break;
        case BlockCompressionType::E_BLOCKCOMPRESSIONTYPE_BC3:
            fmt = DDSDXGIFormat::DXGI_FORMAT_BC3_UNORM;
            break;
        case BlockCompressionType::E_BLOCKCOMPRESSIONTYPE_NONE: {
            if (expected_channels == 1 && pixel_size == 1) {
                fmt = DDSDXGIFormat::DXGI_FORMAT_R8_UNORM;
            } else if (expected_channels == 1 && pixel_size == 2) {
                fmt = DDSDXGIFormat::DXGI_FORMAT_R16_UNORM;
            } else if (expected_channels == 2 && pixel_size == 4) {
                fmt = DDSDXGIFormat::DXGI_FORMAT_R16G16_UNORM;
            } else if (expected_channels == 3 && pixel_size == 3) {
                fmt = DDSDXGIFormat::DXGI_FORMAT_CUSTOM_R8G8B8_UNORM;
            } else if (expected_channels == 4 && pixel_size == 4) {
                fmt = DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_UNORM;
            } else {
                GLog_Error("Unsupported terrain texture format with %i channels and pixel size %f", expected_channels,
                           pixel_size);
                return nullptr;
            }
            break;
        }
    }

    return std::move(std::make_unique<Texture>(
        Texture::from_dxgi(fmt, decompressed_data->as_span(), terrain_texture.Width, terrain_texture.Height, 1)));
}

GltfHelper::Handle<tinygltf::Node> export_adf_file(ApexAppState &app_state, const uint32 path_hash) {
    ZoneScoped
    auto result = app_state.manager().get(path_hash);

    if (!result) {
        GLog_Error("File not found\n");
        return {};
    }

    return export_adf_file_from_buffer(app_state, path_hash, std::move(result));
}

GltfHelper::Handle<tinygltf::Node> export_terrain_patch(ApexAppState &app_state, const StreamPatchBlockHeader *header,
                                                        const TerrainPatch *terrain_patch) {
    ZoneScoped
    const uint32 patch_x_pos = header->PatchPositionX;
    const uint32 patch_z_pos = header->PatchPositionZ;
    const TerrainMesh *terrain_mesh = &terrain_patch->TerrainMesh;

    auto vertices1_buffer = decompress_data(terrain_mesh->Vertices);
    if (!vertices1_buffer) {
        GLog_Error("Failed to decompress terrain texture");
        return {};
    }
    auto vertices2_buffer = decompress_data(terrain_mesh->Vertices2);
    if (!vertices2_buffer) {
        GLog_Error("Failed to decompress terrain texture");
        return {};
    }

    auto indices_buffer = decompress_data(terrain_mesh->Indices);
    if (!indices_buffer) {
        GLog_Error("Failed to decompress terrain texture");
        return {};
    }

    std::string patch_name = std::format("terrain_patch_{}_{}_lod_{}", patch_x_pos, patch_z_pos, header->PatchLod);


    assert(vertices1_buffer->size()%8==0);
    assert(vertices2_buffer->size()%12==0);
    assert(indices_buffer->size()%6==0);
    const uint32 vertex_count = vertices1_buffer->size() / sizeof(VertexID_UV);

    std::vector<glm::vec3> positions(vertex_count);
    std::vector<glm::vec3> normals(vertex_count);
    std::vector<glm::vec2> uv(vertex_count);
    std::vector<uint32> indices(indices_buffer->size() / 2);

    // vec3 bbox_min = {};
    // vec3 bbox_max = {};

    int16 uv_dims[2] = {};
    uv_dims[0] = (int16) terrain_patch->TerrainDisplacementTexture.Width;
    uv_dims[1] = (int16) terrain_patch->TerrainDisplacementTexture.Height;
    if (terrain_patch->DisplacementDownsampled) {
        uv_dims[0] *= 2;
        uv_dims[1] *= 2;
    }

    auto id_uv = (vertices1_buffer->readonly_view_as<VertexID_UV>());
    auto pos_norm = (vertices2_buffer->readonly_view_as<VertexPosNorm>());

    for (int i = 0; i < vertex_count; ++i) {
        uint32 vert_id = id_uv[i].vertex_id;
        const uint16 *pos = pos_norm[vert_id].pos;
        const int8 *norm = pos_norm[vert_id].norm;

        float32 x = ((float32) (pos[0] - 8192) / 32768.f);
        float32 y = ((float32) (pos[1] - 8192) / 32768.f) * 10.f;
        float32 z = ((float32) (pos[2] - 8192) / 32768.f);
        positions[i] = {x * 200.f, y * 200.f, z * 200.f};
        float32 nx = ((float32) (norm[0]) / 127.f);
        float32 ny = ((float32) (norm[1]) / 127.f);
        float32 nz = ((float32) (norm[2]) / 127.f);
        normals[i] = {nx, ny, nz};

        const float32 half_pixel_x = 1.0f / (float32) uv_dims[0];
        const float32 half_pixel_y = 1.0f / (float32) uv_dims[1];

        uv.emplace_back(
            (float32) id_uv[i].UV[0] / (float32) uv_dims[0] + half_pixel_x,
            (float32) id_uv[i].UV[1] / (float32) uv_dims[1] + half_pixel_y
        );
    }

    std::copy_n(reinterpret_cast<const uint16 *>(indices_buffer->data()), indices_buffer->size() / 2, indices.data());

    auto &gltf_helper = app_state.helper();
    auto mesh = gltf_helper.make<tinygltf::Mesh>();
    mesh->name = patch_name;

    auto &primitive = mesh->primitives.emplace_back();

    gltf_helper.set_primitive_attribute(primitive, "POSITIONS",
                                        reinterpret_cast<const uint8 *>(positions.data()),
                                        positions.size() * sizeof(glm::vec3),
                                        TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3, vertex_count, false,
                                        sizeof(glm::vec3), 0, "POSITIONS"
    );


    gltf_helper.set_primitive_attribute(primitive, "NORMAL",
                                        reinterpret_cast<const uint8 *>(normals.data()),
                                        normals.size() * sizeof(glm::vec3),
                                        TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3, vertex_count, false,
                                        sizeof(glm::vec3), 0, "NORMALS");

    gltf_helper.set_primitive_attribute(primitive, "TEXCOORD_0",
                                        reinterpret_cast<const uint8 *>(uv.data()),
                                        uv.size() * sizeof(glm::vec2),
                                        TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC2, vertex_count, false,
                                        sizeof(glm::vec2), 24, "TEXCOORD_0");

    gltf_helper.set_primitive_indices(primitive, reinterpret_cast<const uint8 *>(indices.data()),
                                      indices.size() * sizeof(uint32), TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
                                      indices.size(), sizeof(uint32), 0, "INDICES");

    auto patch_mesh_node = gltf_helper.make<tinygltf::Node>();
    patch_mesh_node->name = patch_name;
    patch_mesh_node->mesh = mesh.index();

    patch_mesh_node->translation = {patch_x_pos * 200.f, 0.0f, patch_z_pos * 200.f};

    auto material = gltf_helper.make<tinygltf::Material>();
    primitive.material = material.index();


    material->pbrMetallicRoughness.metallicFactor = 0.f;
    material->pbrMetallicRoughness.roughnessFactor = 1.f;


    if (auto displacement_texture = export_terrain_texture(terrain_patch->TerrainDisplacementTexture, 1)) {
        std::string patch_texture_name = std::format("{}_disp", patch_name);

        if (displacement_texture->bpc() == 1) {
            gltf_helper.add_extra_save_data(patch_texture_name,
                                            displacement_texture->save_to_memory(MemoryFormat::PNG));
        } else {
            gltf_helper.add_extra_save_data(patch_texture_name,
                                            displacement_texture->save_to_memory(MemoryFormat::DDS));
        }
    }

    if (auto color_texture = export_terrain_texture(terrain_patch->TerrainColorTexture, 4)) {
        std::string patch_texture_name = std::format("{}_color", patch_name);
        auto texture = gltf_helper.create_image_png_data(color_texture->save_to_memory(MemoryFormat::PNG),
                                                         patch_texture_name);
        material->pbrMetallicRoughness.baseColorTexture.index = texture.index();
    }

    // Texture *normal_texture = export_terrain_texture(&terrain_patch->TerrainNormalTexture, 4, NULL);
    // if (normal_texture != NULL) {
    //     std::string patch_texture_name = {};
    //     String_copy_from(&patch_texture_name, &patch_name);
    //     String_append_cstr(&patch_texture_name, "_normal");
    //     GLTFContext_material_set_normal_from_data(context, &patch_texture_name, material_id, normal_texture);
    //     Texture_free(normal_texture);
    //     String_free(&patch_texture_name);
    // }
    // std::string tmp_name = {}; {
    //     std::string *texture_save_path = GLTFContext_data_path(context);
    //     String_copy_from(&tmp_name, texture_save_path);
    //     Path_join(&tmp_name, &patch_name);
    //     String_append_cstr(&tmp_name, "_duplex");
    //     String_free(texture_save_path);
    //     Path_normalize_native(&tmp_name);
    // }

    // Texture *duplex_texture = export_terrain_texture(&terrain_patch->TerrainMaterialDuplexTexture, 2, &tmp_name);
    // String_free(&tmp_name);
    // if (duplex_texture != NULL) {
    //     std::string patch_texture_name = {};
    //     String_copy_from(&patch_texture_name, &patch_name);
    //     String_append_cstr(&patch_texture_name, "_duplex");
    //     std::string *texture_save_path = GLTFContext_data_path(context);
    //     const uint32 hash = hash_string(&patch_texture_name);
    //     String_append_format(texture_save_path, "/%s_%08X", String_data(&patch_texture_name), hash);
    //     String_free(&patch_texture_name);
    //     Texture_save(duplex_texture, texture_save_path);
    //     String_free(texture_save_path);
    //     Texture_free(duplex_texture);
    //     // std::string patch_texture_name = {};
    //     // String_copy_from(&patch_texture_name, &patch_name);
    //     // String_append_cstr(&patch_texture_name, "_duplex");
    //     // GLTFContext_material_set_roughness_metallic_from_data(context, &patch_texture_name, material_id,
    //     //                                                       duplex_texture);
    // }
    return patch_mesh_node;
}

void export_terrain_instances(ApexAppState &app_state,
                              const StreamPatchBlockHeader &header,
                              const InstanceDataPatch &instance_data_patch
) {
    ZoneScoped
    const uint32 patch_x_pos = header.PatchPositionX;
    const uint32 patch_z_pos = header.PatchPositionZ;
    const uint32 patch_size = 1 << header.PatchLod;

    for (const auto &instance_layer: instance_data_patch.InstanceDataLayers) {
        const auto layer_name = find_name(instance_layer.Name).value_or(
            std::format("layer_0x{:08X}", instance_layer.Name.storage));
        for (int j = 0; j < instance_layer.Instances.size(); ++j) {
            const VegetationSystemInstance &veg_instance = instance_layer.Instances[j];
            std::string instance_name = std::format("instance_{}_{}", layer_name, j);
            const auto instance_node = app_state.helper().make<tinygltf::Node>();
            instance_node->translation = {
                patch_x_pos * 50 + static_cast<float32>(veg_instance.X) / patch_size * 0.93f / 8.f,
                (veg_instance.Y / patch_size * 0.93f / 32.f),
                patch_z_pos * 50 + static_cast<float32>(veg_instance.Z) / patch_size * 0.93f / 8.f
            };
        }
    }
}

GltfHelper::Handle<tinygltf::Node> export_stream_patch_file(ApexAppState &app_state, ADF::ADFFile &adf) {
    ZoneScoped
    const auto patch_file_header = convert<StreamPatchFileHeader>(adf.read_instance(0));
    for (int i = 1; i < adf.instances().size(); ++i) {
        const auto patch_instance = adf.read_instance(i);

        if (const StreamPatchBlockHeader *block_header = as<StreamPatchBlockHeader>(patch_instance)) {
            const auto block_data = adf.read_instance(i + 1);
            if (const auto *terrain_patch = as<TerrainPatch>(block_data)) {
                const auto node = export_terrain_patch(app_state, block_header, terrain_patch);
                if (node.is_valid()) {
                    app_state.helper().add_to_scene(node);
                }
                // } else if (block_data_instance->type_hash == STI_TYPE_HASH_InstanceDataPatch) {
                //     const InstanceDataPatch *instance_data_patch = (InstanceDataPatch *) block_data;
                //     export_terrain_instances(context, archive_manager, block_header, instance_data_patch, export_path);
                // } else if (const auto *instance_patch = as<InstanceDataPatch>(block_data)) {
                // } else if (const auto *world_audio_patch = as<WorldAudioPatchData>(block_data)) {
                // } else if (const auto *world_audio_normal_patch = as<WorldAudioPatchNormalData>(block_data)) {
            } else {
                const auto &ti = typeid(block_data.get());
                GLog_Warning("Unsupported block type: {}", ti.name());
                // throw std::runtime_error("Unsupported block type: " + std::string(ti.name()));
            }
            i++;
        }
    }
    return {};
}

GltfHelper::Handle<tinygltf::Node> export_adf_file_from_buffer(ApexAppState &app_state, const uint32 path_hash,
                                                               std::unique_ptr<IO::File> mb) {
    ZoneScoped
    ADF::ADFFile adf = ADF::ADFFile::from_buffer(std::move(mb));

    const auto instances = adf.instances();

    for (int instanceId = 0; instanceId < instances.size(); instanceId++) {
        const auto &instance = instances[instanceId];
        if (instance.type_hash == std::to_underlying(ADFHashes::WorldSettings)) {
            const auto world_settings = adf.read_instance<WorldSettings>(instanceId);
            const uint32 base_lod = world_settings->PatchBaseLod;
            // const uint32 base_lod = 9;
            const uint32 lod_size = 1 << base_lod;
            const uint32 world_x_size_in_chunks = world_settings->WorldSize[0] / lod_size;
            const uint32 world_y_size_in_chunks = world_settings->WorldSize[2] / lod_size;
            for (int x = 0; x < world_x_size_in_chunks; ++x) {
                for (int y = 0; y < world_y_size_in_chunks; ++y) {
                    GLog_Info("Exporting tile %02ix%02i at LOD %i", x, y, base_lod);
                    std::string chunk_patch_path = std::format(
                        "terrain/hp/patches/patch_{:02}_{:02}_{:02}.streampatch", base_lod, x, y);
                    export_adf_file(app_state, hash_string(chunk_patch_path));
                    // break;
                }
                // break;
            }
        } else if (instance.type_hash == std::to_underlying(ADFHashes::StreamPatchFileHeader)) {
            return export_stream_patch_file(app_state, adf);
            // }
            // if (instance->type_hash == STI_TYPE_HASH_StreamPatchFileHeader) {
            //     const StreamPatchFileHeader *ph = instance_data;
            //     tile_x = ph->PatchPositionX;
            //     tile_y = ph->PatchPositionZ;
            //     lod = ph->PatchLod;
            // } else if (instance->type_hash == STI_TYPE_HASH_TerrainPatch) {
            //     export_terrain_patch(&mesh_export_path, instance_data, tile_x, tile_y, lod);
        } else if (instance.type_hash == std::to_underlying(ADFHashes::AmfModel)) {
            if (instances.size() != 1) {
                throw std::runtime_error("ADF with AmfModel should have only one instance");
            }
            const auto model = adf.read_instance<AmfModel>(instanceId);
            return export_amf_model(app_state, model.get(), path_hash);
        } else if (instance.type_hash == std::to_underlying(ADFHashes::AmfMeshHeader)) {
            if (instances.size() != 2) {
                throw std::runtime_error("ADF with AmfMeshHeader should have only two instances");
            }
            const auto mesh_header = adf.read_instance<AmfMeshHeader>(instanceId);
            instanceId++;
            const auto mesh_buffers = adf.read_instance<AmfMeshBuffers>(instanceId);

            return export_amf_mesh(app_state, path_hash, mesh_header.get(), mesh_buffers.get());
        } else if (instance.type_hash == std::to_underlying(ADFHashes::StringLookup)) {
            if (instances.size() != 1) {
                throw std::runtime_error("ADF with StringLookup should have only one instance");
            }
            const auto string_lookup = adf.read_instance<StringLookup>(instanceId);
            const char *string_data = reinterpret_cast<const char *>(string_lookup->Text.data());

            std::unordered_map<u32, std::string> string_map;

            nlohmann::json root;
            auto &sorted_pairs = root["SortedPairs"];
            for (const auto &sorted_pair: string_lookup->SortedPairs) {
                string_map[sorted_pair.Hash] = std::string_view(string_data + sorted_pair.TextOffset);

                sorted_pairs.emplace_back(nlohmann::json{
                    {"Hash", sorted_pair.Hash},
                    {"Name", std::string_view(string_data + sorted_pair.NameOffset)},
                    {"Text", std::string_view(string_data + sorted_pair.TextOffset)}
                });
            }
            auto &sorted_dialogue_lines = root["SortedDialogueLines"];
            for (const auto &sorted_dialogue_line: string_lookup->SortedDialogueLines) {
                nlohmann::json subtitles;
                for (const auto & subtitle : sorted_dialogue_line.Subtitles) {
                    std::string subtitle_line = "<!!LINE NOT FOUND!!>";
                    if (string_map.contains(subtitle.LineHash)) {
                        subtitle_line = string_map.at(subtitle.LineHash);
                    }
                    subtitles.emplace_back(nlohmann::json{
                        {"LineHash", subtitle.LineHash},
                        {"Line", subtitle_line},
                        {"Start", subtitle.Start},
                        {"Duration", subtitle.Duration},
                    });
                }

                sorted_dialogue_lines.emplace_back(nlohmann::json{
                    {"Hash", sorted_dialogue_line.Hash},
                    {"Name", std::string_view(string_data + sorted_dialogue_line.NameOffset)},
                    {"Subtitles", subtitles},
                    {"FMODEvent", sorted_dialogue_line.FMODEvent},
                    {"IncomingCall", sorted_dialogue_line.IncomingCall.to_json()},
                    {"CharacterName", sorted_dialogue_line.IncomingCall.to_json()},
                    {"Flags", sorted_dialogue_line.Flags},

                });
            }

            auto path = find_name(path_hash).value_or(std::format("unknown_{:08X}", path_hash));
            path += std::format("_{:08X}", instance.type_hash);
            std::filesystem::path unk_file_export_path = app_state.export_path() / path;
            std::filesystem::create_directories(unk_file_export_path.parent_path());

            unk_file_export_path.replace_extension("json");
            std::ofstream json_out(unk_file_export_path);
            json_out << root.dump(2);
            json_out.close();

        } else {
            // const auto instance_obj = adf.read_instance(instanceId);
            auto path = find_name(path_hash).value_or(std::format("unknown_{:08X}", path_hash));
            path += std::format("_{:08X}", instance.type_hash);
            std::filesystem::path unk_file_export_path = app_state.export_path() / path;
            std::filesystem::create_directories(unk_file_export_path.parent_path());
            auto data = adf.get_instance_data(instanceId);
            write_file(unk_file_export_path, data);

            unk_file_export_path.replace_extension("json");
            std::ofstream json_out(unk_file_export_path);
            const auto json_data = adf.read_instance<ADF::BaseType>(instanceId);
            json_out << json_data->to_json().dump(2);
            json_out.close();
        }
    }

    return {};
}
