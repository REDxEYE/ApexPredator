// Created by RED on 12.01.2026.

#include "exporter/amf_export.h"

#include "apex/hashes.h"
#include "apex/adf/adf.h"
#include "exporter/adf_export.h"
#include "exporter/ddsc_export.h"
#include "havok/havok_codegen.h"
#include "platform/texture_ops.h"
#include "utils/hash_helper.h"
#include "utils/path.h"


GL_ID export_amf_mesh(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib, String *export_path,
                      uint32 path_hash, const String *path, AmfMeshHeader *header, AmfMeshBuffers *mesh_buffers) {
    String mesh_name = {0};
    if (path != NULL) {
        Path_filename(path, &mesh_name);
    } else {
        String_from_cstr(&mesh_name, "mesh_");
        String_append_format(&mesh_name, "%08X", path_hash);
    }
    GL_ID mesh_root_node_id = GLTFContext_node_add(context, String_data(&mesh_name));

    DynamicArray_AmfBuffer all_vertex_buffer = {0};
    DA_init(&all_vertex_buffer, AmfBuffer, mesh_buffers->VertexBuffers.count);

    DynamicArray_AmfBuffer all_index_buffer = {0};
    DA_init(&all_index_buffer, AmfBuffer, mesh_buffers->IndexBuffers.count);

    AmfMeshBuffers *hi_res_buffers = NULL;
    freeSTIObject hi_res_free_fn = NULL;
    // hires fix
    {
        MemoryBuffer hi_res_buffer = {0};
        // String *hi_res_path_full = DM_get(&lib->hash_strings, header->HighLodPath);
        const String *hi_res_path_full = find_name32(header->HighLodPath);
        if (String_find_subcstring(hi_res_path_full, "intermediate/") != UINT32_MAX) {
            String hi_res_path = {0};

            String_from_cstr(&hi_res_path, String_data(hi_res_path_full) + strlen("intermediate/"));
            if (ArchiveManager_get_file(archive_manager, &hi_res_path, &hi_res_buffer)) {
                ADF hi_res_adf = {0};
                ADF_from_buffer(&hi_res_adf, (Buffer *) &hi_res_buffer, lib);
                ADFInstance *instance = ADF_get_instance(&hi_res_adf, 0);
                if (instance->type_hash == STI_TYPE_HASH_AmfMeshBuffers) {
                    hi_res_buffers = ADF_read_instance(&hi_res_adf, lib, instance, &hi_res_buffer);
                    hi_res_free_fn = ((STI_ObjectMethods *) DM_get(&lib->object_functions, instance->type_hash))->free;
                }
                ADF_free(&hi_res_adf);
                String_free(&hi_res_path);
                hi_res_buffer.close(&hi_res_buffer);
            }
        }
    }

    for (int i = 0; i < mesh_buffers->VertexBuffers.count; ++i) {
        AmfBuffer *vertex_buffer = DA_at(&mesh_buffers->VertexBuffers, i);
        *(AmfBuffer *) DA_append_get(&all_vertex_buffer) = *vertex_buffer;
    }
    for (int i = 0; i < mesh_buffers->IndexBuffers.count; ++i) {
        AmfBuffer *index_buffer = DA_at(&mesh_buffers->IndexBuffers, i);
        *(AmfBuffer *) DA_append_get(&all_index_buffer) = *index_buffer;
    }


    if (hi_res_buffers != NULL) {
        for (int i = 0; i < hi_res_buffers->VertexBuffers.count; ++i) {
            AmfBuffer *vertex_buffer = DA_at(&hi_res_buffers->VertexBuffers, i);
            *(AmfBuffer *) DA_append_get(&all_vertex_buffer) = *vertex_buffer;
        }
        for (int i = 0; i < hi_res_buffers->IndexBuffers.count; ++i) {
            AmfBuffer *index_buffer = DA_at(&hi_res_buffers->IndexBuffers, i);
            *(AmfBuffer *) DA_append_get(&all_index_buffer) = *index_buffer;
        }
    }


    String mesh_export_path = {};
    String mesh_without_ext = {};
    Path_remove_extension(path, &mesh_without_ext);
    Path_join(&mesh_export_path, export_path);
    Path_join(&mesh_export_path, &mesh_without_ext);
    String_append_cstr(&mesh_export_path, ".gltf");

    String lod_name = {0};
    for (uint32 lod_id = header->LodGroups.count - 1; lod_id < header->LodGroups.count; ++lod_id) {
        AmfLodGroup *lod_group = DA_at(&header->LodGroups, lod_id);
        for (int mesh_id = 0; mesh_id < lod_group->Meshes.count; ++mesh_id) {
            String_init(&lod_name, 64);
            String_copy_from(&lod_name, &mesh_name);
            String_append_format(&lod_name, "_lod_%i_mesh_%i", lod_id, mesh_id);
            GL_ID gl_node_id = GLTFContext_node_add(context, String_data(&lod_name));
            GLTFContext_node_set_parent(context, gl_node_id, mesh_root_node_id);
            String_free(&lod_name);

            AmfMesh *mesh = DA_at(&lod_group->Meshes, mesh_id);
            const String *mesh_type_name = find_name32(mesh->MeshTypeId);
            uint32 vertex_count = mesh->VertexCount;
            uint32 index_buffer_index = mesh->IndexBufferIndex;
            if (index_buffer_index > all_index_buffer.count) {
                continue;
            }

            uint32 index_buffer_stride = mesh->IndexBufferStride;
            uint32 index_buffer_offset = mesh->IndexBufferOffset;
            DynamicArray_STI_uint8 *vertex_buffer_indices = &mesh->VertexBufferIndices;
            DynamicArray_STI_uint8 *vertex_buffer_strides = &mesh->VertexStreamStrides;
            DynamicArray_STI_uint32 *vertex_buffer_offsets = &mesh->VertexStreamOffsets;
            DynamicArray_STI_int16 *bone_lookup = &mesh->BoneIndexLookup;
            DynamicArray_AmfStreamAttribute *amf_attributes = &mesh->StreamAttributes;


            AmfBuffer *usedIndexBuffer = DA_at(&all_index_buffer, index_buffer_index);


            GL_ID gl_mesh_id = GLTFContext_mesh_add(context, String_data(mesh_type_name), mesh->SubMeshes.count);
            GLTFContext_node_set_mesh(context, gl_node_id, gl_mesh_id);

            bool has_bone_data = false;
            for (int sub_mesh_id = 0; sub_mesh_id < mesh->SubMeshes.count; ++sub_mesh_id) {
                AmfSubMesh *sub_mesh = DA_at(&mesh->SubMeshes, sub_mesh_id);
                const String *material_name = find_name32(sub_mesh->SubMeshId);

                GL_ID material_id = GLTFContext_material_find_by_name(context, String_data(material_name));

                cgltf_primitive *primitive = GLTFContext_mesh_get_primitive(context, gl_mesh_id, sub_mesh_id);
                if (material_id.v != UINT32_MAX) {
                    GLTFContext_primitive_set_material(context, gl_mesh_id, sub_mesh_id, material_id);
                }
                primitive->type = cgltf_primitive_type_triangles;
                GL_ID index_accessor_id = GLTFContext_create_indices_accessor_from_data(
                    context,
                    usedIndexBuffer->Data.items + index_buffer_offset + sub_mesh->IndexStreamOffset,
                    sub_mesh->IndexCount * index_buffer_stride, sub_mesh->IndexCount, NULL,
                    index_buffer_stride == 2 ? cgltf_component_type_r_16u : cgltf_component_type_r_32u,
                    sub_mesh->IndexStreamOffset);
                GLTFContext_set_primitive_indices_accessor(context, gl_mesh_id, sub_mesh_id, index_accessor_id);
                GLTFContext_primitive_init_attributes(context, gl_mesh_id, sub_mesh_id, amf_attributes->count);
                uint32 uv_count = 0;
                for (int attr_id = 0; attr_id < amf_attributes->count; ++attr_id) {
                    AmfStreamAttribute *amf_attribute = &amf_attributes->items[attr_id];
                    uint32 stream_index = vertex_buffer_indices->items[amf_attribute->StreamIndex];
                    uint32 stream_offset = amf_attribute->StreamOffset;
                    uint32 stream_stride = vertex_buffer_strides->items[amf_attribute->StreamIndex];

                    cgltf_type data_type;
                    cgltf_component_type comp_type;
                    bool normalized;
                    String attr_name = {0};
                    String_from_cstr(&attr_name, "_INVALID_ATTRIBUTE");
                    void *attribute_data;
                    uint8 *raw_vertex_buffer_data =
                            all_vertex_buffer.items[stream_index].Data.items + vertex_buffer_offsets->items[
                                amf_attribute->StreamIndex] + stream_offset;
                    float32 bbox_min[3] = {0, 0, 0};
                    float32 bbox_max[3] = {0, 0, 0};
                    bool use_bbox = false;

                    uint32 stride;
                    switch (amf_attribute->Usage) {
                        case AmfUsage_Position: {
                            data_type = cgltf_type_vec3;
                            comp_type = cgltf_component_type_r_32f;
                            normalized = false;
                            use_bbox = true;
                            stride = 12;
                            String_from_cstr(&attr_name, "POSITION");
                            float32 packing_data = *(float32 *) amf_attribute->PackingData;

                            attribute_data = malloc(vertex_count * 3 * 4);
                            float32 *positions_data = attribute_data;
                            switch (amf_attribute->Format) {
                                case AmfFormat_R16G16B16_SNORM: {
                                    for (int i = 0; i < vertex_count; ++i) {
                                        int16 *vertex_data = (int16 *) (raw_vertex_buffer_data + i * stream_stride);
                                        float32 x = packing_data * (float32) vertex_data[0] / 32767.0f;
                                        float32 y = packing_data * (float32) vertex_data[1] / 32767.0f;
                                        float32 z = packing_data * (float32) vertex_data[2] / 32767.0f;
                                        // Update min/max
                                        if (i == 0) {
                                            bbox_min[0] = x;
                                            bbox_min[1] = y;
                                            bbox_min[2] = z;
                                            bbox_max[0] = x;
                                            bbox_max[1] = y;
                                            bbox_max[2] = z;
                                        } else {
                                            if (x < bbox_min[0]) bbox_min[0] = x;
                                            if (y < bbox_min[1]) bbox_min[1] = y;
                                            if (z < bbox_min[2]) bbox_min[2] = z;
                                            if (x > bbox_max[0]) bbox_max[0] = x;
                                            if (y > bbox_max[1]) bbox_max[1] = y;
                                            if (z > bbox_max[2]) bbox_max[2] = z;
                                        }

                                        positions_data[i * 3 + 0] = x;
                                        positions_data[i * 3 + 1] = y;
                                        positions_data[i * 3 + 2] = z;
                                    }
                                    break;
                                }
                                default: {
                                    printf("Unsupported position attribute format:%d\n", amf_attribute->Format);
                                    exit(1);
                                }
                            }
                            break;
                        }
                        case AmfUsage_TextureCoordinate: {
                            data_type = cgltf_type_vec2;
                            comp_type = cgltf_component_type_r_32f;
                            normalized = false;
                            stride = 8;
                            String_from_cstr(&attr_name, "TEXCOORD_");
                            String_append_format(&attr_name, "%d", uv_count);
                            uv_count++;
                            float32 *packing_data = (float32 *) amf_attribute->PackingData;

                            attribute_data = malloc(vertex_count * 2 * 8);
                            float32 *uv_data = attribute_data;
                            switch (amf_attribute->Format) {
                                case AmfFormat_R16G16_SNORM: {
                                    for (int i = 0; i < vertex_count; ++i) {
                                        int16 *vertex_data = (int16 *) (raw_vertex_buffer_data + i * stream_stride);
                                        uv_data[i * 2 + 0] = ((float32) vertex_data[0] / 32767.0f) * packing_data[0];
                                        uv_data[i * 2 + 1] = ((float32) vertex_data[1] / 32767.0f) * packing_data[1];
                                    }
                                    break;
                                }
                                default: {
                                    printf("Unsupported texcoord attribute format:%d\n", amf_attribute->Format);
                                    exit(1);
                                }
                            }
                            break;
                        }
                        case AmfUsage_Normal: {
                            data_type = cgltf_type_vec3;
                            comp_type = cgltf_component_type_r_32f;
                            normalized = false;
                            stride = 12;
                            String_from_cstr(&attr_name, "NORMAL");

                            attribute_data = malloc(vertex_count * 3 * 4);
                            float32 *normals_data = attribute_data;
                            switch (amf_attribute->Format) {
                                case AmfFormat_R32_UNIT_VEC_AS_FLOAT: {
                                    for (int i = 0; i < vertex_count; ++i) {
                                        float32 vertex_data = *(float32 *) (raw_vertex_buffer_data + i * stream_stride);
                                        float32 x = vertex_data;
                                        float32 y = vertex_data * (1.0f / 256.f);
                                        float32 z = vertex_data * (1.0f / (256.f * 256.f));
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

                                        normals_data[i * 3 + 0] = x;
                                        normals_data[i * 3 + 1] = y;
                                        normals_data[i * 3 + 2] = z;
                                    }
                                    break;
                                }
                                default: {
                                    printf("Unsupported normal attribute format:%d\n", amf_attribute->Format);
                                    exit(1);
                                }
                            }
                            break;
                        }
                        case AmfUsage_Color: {
                            data_type = cgltf_type_vec4;
                            comp_type = cgltf_component_type_r_8u;
                            normalized = true;
                            stride = 4;
                            String_from_cstr(&attr_name, "_COLOR_0");

                            attribute_data = malloc(vertex_count * 4);
                            uint8 *color_data = attribute_data;
                            switch (amf_attribute->Format) {
                                case AmfFormat_R32_R8G8B8A8_UNORM_AS_FLOAT: {
                                    for (int i = 0; i < vertex_count; ++i) {
                                        float32 vertex_data = *(float32 *) (raw_vertex_buffer_data + i * stream_stride);
                                        float32 r = vertex_data;
                                        float32 g = vertex_data * (1.0f / 256.f);
                                        float32 b = vertex_data * (1.0f / 256.f * 256.f);
                                        float32 a = vertex_data * (1.0f / 256.f * 256.f * 256.f);
                                        float32 bogus;
                                        r = modff(r, &bogus);
                                        g = modff(g, &bogus);
                                        b = modff(b, &bogus);
                                        a = modff(a, &bogus);

                                        color_data[i * 4 + 0] = (uint8) (r * 255.f);
                                        color_data[i * 4 + 1] = (uint8) (g * 255.f);
                                        color_data[i * 4 + 2] = (uint8) (b * 255.f);
                                        color_data[i * 4 + 3] = (uint8) (a * 255.f);
                                    }
                                    break;
                                }
                                default: {
                                    printf("Unsupported color attribute format:%d\n", amf_attribute->Format);
                                    exit(1);
                                }
                            }
                            break;
                        }
                        case AmfUsage_BoneIndex: {
                            data_type = cgltf_type_vec4;
                            comp_type = cgltf_component_type_r_16u;
                            has_bone_data = true;
                            normalized = false;
                            stride = 4 * sizeof(uint16);
                            String_from_cstr(&attr_name, "JOINTS_0");
                            attribute_data = malloc(vertex_count * 4 * sizeof(uint16));
                            uint8 *bone_index_data = attribute_data;
                            switch (amf_attribute->Format) {
                                case AmfFormat_R8G8B8A8_UINT: {
                                    for (int i = 0; i < vertex_count; ++i) {
                                        uint8 *vertex_data = (uint8 *) (raw_vertex_buffer_data + i * stream_stride);
                                        for (int j = 0; j < 4; ++j) {
                                            int16 bone_index = bone_lookup->items[vertex_data[j]];
                                            ((uint16 *) bone_index_data)[i * 4 + j] = (uint16) bone_index;
                                        }
                                    }
                                    break;
                                }
                                default: {
                                    printf("Unsupported bone index attribute format:%d\n", amf_attribute->Format);
                                    exit(1);
                                }
                            }
                            break;
                        }
                        case AmfUsage_BoneWeight: {
                            data_type = cgltf_type_vec4;
                            comp_type = cgltf_component_type_r_32f;
                            normalized = false;
                            stride = 4 * sizeof(float32);
                            String_from_cstr(&attr_name, "WEIGHTS_0");
                            attribute_data = malloc(vertex_count * 4 * sizeof(float32));
                            float32 *bone_weight_data = attribute_data;
                            switch (amf_attribute->Format) {
                                case AmfFormat_R8G8B8A8_UNORM: {
                                    for (int i = 0; i < vertex_count; ++i) {
                                        uint8 *vertex_data = (uint8 *) (raw_vertex_buffer_data + i * stream_stride);
                                        float32 weight_sum = 0.0f;
                                        for (int j = 0; j < 4; ++j) {
                                            bone_weight_data[i * 4 + j] = (float32) vertex_data[j] / 255.0f;
                                            weight_sum += bone_weight_data[i * 4 + j];
                                        }
                                        // Normalize weights
                                        // if (weight_sum==0.0f) {
                                        //     bone_weight_data[i * 4 + 0] = 1.0f;
                                        //     printf("AAAAAAAAAA\n");
                                        // }
                                        if (weight_sum > 0.0f) {
                                            for (int j = 0; j < 4; ++j) {
                                                bone_weight_data[i * 4 + j] /= weight_sum;
                                            }
                                        }
                                    }
                                    break;
                                }
                                case AmfFormat_R32G32B32A32_FLOAT: {
                                    for (int i = 0; i < vertex_count; ++i) {
                                        float32 *vertex_data = (float32 *) (raw_vertex_buffer_data + i * stream_stride);
                                        float32 weight_sum = 0.0f;
                                        for (int j = 0; j < 4; ++j) {
                                            bone_weight_data[i * 4 + j] = vertex_data[j];
                                            weight_sum += vertex_data[j];
                                        }
                                        // Normalize weights
                                        if (weight_sum > 0.0f) {
                                            for (int j = 0; j < 4; ++j) {
                                                bone_weight_data[i * 4 + j] /= weight_sum;
                                            }
                                        }
                                    }
                                    break;
                                }
                                default: {
                                    printf("Unsupported bone weight attribute format:%d\n", amf_attribute->Format);
                                    exit(1);
                                }
                            }
                            break;
                        }
                        default: {
                            printf("Unsupported attribute usage: %d\n", amf_attribute->Usage);
                            continue;
                            // exit(1);
                        }
                    }

                    GL_ID buffer_view_id = GLTFContext_accessor_from_data(context,
                                                                          attribute_data, vertex_count * stride,
                                                                          vertex_count, String_data(&attr_name),
                                                                          data_type, comp_type,
                                                                          cgltf_buffer_view_type_vertices, normalized,
                                                                          stride, 0);
                    if (use_bbox)
                        GLTFContext_accessor_set_minmax(context, buffer_view_id, bbox_min, bbox_max);
                    GLTFContext_primitive_set_attribute_accessor(context, gl_mesh_id, sub_mesh_id, attr_id,
                                                                 buffer_view_id, String_data(&attr_name));
                    String_free(&attr_name);
                    free(attribute_data);
                }
            }
            GL_ID current_skin = GLTFContext_current_skin(context);
            if (has_bone_data && IS_VALID_GL_ID(current_skin))
                GLTFContext_node_set_skin(context, gl_node_id, current_skin);
        }
    }

    context->options.type = cgltf_file_type_gltf;
    Path_ensure_parent_dirs(&mesh_export_path);
    GLTFContext_set_save_path(context, &mesh_export_path);

    String_free(&mesh_export_path);

    if (hi_res_buffers != NULL) {
        hi_res_free_fn(hi_res_buffers, lib);
        free(hi_res_buffers);
    }
    DA_free(&all_index_buffer);
    DA_free(&all_vertex_buffer);

    return mesh_root_node_id;
}

bool export_textures = true;

GL_ID export_amf_model(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                       Havok_TypeLibrary *havok_lib, const AmfModel *amf_model,
                       const String *path, const uint32 path_hash, const String *export_path) {
    assert(context!=NULL && "context must be initialized");

    String model_export_path = {0};
    String model_without_ext = {0};
    String model_name = {0};
    if (path == NULL) {
        String_init(&model_without_ext, 64);
        String_append_format(&model_without_ext, "0x%08X", path_hash);
        String_from_cstr(&model_name, "model_");
        String_append_format(&model_name, "%08X", path_hash);
    } else {
        Path_remove_extension(path, &model_without_ext);
        Path_filename(path, &model_name);
    }
    Path_join(&model_export_path, export_path);
    Path_join(&model_export_path, &model_without_ext);

    const GL_ID model_root_node_id = GLTFContext_node_add(context, String_data(&model_name));
    String_free(&model_name);

    for (int mat_id = 0; mat_id < amf_model->Materials.count; ++mat_id) {
        const AmfMaterial *amf_material = &amf_model->Materials.items[mat_id];
        const String *material_name = find_name32(amf_material->Name);
        GL_ID material_id = GLTFContext_material_find_by_name(context, String_data(material_name));
        if (!IS_VALID_GL_ID(material_id)) {
            material_id = GLTFContext_material_new(context, String_data(material_name));
            const String *render_block_id = find_name32(amf_material->RenderBlockId);
            if (String_cequals(render_block_id, "GeneralR2") && export_textures) {
                if (amf_material->Attributes.type_hash != STI_TYPE_HASH_GeneralR2Constants) {
                    printf("Unsupported GeneralR2 material attribute type: %08X\n", amf_material->Attributes.type_hash);
                    continue;
                }
                const GeneralR2Constants *constants = amf_material->Attributes.data;
                cgltf_material *mat = &context->materials.items[material_id.v];
                mat->has_pbr_metallic_roughness = true;
                mat->pbr_metallic_roughness.base_color_factor[0] = 1.0f;
                mat->pbr_metallic_roughness.base_color_factor[1] = 1.0f;
                mat->pbr_metallic_roughness.base_color_factor[2] = 1.0f;
                mat->pbr_metallic_roughness.base_color_factor[3] = 1.0f;
                mat->pbr_metallic_roughness.metallic_factor = 1.0f;
                mat->pbr_metallic_roughness.roughness_factor = 1.0f;

                if (amf_material->Textures.count >= 3) {
                    const String *diffuse_path = find_name32(amf_material->Textures.items[0]);
                    const String *normal_path = find_name32(amf_material->Textures.items[1]);
                    const String *orm_path = find_name32(amf_material->Textures.items[2]);
                    const String *emission_path = find_name32(amf_material->Textures.items[4]);
                    Texture *diffuse_texture = NULL;
                    Texture *normal_texture = NULL;
                    Texture *orm_texture = NULL;
                    Texture *emission_texture = NULL;

                    if (diffuse_path != NULL && diffuse_path->size > 0) {
                        diffuse_texture = convert_ddsc(archive_manager, diffuse_path);
                        GLTFContext_material_set_diffuse_texture_from_data(
                            context, diffuse_path, material_id, diffuse_texture);
                    }
                    if (normal_path != NULL && normal_path->size > 0) {
                        normal_texture = convert_ddsc(archive_manager, normal_path);
                        GLTFContext_material_set_normal_from_data(context, normal_path, material_id, normal_texture);
                    }
                    if (orm_path != NULL && orm_path->size > 0) {
                        orm_texture = convert_ddsc(archive_manager, orm_path);
                        GLTFContext_material_set_roughness_metallic_from_data(
                            context, orm_path, material_id, orm_texture);
                    }
                    if (emission_path != NULL && emission_path->size > 0 && constants->UseEmissive) {
                        emission_texture = convert_ddsc(archive_manager, emission_path);
                        if (!constants->EmissiveTextureHasColor && diffuse_texture != NULL) {
                            Texture *new_emission_texture = TextureOps_multiply(diffuse_texture, emission_texture);
                            if (new_emission_texture != NULL) {
                                Texture_free(emission_texture);
                                emission_texture = new_emission_texture;
                            }
                        }
                        GLTFContext_material_set_emissive_from_data(context, emission_path, material_id,
                                                                    emission_texture);
                    }

                    if (constants->UseAlbedoDetail) {
                        const String *albedo_detail_path = find_name32(amf_material->Textures.items[5]);
                        Texture *albedo_detail = convert_ddsc(archive_manager, albedo_detail_path);
                        String* texture_save_path = GLTFContext_data_path(context);
                        const uint32 hash = hash_string(albedo_detail_path);
                        String tex_name = {0};
                        Path_filename(albedo_detail_path, &tex_name);
                        String_append_format(texture_save_path, "/%s_%08X", String_data(&tex_name), hash);
                        String_free(&tex_name);
                        Texture_save(albedo_detail, texture_save_path);
                        String_free(texture_save_path);
                    }
                    if (constants->UseNormalDetail) {
                        const String *normal_detail_path = find_name32(amf_material->Textures.items[6]);
                        Texture *normal_detail = convert_ddsc(archive_manager, normal_detail_path);
                        String* texture_save_path = GLTFContext_data_path(context);
                        const uint32 hash = hash_string(normal_detail_path);
                        String tex_name = {0};
                        Path_filename(normal_detail_path, &tex_name);
                        String_append_format(texture_save_path, "/%s_%08X", String_data(&tex_name), hash);
                        String_free(&tex_name);
                        Texture_save(normal_detail, texture_save_path);
                        String_free(texture_save_path);
                    }

                    if (diffuse_texture != NULL) {
                        Texture_free(diffuse_texture);
                    }
                    if (normal_texture != NULL) {
                        Texture_free(normal_texture);
                    }
                    if (orm_texture != NULL) {
                        Texture_free(orm_texture);
                    }
                    if (emission_texture != NULL) {
                        Texture_free(emission_texture);
                    }
                }
            } else {
                printf("Unsupported material render block: %s\n", String_data(render_block_id));
                continue;
            }
            printf("Material %s -> %s\n", String_data(material_name), String_data(render_block_id));
            for (int tex_id = 0; tex_id < amf_material->Textures.count; ++tex_id) {
                const uint32 texture_hash = amf_material->Textures.items[tex_id];
                const String *tex_path = find_name32(texture_hash);
                if (tex_path->size == 0) {
                    continue;
                }
                printf("\tSlot %i -> %s\n", tex_id, String_data(tex_path));
            }
        }
    }

    const String *mesh_path = find_name32(amf_model->Mesh);


    MemoryBuffer mb = {0};
    if (!ArchiveManager_get_file(archive_manager, mesh_path, &mb)) {
        printf("File not found\n");
        return model_root_node_id;
    }

    const GL_ID mesh_root_node = export_adf_file_from_buffer(context, archive_manager, lib, havok_lib, hash_string(mesh_path),
                                                 mesh_path,
                                                 &mb,
                                                 export_path);
    GLTFContext_node_set_parent(context, mesh_root_node, model_root_node_id);

    mb.close(&mb);

    return model_root_node_id;
}
