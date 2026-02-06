// Created by RED on 12.01.2026.

#include "exporter/adf_export.h"

#include "zstd.h"
#include "cglm/cglm.h"

#include "apex/adf/adf.h"
#include "apex/adf/adf_types.h"
#include "apex/hashes.h"
#include "exporter/amf_export.h"
#include "platform/logger.h"
#include "utils/buffer/file_buffer.h"
#include "utils/hash_helper.h"
#include "utils/path.h"
#include "utils/stb_image_write.h"


#pragma pack(push, 1)
typedef struct {
    uint32 vertex_id;
    int16 UV[2];
} VertexID_UV;

static_assert(sizeof(VertexID_UV) == 8, "VertexID_UV size mismatch");

typedef struct {
    uint16 pos[4];
    uint8 mask_a;
    int8 curvature;
    uint8 mask_c;
    uint8 mask_d;
} VertexPosNorm;

static_assert(sizeof(VertexPosNorm) == 12, "VertexPosNorm size mismatch");
#pragma pack(pop)


bool decompress_data(const CompressedData *data, MemoryBuffer *mb) {
    MemoryBuffer_allocate(mb, data->UncompressedSize);
    mb->size = data->UncompressedSize;

    const CompressedHeader *compressed_header = (CompressedHeader *) data->Data.items;
    const uint8 *compressed_data = (data->Data.items) + sizeof(CompressedHeader);

    switch (compressed_header->comp_type) {
        case zstd: {
            const size_t zstd_res = ZSTD_decompress(mb->data, mb->size, compressed_data,
                                                    data->Data.count - sizeof(CompressedHeader));
            const ZSTD_ErrorCode error = ZSTD_isError(zstd_res);
            if (error != ZSTD_error_no_error) {
                GLog_Error("ZSTD decompression error: %s", ZSTD_getErrorName(error));
                // assert(false && "ZSTD decompression failed");
                return false;
            }
            assert(zstd_res == compressed_header->decomp_size && "ZSTD decompression size mismatch");
        }
        break;
        default:
            GLog_Error("Unsupported compression type: %d", compressed_header->comp_type);
            // assert(false && "Unsupported compression type");
            return false;
    }

    return true;
}

Texture *export_terrain_texture(const TerrainTexture *terrain_texture, uint32 expected_channels,
                                const String *dump_path) {
    if (terrain_texture->Width == 0 || terrain_texture->Height == 0 || terrain_texture->Data.UncompressedSize == 0) {
        return NULL;
    }

    const float32 pixel_size = (float32) terrain_texture->Data.UncompressedSize / (
                                   (float32) terrain_texture->Width * terrain_texture->Height);
    // GLog_Info("width: %i, height: %i, size: %i, pixel size: %f",
    //           terrain_texture->Width,
    //           terrain_texture->Height, terrain_texture->Data.UncompressedSize,
    //           pixel_size);


    MemoryBuffer decompressed_buffer = {0};
    if (!decompress_data(&terrain_texture->Data, &decompressed_buffer)) {
        GLog_Error("Failed to decompress terrain texture");
        return NULL;
    }

    // if (dump_path != NULL) {
    //     String texture_path = {0};
    //     String_copy_from(&texture_path, dump_path);
    //     Path_ensure_parent_dirs(&texture_path);
    //     String_append_cstr(&texture_path, ".raw");
    //     FileBuffer file_buffer = {0};
    //     if (FileBuffer_open_write(&file_buffer, String_data(&texture_path)) == BUFFER_SUCCESS) {
    //         uint32 written = 0;
    //         file_buffer.write(&file_buffer, decompressed_buffer.data, decompressed_buffer.size, &written);
    //         file_buffer.close(&file_buffer);
    //     } else {
    //         GLog_Error("Failed to open file for writing: %s", String_data(&texture_path));
    //     }
    //     String_free(&texture_path);
    // }

    Texture *texture = Texture_new();
    DDSDXGIFormat fmt = DXGI_FORMAT_B8G8R8A8_UNORM;
    if (terrain_texture->BlockCompressionType == E_BLOCKCOMPRESSIONTYPE_NONE) {
        if (expected_channels == 1 && pixel_size == 1) {
            fmt = DXGI_FORMAT_R8_UNORM;
        }
        else if (expected_channels == 1 && pixel_size == 2) {
            fmt = DXGI_FORMAT_R16_UNORM;
        }
        else if (expected_channels == 2 && pixel_size == 4) {
            fmt = DXGI_FORMAT_R16G16_UNORM;
        }
        else if (expected_channels == 3 && pixel_size == 3) {
            fmt = DXGI_FORMAT_CUSTOM_R8G8B8_UNORM;
        }
        else if (expected_channels == 4 && pixel_size == 4) {
            fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
        }
        else {
            GLog_Error("Unsupported terrain texture format with %i channels and pixel size %f", expected_channels,
                       pixel_size);
            return NULL;
        }
    }

    switch (terrain_texture->BlockCompressionType) {
        case E_BLOCKCOMPRESSIONTYPE_BC7:
            fmt = DXGI_FORMAT_BC7_UNORM;
            break;
        case E_BLOCKCOMPRESSIONTYPE_BC3:
            fmt = DXGI_FORMAT_BC3_UNORM;
            break;
        case E_BLOCKCOMPRESSIONTYPE_NONE:
            break;
    }


    Texture_from_dxgi(texture, fmt, terrain_texture->Width, terrain_texture->Height, 1, decompressed_buffer.data,
                      decompressed_buffer.size);
    decompressed_buffer.close(&decompressed_buffer);
    return texture;
}

GL_ID export_adf_file(AppState *app_state,
                      const String *path, const uint32 path_hash) {
    if (!ArchiveManager_has_file_by_hash(&app_state->archive_manager, path_hash)) {
        GLog_error("File not found\n");
        return INVALID_GL_ID;
    }

    MemoryBuffer mb = {0};
    if (!ArchiveManager_get_file_by_hash(&app_state->archive_manager, path_hash, &mb)) {
        GLog_error("File not found\n");
        return INVALID_GL_ID;
    }

    const GL_ID result = export_adf_file_from_buffer(app_state, path_hash, path, &mb);
    mb.close(&mb);
    return result;
}

void export_terrain_patch(AppState *app_state,
                          const StreamPatchBlockHeader *header, const TerrainPatch *terrain_patch) {
    GLTFContext* context = &app_state->gltf_context;
    const uint32 patch_x_pos = header->PatchPositionX;
    const uint32 patch_z_pos = header->PatchPositionZ;
    const TerrainMesh *terrain_mesh = &terrain_patch->TerrainMesh;

    MemoryBuffer vertices1_buffer = {0};
    if (!decompress_data(&terrain_mesh->Vertices, &vertices1_buffer)){
        GLog_Error("Failed to decompress terrain texture");
        return;
    }
    MemoryBuffer vertices2_buffer = {0};
    if (!decompress_data(&terrain_mesh->Vertices2, &vertices2_buffer)){
        GLog_Error("Failed to decompress terrain texture");
        return;
    }

    MemoryBuffer indices_buffer = {0};
    if (!decompress_data(&terrain_mesh->Indices, &indices_buffer)){
        GLog_Error("Failed to decompress terrain texture");
        return;
    }

    String patch_name = {0};
    String_format(&patch_name, "terrain_patch_%02i_%02i_lod_%02i", patch_x_pos, patch_z_pos, header->PatchLod);
    const GL_ID mesh_id = GLTFContext_mesh_add(context, String_cstr(&patch_name), 1);

    assert(vertices1_buffer.size%8==0);
    assert(vertices2_buffer.size%12==0);
    assert(indices_buffer.size%6==0);
    const uint32 vertex_count = vertices1_buffer.size / sizeof(VertexID_UV);

    DynamicArray_float32 positions = {0};
    DA_init(&positions, float32, vertex_count*3);
    DynamicArray_float32 normals = {0};
    DA_init(&normals, float32, vertex_count*3);
    DynamicArray_float32 uv = {0};
    DA_init(&uv, float32, vertex_count*2);

    DynamicArray_uint32 indices = {0};
    DA_init(&indices, uint32, indices_buffer.size/2);

    // vec3 bbox_min = {0};
    // vec3 bbox_max = {0};

    int16 uv_dims[2] = {0};
    uv_dims[0] = (int16) terrain_patch->TerrainDisplacementTexture.Width;
    uv_dims[1] = (int16) terrain_patch->TerrainDisplacementTexture.Height;
    if (terrain_patch->DisplacementDownsampled) {
        uv_dims[0] *= 2;
        uv_dims[1] *= 2;
    }

    VertexID_UV *id_uv = (VertexID_UV *) vertices1_buffer.data;
    VertexPosNorm *pos_norm = (VertexPosNorm *) vertices2_buffer.data;
    // for (int i = 0; i < vertex_count; ++i) {
    //     uint32 vert_id = id_uv[i].vertex_id;
    //     uint16 *pos = pos_norm[vert_id].pos;
    //
    //     //Collect bbox size
    //     if ((float32) pos[0] < bbox_min[0]) bbox_min[0] = pos[0];
    //     if ((float32) pos[1] < bbox_min[1]) bbox_min[1] = pos[1];
    //     if ((float32) pos[2] < bbox_min[2]) bbox_min[2] = pos[2];
    //     if ((float32) pos[0] > bbox_max[0]) bbox_max[0] = pos[0];
    //     if ((float32) pos[1] > bbox_max[1]) bbox_max[1] = pos[1];
    //     if ((float32) pos[2] > bbox_max[2]) bbox_max[2] = pos[2];
    // }
    // vec3 patch_dims = {0};
    // glm_vec3_sub(bbox_max, bbox_min, patch_dims);

    for (int i = 0; i < vertex_count; ++i) {
        uint32 vert_id = id_uv[i].vertex_id;
        uint16 *pos = pos_norm[vert_id].pos;
        uint8 *norm = &pos_norm[vert_id].mask_a;

        float32 x = ((float32) (pos[0] - 8192) / 32768.f);
        float32 y = ((float32) (pos[1] - 8192) / 32768.f) * 10.f;
        float32 z = ((float32) (pos[2] - 8192) / 32768.f);
        *(float32 *) (DA_append_get(&positions)) = x * 200.f;
        *(float32 *) (DA_append_get(&positions)) = y * 200.f;
        *(float32 *) (DA_append_get(&positions)) = z * 200.f;

        float32 nx = ((float32) ((int8) norm[0]) / 127.f);
        float32 ny = ((float32) ((int8) norm[1]) / 127.f);
        float32 nz = ((float32) ((int8) norm[2]) / 127.f);
        *(float32 *) (DA_append_get(&normals)) = nx;
        *(float32 *) (DA_append_get(&normals)) = ny;
        *(float32 *) (DA_append_get(&normals)) = nz;

        const float32 half_pixel_x = 1.0f / (float32)uv_dims[0];
        const float32 half_pixel_y = 1.0f / (float32)uv_dims[1];

        *(float32 *) (DA_append_get(&uv)) = ((float32) id_uv[i].UV[0] / (float32) uv_dims[0]) + half_pixel_x;
        *(float32 *) (DA_append_get(&uv)) = ((float32) id_uv[i].UV[1] / (float32) uv_dims[1]) + half_pixel_y;
    }
    for (int i = 0; i < indices_buffer.size / 2; ++i) {
        uint16 index = ((uint16 *) indices_buffer.data)[i];
        assert(index < vertex_count);
        *(uint32 *) (DA_append_get(&indices)) = index;
    }

    GLTFContext_primitive_init_attributes(context, mesh_id, 0, 3);
    GL_ID positions_accessor = GLTFContext_accessor_from_data(context,
                                                              DA_get_buffer(&positions),
                                                              positions.count * positions.item_size,
                                                              positions.count / 3,
                                                              "POSITIONS",
                                                              cgltf_type_vec3,
                                                              cgltf_component_type_r_32f,
                                                              cgltf_buffer_view_type_vertices,
                                                              false,
                                                              12,
                                                              0);
    GLTFContext_primitive_set_attribute_accessor(context, mesh_id, 0, 0, positions_accessor, "POSITION");

    GL_ID normals_accessor = GLTFContext_accessor_from_data(context,
                                                            DA_get_buffer(&normals),
                                                            normals.count * normals.item_size,
                                                            normals.count / 3,
                                                            "NORMALS",
                                                            cgltf_type_vec3,
                                                            cgltf_component_type_r_32f,
                                                            cgltf_buffer_view_type_vertices,
                                                            false,
                                                            12,
                                                            0);
    GLTFContext_primitive_set_attribute_accessor(context, mesh_id, 0, 1, normals_accessor, "NORMAL");

    GL_ID uv_accessor = GLTFContext_accessor_from_data(context,
                                                       DA_get_buffer(&uv),
                                                       uv.count * uv.item_size,
                                                       uv.count / 2,
                                                       "TEXCOORD_0",
                                                       cgltf_type_vec2,
                                                       cgltf_component_type_r_32f,
                                                       cgltf_buffer_view_type_vertices,
                                                       false,
                                                       8,
                                                       0);
    GLTFContext_primitive_set_attribute_accessor(context, mesh_id, 0, 2, uv_accessor, "TEXCOORD_0");

    GL_ID indices_accessor = GLTFContext_create_indices_accessor_from_data(
        context,
        DA_get_buffer(&indices),
        indices.count * indices.item_size, indices.count, "indices",
        cgltf_component_type_r_32u,
        0);
    GLTFContext_set_primitive_indices_accessor(context, mesh_id, 0, indices_accessor);

    DA_free(&positions);
    DA_free(&indices);
    DA_free(&normals);
    DA_free(&uv);
    vertices1_buffer.close(&vertices1_buffer);
    vertices2_buffer.close(&vertices2_buffer);
    indices_buffer.close(&indices_buffer);

    GL_ID patch_mesh_node = GLTFContext_node_add(context, String_cstr(&patch_name));
    GLTFContext_node_set_mesh(context, patch_mesh_node, mesh_id);

    mat4 patch_matrix;
    glm_mat4_identity(patch_matrix);
    glm_translate(patch_matrix, (vec3){(float32) patch_x_pos * 200.f, 0.0f, (float32) patch_z_pos * 200.f});
    GLTFContext_node_set_matrix(context, patch_mesh_node, (float32 *) patch_matrix);

    GL_ID material_id = GLTFContext_material_new(context, String_cstr(&patch_name));
    GLTFContext_primitive_set_material(context, mesh_id, 0, material_id);

    context->materials.items[material_id.v].pbr_metallic_roughness.metallic_factor = 0.f;
    context->materials.items[material_id.v].pbr_metallic_roughness.roughness_factor = 1.f;


    Texture *displacement_texture = export_terrain_texture(&terrain_patch->TerrainDisplacementTexture, 1, NULL);
    if (displacement_texture != NULL) {
        String patch_texture_name = {0};
        String_copy_from(&patch_texture_name, &patch_name);
        String_append_cstr(&patch_texture_name, "_disp");
        String *texture_save_path = GLTFContext_data_path(context);
        const uint32 hash = hash_string(&patch_texture_name);
        String_append_format(texture_save_path, "/%s_%08X", String_cstr(&patch_texture_name), hash);
        Texture_save(displacement_texture, texture_save_path);
        Texture_free(displacement_texture);
        String_free(&patch_texture_name);
        String_free(texture_save_path);
    }

    Texture *color_texture = export_terrain_texture(&terrain_patch->TerrainColorTexture, 4, NULL);
    if (color_texture != NULL) {
        String patch_texture_name = {0};
        String_copy_from(&patch_texture_name, &patch_name);
        String_append_cstr(&patch_texture_name, "_color");
        GLTFContext_material_set_diffuse_texture_from_data(context, &patch_texture_name, material_id, color_texture);
        Texture_free(color_texture);
        String_free(&patch_texture_name);
    }

    // Texture *normal_texture = export_terrain_texture(&terrain_patch->TerrainNormalTexture, 4, NULL);
    // if (normal_texture != NULL) {
    //     String patch_texture_name = {0};
    //     String_copy_from(&patch_texture_name, &patch_name);
    //     String_append_cstr(&patch_texture_name, "_normal");
    //     GLTFContext_material_set_normal_from_data(context, &patch_texture_name, material_id, normal_texture);
    //     Texture_free(normal_texture);
    //     String_free(&patch_texture_name);
    // }
    // String tmp_name = {0}; {
    //     String *texture_save_path = GLTFContext_data_path(context);
    //     String_copy_from(&tmp_name, texture_save_path);
    //     Path_join(&tmp_name, &patch_name);
    //     String_append_cstr(&tmp_name, "_duplex");
    //     String_free(texture_save_path);
    //     Path_normalize_native(&tmp_name);
    // }

    // Texture *duplex_texture = export_terrain_texture(&terrain_patch->TerrainMaterialDuplexTexture, 2, &tmp_name);
    // String_free(&tmp_name);
    // if (duplex_texture != NULL) {
    //     String patch_texture_name = {0};
    //     String_copy_from(&patch_texture_name, &patch_name);
    //     String_append_cstr(&patch_texture_name, "_duplex");
    //     String *texture_save_path = GLTFContext_data_path(context);
    //     const uint32 hash = hash_string(&patch_texture_name);
    //     String_append_format(texture_save_path, "/%s_%08X", String_data(&patch_texture_name), hash);
    //     String_free(&patch_texture_name);
    //     Texture_save(duplex_texture, texture_save_path);
    //     String_free(texture_save_path);
    //     Texture_free(duplex_texture);
    //     // String patch_texture_name = {0};
    //     // String_copy_from(&patch_texture_name, &patch_name);
    //     // String_append_cstr(&patch_texture_name, "_duplex");
    //     // GLTFContext_material_set_roughness_metallic_from_data(context, &patch_texture_name, material_id,
    //     //                                                       duplex_texture);
    // }


    String_free(&patch_name);
}

void export_terrain_instances(GLTFContext *context, ArchiveManager *archive_manager,
                              const StreamPatchBlockHeader *header, const InstanceDataPatch *instance_data_patch,
                              const String *export_path) {
    const uint32 patch_x_pos = header->PatchPositionX;
    const uint32 patch_z_pos = header->PatchPositionZ;

    const uint32 patch_size = 1 << header->PatchLod;

    for (int i = 0; i < instance_data_patch->InstanceDataLayers.count; ++i) {
        const InstanceDataLayer *instance_layer = DA_at(&instance_data_patch->InstanceDataLayers, i);
        String *layer_name = find_name32(instance_layer->Name);
        // uint32 used_type = 0;
        if (layer_name == NULL) {
            layer_name = String_new(16);
            String_format(layer_name, "layer_%08X", instance_layer->Name);
        }
        for (int j = 0; j < instance_layer->Instances.count; ++j) {
            const VegetationSystemInstance *veg_instance = DA_at(&instance_layer->Instances, j);
            // if (used_type==0) {
            //     used_type = veg_instance->NameHash;
            // }else {
            //     assert(used_type==veg_instance->NameHash);
            // }
            String instance_name = {0};
            String_format(&instance_name, "instance_%s_%03i", String_cstr(layer_name), j);

            const GL_ID instance_node = GLTFContext_node_add(context, String_cstr(&instance_name));
            mat4 instance_matrix;
            glm_mat4_identity(instance_matrix);
            glm_translate(instance_matrix, (vec3){
                              patch_x_pos * 50 + ((((float32) veg_instance->X / patch_size) * 0.93f) / 8.f),
                              ((float32) (((veg_instance->Y) / patch_size) * 0.93f) / 32.f),
                              patch_z_pos * 50 + ((((float32) veg_instance->Z / patch_size) * 0.93f) / 8.f)
                          });
            GLTFContext_node_set_matrix(context, instance_node, (float32 *) instance_matrix);
        }
        String_free(layer_name);
    }
}

GL_ID export_stream_path_file(AppState *app_state, ADF *adf,
                              const MemoryBuffer *adf_buffer) {
    const ADFInstance *path_file_header_instance = DA_at(&adf->instances, 0);
    StreamPatchFileHeader *path_file_header = ADF_read_instance(adf, path_file_header_instance, adf_buffer, &ADF_TYPES_type_info);
    // const ADFInstance *patch_block_header_instance = DA_at(&adf->instances, 1);
    // StreamPatchBlockHeader* patch_block_header = ADF_read_instance(adf, patch_block_header_instance, adf_buffer);

    for (int i = 1; i < adf->instances.count; ++i) {
        const ADFInstance *patch_instance = DA_at(&adf->instances, i);
        void *instance_data = ADF_read_instance(adf, patch_instance, adf_buffer, &ADF_TYPES_type_info);

        // printf("Instance %i\n", i);
        // ADF_print_instance(lib, patch_instance, instance_data, 0);
        // printf("\n");

        if (patch_instance->type_hash == STI_TYPE_HASH_StreamPatchBlockHeader) {
            const StreamPatchBlockHeader *block_header = instance_data;
            const ADFInstance *block_data_instance = DA_at(&adf->instances, i+1);
            const void *block_data = ADF_read_instance(adf, block_data_instance, adf_buffer, &ADF_TYPES_type_info);
            if (block_data_instance->type_hash == STI_TYPE_HASH_TerrainPatch) {
                const TerrainPatch *terrain_patch = (TerrainPatch *) block_data;
                export_terrain_patch(app_state, block_header, terrain_patch);
                // printf("1111");
                // } else if (block_data_instance->type_hash == STI_TYPE_HASH_InstanceDataPatch) {
                //     const InstanceDataPatch *instance_data_patch = (InstanceDataPatch *) block_data;
                //     export_terrain_instances(context, archive_manager, block_header, instance_data_patch, export_path);
            }
            else {
                // ADF_print_instance(lib, block_data_instance, block_data, 0);
            }
            assert(block_data_instance->type_hash!=STI_TYPE_HASH_StreamPatchBlockHeader);
            ADF_free_instance(block_data_instance, (void *) block_data, &ADF_TYPES_type_info);
            i++;
        }
        // printf("\n");
        ADF_free_instance(patch_instance, instance_data, &ADF_TYPES_type_info);
    }


    ADF_free_instance(path_file_header_instance, path_file_header, &ADF_TYPES_type_info);
    // ADF_free_instance(lib, patch_block_header_instance, patch_block_header);
    return INVALID_GL_ID;
}

GL_ID export_adf_file_from_buffer(AppState* app_state, const uint32 path_hash, const String *path,
                                  MemoryBuffer *mb) {
    CHECK_APP_STATE(app_state);
    CHECK_GLTF_STATE(&app_state->gltf_context);

    ADF adf = {0};
    ADF_from_buffer(&adf, (Buffer *) mb);

    GL_ID output_node_id = INVALID_GL_ID;

    for (int instanceId = 0; instanceId < adf.header.instance_count; instanceId++) {
        const ADFInstance *instance = DA_at(&adf.instances, instanceId);
        if (instance->type_hash == STI_TYPE_HASH_WorldSettings) {
            const WorldSettings *world_settings = ADF_read_instance(&adf, instance, mb, &ADF_TYPES_type_info);
            // const uint32 base_lod = world_settings->PatchBaseLod;
            const uint32 base_lod = 9;
            const uint32 lod_size = 1 << base_lod;
            const uint32 world_x_size_in_chunks = world_settings->WorldSize[0] / lod_size;
            const uint32 world_y_size_in_chunks = world_settings->WorldSize[2] / lod_size;
            for (int x = 0; x < world_x_size_in_chunks; ++x) {
                for (int y = 0; y < world_y_size_in_chunks; ++y) {
                    GLog_Info("Exporting tile %02ix%02i at LOD %i", x, y, base_lod);
                    String chunk_patch_path = {0};
                    String_format(&chunk_patch_path, "terrain/hp/patches/patch_%02i_%02i_%02i.streampatch", base_lod, x,
                                  y);
                    export_adf_file(app_state, &chunk_patch_path, hash_string(&chunk_patch_path));
                    String_free(&chunk_patch_path);
                    // break;
                }
                // break;
            }
            ADF_free_instance(instance, (void *) world_settings, &ADF_TYPES_type_info);
        }
        else if (instance->type_hash == STI_TYPE_HASH_StreamPatchFileHeader) {
            output_node_id = export_stream_path_file(app_state, &adf, mb);
            break;
            // }
            // if (instance->type_hash == STI_TYPE_HASH_StreamPatchFileHeader) {
            //     const StreamPatchFileHeader *ph = instance_data;
            //     tile_x = ph->PatchPositionX;
            //     tile_y = ph->PatchPositionZ;
            //     lod = ph->PatchLod;
            // } else if (instance->type_hash == STI_TYPE_HASH_TerrainPatch) {
            //     export_terrain_patch(&mesh_export_path, instance_data, tile_x, tile_y, lod);
        }
        else if (instance->type_hash == STI_TYPE_HASH_AmfModel) {
            // ADF_print_instance(lib, instance, instance_data, 0);
            assert(adf.instances.count==1 && "ADF with AmfModel should have only one instance");
            const AmfModel *model = ADF_read_instance(&adf, instance, mb, &ADF_TYPES_type_info);
            output_node_id = export_amf_model(app_state, model, path, path_hash);
            ADF_free_instance(instance, (void *) model, &ADF_TYPES_type_info);
        }
        else if (instance->type_hash == STI_TYPE_HASH_AmfMeshHeader) {
            assert(adf.instances.count==2 && "ADF with AmfMeshHeader should have only two instances");
            instanceId++;
            const AmfMeshHeader *mesh_header = ADF_read_instance(&adf, instance, mb, &ADF_TYPES_type_info);
            const ADFInstance *mesh_buffers_instance = DA_at(&adf.instances, instanceId);
            const AmfMeshBuffers *mesh_buffers = ADF_read_instance(&adf, mesh_buffers_instance, mb, &ADF_TYPES_type_info);

            // ADF_print_instance(lib, instance, instance_data, 0);
            // ADF_print_instance(lib, mesh_buffers_instance, mesh_buffers, 0);

            output_node_id = export_amf_mesh(app_state, path_hash, path, mesh_header, mesh_buffers);
            ADF_free_instance(instance, (void *) mesh_header, &ADF_TYPES_type_info);
            ADF_free_instance(mesh_buffers_instance, (void *) mesh_buffers, &ADF_TYPES_type_info);
        }
        else {
            void *instance_data = ADF_read_instance(&adf, instance, mb, &ADF_TYPES_type_info);
            String unk_file_export_path = {};
            Path_join(&unk_file_export_path, &app_state->export_path);
            Path_join(&unk_file_export_path, path);
            String_append_format(&unk_file_export_path, "_%08X", instance->type_hash);
            Path_ensure_parent_dirs(&unk_file_export_path);
            FILE *f = fopen(String_cstr(&unk_file_export_path), "wb");
            fwrite(mb->data + instance->offset, 1, instance->size, f);
            fclose(f);
            String_append_format(&unk_file_export_path, ".json");
            JsonContext ctx;
            f = fopen(String_cstr(&unk_file_export_path), "wb");
            jsonInit(&ctx, f);
            ADF_print_instance(instance, instance_data, &ctx, &ADF_TYPES_type_info);
            fclose(f);
            printf("\n");
            ADF_free_instance(instance, instance_data, &ADF_TYPES_type_info);
            String_free(&unk_file_export_path);
        }
    }

    ADF_free(&adf);
    return output_node_id;
}
