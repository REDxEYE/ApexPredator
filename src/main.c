#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cgltf_write.h"
#include "zstd.h"
#include "apex/avtx.h"
#include "apex/hashes.h"
#include "apex/rtpc.h"
#include "apex/sarc.h"
#include "utils/dynamic_array.h"
#include "apex/adf/adf.h"
#include "apex/adf/adf_types.h"
#include "apex/aaf/aaf.h"
#include "apex/package/tab_archive.h"
#include "../include/havok/tag_file/havok_tag_file.h"
#include "platform/archive_manager.h"
#include "utils/string.h"
#include "utils/path.h"
#include "utils/hash_helper.h"
#include "utils/buffer/buffer.h"
#include "utils/buffer/file_buffer.h"
#include "utils/gltf/cgltf_helper.h"
#include "utils/stb_image_write.h"

#include "platform/common_arrays.h"

bool decompress_data(CompressedData *data, MemoryBuffer *mb) {
    MemoryBuffer_allocate(mb, data->UncompressedSize);
    mb->size = data->UncompressedSize;

    CompressedHeader *compressed_header = (CompressedHeader *) data->Data.items;
    uint8 *compressed_data = (data->Data.items) + sizeof(CompressedHeader);

    switch (compressed_header->comp_type) {
        case zstd: {
            size_t zstd_res = ZSTD_decompress(mb->data, mb->size, compressed_data,
                                              data->Data.count - sizeof(CompressedHeader));
            ZSTD_ErrorCode error = ZSTD_isError(zstd_res);
            if (error != ZSTD_error_no_error) {
                printf("ZSTD decompression error: %s\n", ZSTD_getErrorName(error));
                assert(false && "ZSTD decompression failed");
                return false;
            }
            assert(zstd_res == compressed_header->decomp_size && "ZSTD decompression size mismatch");
        }
        break;
        default:
            printf("Unsupported compression type: %d\n", compressed_header->comp_type);
            assert(false && "Unsupported compression type");
            return false;
    }

    return true;
}


void export_terrain_texture(String *export_path, TerrainTexture *texture, const char *type) {
    if (texture->Width == 0 || texture->Height == 0 || texture->Data.UncompressedSize == 0) {
        return;
    }

    String texture_path = {};
    String_copy_from(&texture_path, export_path);
    Path_ensure_dirs(&texture_path);
    Path_join_format(&texture_path, "%s.png", type);
    float32 pixel_size = (float32) texture->Data.UncompressedSize / (
                             (float32) texture->Width * texture->Height);
    printf("%s : width: %i, height: %i, size: %i, pixel size: %f\n", String_data(&texture_path), texture->Width,
           texture->Height, texture->Data.UncompressedSize,
           pixel_size);


    MemoryBuffer decompressed_buffer = {0};
    decompress_data(&texture->Data, &decompressed_buffer);

    switch (texture->BlockCompressionType) {
        case E_BLOCKCOMPRESSIONTYPE_BC7:
        case E_BLOCKCOMPRESSIONTYPE_BC3: {
            assert(false && "BCn not supported yet");
            break;
        }
        case E_BLOCKCOMPRESSIONTYPE_NONE: {
            stbi_write_png(String_data(&texture_path), texture->Width, texture->Height, (int32) pixel_size,
                           decompressed_buffer.data, 0);
        }
    }

    String texture_raw_path = {};
    String_copy_from(&texture_raw_path, export_path);
    Path_ensure_dirs(&texture_raw_path);
    Path_join_format(&texture_raw_path, "%s.texture", type);

    FileBuffer file_buffer = {0};
    assert(FileBuffer_open_write(&file_buffer, String_data(&texture_raw_path))==BUFFER_SUCCESS);
    file_buffer.write(&file_buffer, decompressed_buffer.data, decompressed_buffer.size, NULL);
    file_buffer.close(&file_buffer);

    decompressed_buffer.close(&decompressed_buffer);
    String_free(&texture_path);
    String_free(&texture_raw_path);
}


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

float32 lod_sizes[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 42.666666666f * 2, 42.66635f, 42.666666666f / 2, 42.666666666f / 4};
float32 lod_offsets[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 4.5f, 24.f, 96.f, 384.f};

#pragma pack(pop)

GL_ID export_file(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib, const String *path,
                  uint32 hash, const String *export_path);

GL_ID export_adf_file(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib, uint32 path_hash,
                      const String *path, MemoryBuffer *mb,
                      const String *export_path);

// void export_terrain_mesh(String *export_path, TerrainMesh *mesh, uint32 tile_x, uint32 tile_y, uint32 lod) {
//     if (lod < 9) {
//         printf("LOD %i not supported\n", lod);
//         exit(1);
//     }
//     float x_width = mesh->BoundingBox[3] - mesh->BoundingBox[0];
//     float y_width = mesh->BoundingBox[5] - mesh->BoundingBox[2];
//     float z_width = mesh->BoundingBox[4] - mesh->BoundingBox[1];
//
//     float32 lod_size = lod_sizes[lod];
//
//     float x_offset = tile_x * lod_size * lod_offsets[lod];
//     float y_offset = tile_y * lod_size * lod_offsets[lod];
//     float z_offset = 0;
//
//     printf("Tile at %02i/%02i has lod: %i offset %+10.3f, %+10.3f, %+10.3f and size %+10.3f, %+10.3f, %+10.3f\n",
//            tile_x, tile_y, lod, x_offset, y_offset, z_offset, x_width, y_width, z_width);
//
//     DynamicArray_uint32 indices = {0};
//     DynamicArray_float32 positions = {0};
//     DynamicArray_float32 uvs = {0};
//     DynamicArray_uint8 colors = {0};
//     String mesh_export_path = {};
//     Path_ensure_dirs(export_path);
//
//     FileBuffer file_buffer = {0};
//     uint32 vertex_count = 0; {
//         MemoryBuffer indices_buffer = {0};
//         MemoryBuffer vertices_buffer = {0};
//         MemoryBuffer vertices2_buffer = {0};
//         MemoryBuffer quads_buffer = {0};
//         MemoryBuffer triangle_indices_buffer = {0};
//         MemoryBuffer group_tri_indices_buffer = {0};
//         decompress_data(&mesh->Indices, &indices_buffer);
//         decompress_data(&mesh->Vertices, &vertices_buffer);
//         decompress_data(&mesh->Vertices2, &vertices2_buffer);
//         decompress_data(&mesh->TriangleIndices, &triangle_indices_buffer);
//         decompress_data(&mesh->GroupTriIndices, &group_tri_indices_buffer);
//         decompress_data(&mesh->QuadInfos, &quads_buffer);
//         vertex_count = vertices_buffer.size / 8;
//         assert(vertices_buffer.size%8==0);
//         assert(vertices2_buffer.size%12==0);
//         assert(indices_buffer.size%6==0);
//
//         DA_init(&positions, float32, vertex_count*3);
//         DA_init(&uvs, float32, vertex_count*2);
//         DA_init(&colors, uint8, vertex_count*4);
//         DA_init(&indices, uint32, indices_buffer.size/2);
//
//         VertexID_UV *iduv = (VertexID_UV *) vertices_buffer.data;
//         VertexPosNorm *pos_norm = (VertexPosNorm *) vertices2_buffer.data;
//         for (int i = 0; i < indices_buffer.size / 2; ++i) {
//             uint16 index = ((uint16 *) indices_buffer.data)[i];
//             assert(index<vertex_count);
//             *(uint32 *) (DA_append_get(&indices)) = index;
//         }
//
//         for (int i = 0; i < vertex_count; ++i) {
//             uint32 vertex_id = iduv[i].vertex_id;
//             uint16 *pos = pos_norm[vertex_id].pos;
//             float32 x = (x_offset) + (float32) pos[0] / lod_size;
//             float32 y = (y_offset) + (float32) pos[2] / lod_size;
//             float32 z = z_offset + (float32) pos[1] / lod_size;
//             *(float32 *) (DA_append_get(&positions)) = x;
//             *(float32 *) (DA_append_get(&positions)) = y;
//             *(float32 *) (DA_append_get(&positions)) = z;
//             *(float32 *) (DA_append_get(&uvs)) = (float) iduv[i].UV[0] / (INT16_MAX) * 64 + 0.0009f;
//             *(float32 *) (DA_append_get(&uvs)) = (1.f - (float) iduv[i].UV[1] / (INT16_MAX) * 164) - 0.0009f;
//             *(uint8 *) (DA_append_get(&colors)) = pos_norm[vertex_id].mask_a;
//             *(uint8 *) (DA_append_get(&colors)) = ((int16) pos_norm[vertex_id].curvature + 127) / 2;
//             *(uint8 *) (DA_append_get(&colors)) = pos_norm[vertex_id].mask_c;
//             *(uint8 *) (DA_append_get(&colors)) = pos_norm[vertex_id].mask_d;
//         }
//
//         indices_buffer.close(&indices_buffer);
//         vertices_buffer.close(&vertices_buffer);
//         vertices2_buffer.close(&vertices2_buffer);
//         quads_buffer.close(&quads_buffer);
//         triangle_indices_buffer.close(&triangle_indices_buffer);
//         group_tri_indices_buffer.close(&group_tri_indices_buffer);
//     }
//
//     String_copy_from(&mesh_export_path, export_path);
//     Path_join_format(&mesh_export_path, "mesh_%i_%i_%i.ply", tile_x, tile_y, lod);
//     if (FileBuffer_open_write(&file_buffer, String_data(&mesh_export_path)) != BUFFER_SUCCESS) {
//         printf("Failed to open %s for writing\n", String_data(&mesh_export_path));
//         exit(1);
//     }
//
//     ply_write_tri_mesh_binary_buf((Buffer *) &file_buffer, positions.items,NULL, uvs.items, colors.items, 4,
//                                   vertex_count,
//                                   indices.items, indices.count);
//     file_buffer.close(&file_buffer);
//
//     cgltf_options gltf_options = {0};
//     cgltf_data *gltf_data = malloc(sizeof(cgltf_data));
//     memset(gltf_data, 0, sizeof(cgltf_data));
//     gltf_data->asset.generator = "ApexPredator via cgltf";
//     gltf_data->asset.version = "2.0";
//
//     String_copy_from(&mesh_export_path, export_path);
//     Path_join_format(&mesh_export_path, "mesh_%i_%i_%i.gltf", tile_x, tile_y, lod);
//
//     DynamicArray_cgltf_mesh gl_meshes = {};
//     DynamicArray_cgltf_node gl_nodes = {};
//     DynamicArray_cgltf_accessor gl_accessors = {};
//     DynamicArray_cgltf_buffer gl_buffers = {};
//     DynamicArray_cgltf_buffer_view gl_buffer_views = {};
//     DA_init(&gl_meshes, cgltf_mesh, 1);
//     DA_init(&gl_nodes, cgltf_node, 1);
//     DA_init(&gl_accessors, cgltf_accessor, 1);
//     DA_init(&gl_buffers, cgltf_buffer, 1);
//     DA_init(&gl_buffer_views, cgltf_buffer_view, 1);
//
//     cgltf_node *gl_node = DA_append_get(&gl_nodes);
//     gl_node->name = "root";
//
//     cgltf_mesh *gl_mesh = DA_append_get(&gl_meshes);
//     gl_node->mesh = (void *) (uint64) (gl_meshes.count - 1);
//     gl_mesh->primitives = calloc(1, sizeof(cgltf_primitive));
//     gl_mesh->primitives_count = 1;
//     cgltf_primitive *gl_prim = &gl_mesh->primitives[0];
//     gl_prim->type = cgltf_primitive_type_triangles;
//
//     gl_prim->attributes = calloc(3, sizeof(cgltf_attribute));
//     gl_prim->attributes_count = 3;
//
//     cgltf_attribute *position_attribute = &gl_prim->attributes[0];
//     cgltf_attribute *uv_attribute = &gl_prim->attributes[1];
//     cgltf_attribute *vcolor_attribute = &gl_prim->attributes[2];
//
//     position_attribute->name = "POSITION";
//     position_attribute->type = cgltf_attribute_type_position;
//
//     uv_attribute->name = "TEXCOORD_0";
//     uv_attribute->type = cgltf_attribute_type_texcoord;
//     uv_attribute->index = 0;
//
//     vcolor_attribute->name = "_COLOR_0";
//     vcolor_attribute->type = cgltf_attribute_type_color;
//     vcolor_attribute->index = 0;
//
//     position_attribute->data = (void *) (uint64) gltf_create_accessor_from_data(
//         &gl_buffers, &gl_buffer_views, &gl_accessors,
//         positions.items,
//         positions.count * positions.item_size,
//         positions.count / 3,
//         "positions",
//         cgltf_type_vec3,
//         cgltf_component_type_r_32f,
//         false, 3 * sizeof(float32), 0);
//
//     uv_attribute->data = (void *) (uint64) gltf_create_accessor_from_data(
//         &gl_buffers, &gl_buffer_views, &gl_accessors,
//         uvs.items, uvs.count * uvs.item_size,
//         uvs.count / 2,
//         "uvs",
//         cgltf_type_vec2,
//         cgltf_component_type_r_32f,
//         false, 2 * sizeof(float32), 0);
//
//     vcolor_attribute->data = (void *) (uint64) gltf_create_accessor_from_data(
//         &gl_buffers, &gl_buffer_views, &gl_accessors,
//         colors.items,
//         colors.count * colors.item_size,
//         colors.count / 4,
//         "colors",
//         cgltf_type_vec4,
//         cgltf_component_type_r_8u,
//         true, 4 * sizeof(uint8), 0);
//
//     gl_prim->indices = (void *) (uint64) gltf_create_accessor_from_data(
//         &gl_buffers, &gl_buffer_views, &gl_accessors,
//         indices.items, indices.count * indices.item_size,
//         indices.count,
//         "indices",
//         cgltf_type_scalar,
//         cgltf_component_type_r_32u,
//         false, 0, 0);
//
//     gltf_data->meshes_count = gl_meshes.count;
//     gltf_data->meshes = DA_get_buffer(&gl_meshes);
//     gltf_data->nodes_count = gl_nodes.count;
//     gltf_data->nodes = DA_get_buffer(&gl_nodes);
//     gltf_data->accessors_count = gl_accessors.count;
//     gltf_data->accessors = DA_get_buffer(&gl_accessors);
//     gltf_data->buffers_count = gl_buffers.count;
//     gltf_data->buffers = DA_get_buffer(&gl_buffers);
//
//     gltf_data->buffer_views_count = gl_buffer_views.count;
//     gltf_data->buffer_views = DA_get_buffer(&gl_buffer_views);
//     gltf_data->scenes = malloc(sizeof(cgltf_scene));
//     gltf_data->scenes_count = 1;
//     gltf_data->scenes[0].nodes = calloc(1, sizeof(cgltf_node *));
//     gltf_data->scenes[0].nodes[0] = gl_node;
//     gltf_data->scenes[0].nodes_count = 1;
//     gltf_data->scene = &gltf_data->scenes[0];
//     gltf_data->scene->name = "model";
//     gltf_data->scene->extras.data = NULL;
//
//     gltf_process_referenced(gltf_data, &gl_nodes, &gl_buffers, &gl_buffer_views, &gl_accessors);
//
//     gltf_options.type = cgltf_file_type_gltf;
//     cgltf_write_file(&gltf_options, String_data(&mesh_export_path), gltf_data);
//
//
//     String_free(&mesh_export_path);
//
//     gltf_free_data(gltf_data, &gl_meshes, &gl_nodes, &gl_accessors, &gl_buffer_views, &gl_buffers);
//
//     DA_free(&gl_buffer_views);
//     DA_free(&colors);
//     DA_free(&positions);
//     DA_free(&indices);
//     DA_free(&uvs);
// }

void export_terrain_patch(String *export_path, TerrainPatch *patch, uint32 x, uint32 y, uint32 lod) {
    String tile_export_path = {};
    String_copy_from(&tile_export_path, export_path);
    // Path_join_format(&tile_export_path, "terrain_patch_%d_%d", x, y);
    // export_terrain_mesh(&tile_export_path, &patch->TerrainMesh, x, y, lod);
    // export_terrain_texture(&tile_export_path, &patch->TerrainDisplacementTexture, "displacement");
    // export_terrain_texture(&tile_export_path, &patch->TerrainNormalTexture, "normal");
    // export_terrain_texture(&tile_export_path, &patch->TerrainTriangleMapTexture, "triangle_map");
    // export_terrain_texture(&tile_export_path, &patch->TerrainMaterialDuplexTexture, "material_duplex");
    // export_terrain_texture(&tile_export_path, &patch->TerrainColorTexture, "color");
    // export_terrain_texture(&tile_export_path, &patch->TerrainQualityTexture, "quality");
    // export_terrain_texture(&tile_export_path, &patch->TerrainIndirectionTexture, "indirection");
    // export_terrain_texture(&tile_export_path, &patch->TerrainSSDFAtlas, "ssdf_atlas");
    String_free(&tile_export_path);
}

GL_ID export_amf_mesh(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib, String *export_path,
                      uint32 path_hash,
                      const String *path, AmfMeshHeader *header, AmfMeshBuffers *mesh_buffers) {
    String mesh_name = {0};
    if (path != NULL) {
        Path_filename(path, &mesh_name);
    } else {
        String_from_cstr(&mesh_name, "mesh_");
        String_append_format(&mesh_name, "%08X", path_hash);
    }
    GL_ID mesh_root_node_id = GLTFContext_add_node(context, String_data(&mesh_name));

    DynamicArray_AmfBuffer all_vertex_buffer = {0};
    DA_init(&all_vertex_buffer, AmfBuffer, mesh_buffers->VertexBuffers.count);

    DynamicArray_AmfBuffer all_index_buffer = {0};
    DA_init(&all_index_buffer, AmfBuffer, mesh_buffers->IndexBuffers.count);

    AmfMeshBuffers *hi_res_buffers = NULL;
    freeSTIObject hi_res_free_fn = NULL; {
        MemoryBuffer hi_res_buffer = {0};
        String *hi_res_path_full = DM_get(&lib->hash_strings, header->HighLodPath);
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
            GL_ID gl_node_id = GLTFContext_add_node(context, String_data(&lod_name));
            GLTFContext_node_set_parent(context, gl_node_id, mesh_root_node_id);
            String_free(&lod_name);

            AmfMesh *mesh = DA_at(&lod_group->Meshes, mesh_id);
            String *mesh_type_name = DM_get(&lib->hash_strings, mesh->MeshTypeId);
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


            GL_ID gl_mesh_id = GLTFContext_add_mesh(context, String_data(mesh_type_name), mesh->SubMeshes.count);
            GLTFContext_node_set_mesh(context, gl_node_id, gl_mesh_id);

            for (int sub_mesh_id = 0; sub_mesh_id < mesh->SubMeshes.count; ++sub_mesh_id) {
                AmfSubMesh *sub_mesh = DA_at(&mesh->SubMeshes, sub_mesh_id);
                String *material_name = DM_get(&lib->hash_strings, sub_mesh->SubMeshId);

                GL_ID material_id = GLTFContext_find_material_by_name(context, String_data(material_name));

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
                    void *attribute_data;
                    uint8 *raw_vertex_buffer_data =
                            all_vertex_buffer.items[stream_index].Data.items + vertex_buffer_offsets->items[
                                amf_attribute->StreamIndex] + stream_offset;
                    uint32 stride;
                    switch (amf_attribute->Usage) {
                        case AmfUsage_Position: {
                            data_type = cgltf_type_vec3;
                            comp_type = cgltf_component_type_r_32f;
                            normalized = false;
                            stride = 12;
                            String_from_cstr(&attr_name, "POSITION");
                            float32 packing_data = *(float32 *) amf_attribute->PackingData;

                            attribute_data = malloc(vertex_count * 3 * 4);
                            float32 *positions_data = attribute_data;
                            switch (amf_attribute->Format) {
                                case AmfFormat_R16G16B16_SNORM: {
                                    for (int i = 0; i < vertex_count; ++i) {
                                        int16 *vertex_data = (int16 *) (raw_vertex_buffer_data + i * stream_stride);
                                        positions_data[i * 3 + 0] = packing_data * (float32) vertex_data[0] / 32767.0f;
                                        positions_data[i * 3 + 1] = packing_data * (float32) vertex_data[1] / 32767.0f;
                                        positions_data[i * 3 + 2] = packing_data * (float32) vertex_data[2] / 32767.0f;
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
                            normalized = true;
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
                            normalized = true;
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
                        default: {
                            printf("Unsupported attribute usage: %d\n", amf_attribute->Usage);
                            continue;
                            // exit(1);
                        }
                    }

                    GL_ID buffer_view_id = GLTFContext_create_accessor_from_data(context,
                        attribute_data, vertex_count * stride, vertex_count, NULL,
                        data_type, comp_type, cgltf_buffer_view_type_vertices, normalized,
                        stride, 0);

                    GLTFContext_primitive_set_attribute_accessor(context, gl_mesh_id, sub_mesh_id, attr_id,
                                                                 buffer_view_id, String_data(&attr_name));
                    String_free(&attr_name);
                    free(attribute_data);
                }
            }
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


GL_ID export_amf_model(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib, AmfModel *amf_model,
                       const String *path, uint32 path_hash, const String *export_path) {
    assert(context!=NULL && "context must be initialized");

    String model_export_path = {};
    String model_without_ext = {};
    String model_name = {};
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

    GL_ID model_root_node_id = GLTFContext_add_node(context, String_data(&model_name));
    String_free(&model_name);

    for (int mat_id = 0; mat_id < amf_model->Materials.count; ++mat_id) {
        const AmfMaterial *amf_material = &amf_model->Materials.items[mat_id];
        const String *material_name = DM_get(&lib->hash_strings, amf_material->Name);
        GLTFContext_add_material(context, String_data(material_name));
        for (int tex_id = 0; tex_id < amf_material->Textures.count; ++tex_id) {
            const String *tex_path = DM_get(&lib->hash_strings, amf_material->Textures.items[tex_id]);
            if (tex_path->size == 0) {
                break;
            }
            export_file(context, archive_manager, lib, tex_path, hash_string(tex_path), export_path);
        }
    }

    const String *mesh_path = DM_get(&lib->hash_strings, amf_model->Mesh);


    MemoryBuffer mb = {0};
    if (!ArchiveManager_get_file(archive_manager, mesh_path, &mb)) {
        printf("File not found\n");
        return model_root_node_id;
    }

    GL_ID mesh_root_node = export_adf_file(context, archive_manager, lib, hash_string(mesh_path), mesh_path, &mb,
                                           export_path);
    GLTFContext_node_set_parent(context, mesh_root_node, model_root_node_id);

    mb.close(&mb);

    return model_root_node_id;
}

GL_ID export_adf_file(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                      uint32 path_hash, const String *path, MemoryBuffer *mb,
                      const String *export_path) {
    assert(context!=NULL && "context must be initialized");

    ADF adf = {0};
    ADF_from_buffer(&adf, (Buffer *) mb, lib);

    String mesh_export_path = {};
    String_copy_from(&mesh_export_path, export_path);
    Path_ensure_dirs(&mesh_export_path);

    uint32 tile_x = 0, tile_y = 0, lod = 0;

    GL_ID output_node_id = INVALID_GL_ID;

    for (int instanceId = 0; instanceId < adf.header.instance_count; instanceId++) {
        ADFInstance *instance = DA_at(&adf.instances, instanceId);
        void *instance_data = ADF_read_instance(&adf, lib, instance, mb);

        if (instance->type_hash == STI_TYPE_HASH_StreamPatchFileHeader) {
            StreamPatchFileHeader *ph = instance_data;
            tile_x = ph->PatchPositionX;
            tile_y = ph->PatchPositionZ;
            lod = ph->PatchLod;
        } else if (instance->type_hash == STI_TYPE_HASH_TerrainPatch) {
            export_terrain_patch(&mesh_export_path, instance_data, tile_x, tile_y, lod);
        } else if (instance->type_hash == STI_TYPE_HASH_AmfModel) {
            // ADF_print_instance(lib, instance, instance_data, 0);
            output_node_id = export_amf_model(context, archive_manager, lib, instance_data, path, path_hash,
                                              export_path);
        } else if (instance->type_hash == STI_TYPE_HASH_AmfMeshHeader) {
            instanceId++;
            ADFInstance *mesh_buffers_instance = DA_at(&adf.instances, instanceId);
            AmfMeshBuffers *mesh_buffers = ADF_read_instance(&adf, lib, mesh_buffers_instance, mb);

            // ADF_print_instance(lib, instance, instance_data, 0);
            // ADF_print_instance(lib, mesh_buffers_instance, mesh_buffers, 0);

            output_node_id = export_amf_mesh(context, archive_manager, lib, &mesh_export_path, path_hash, path,
                                             instance_data,
                                             mesh_buffers);
            ADF_free_instance(lib, mesh_buffers_instance, mesh_buffers);
        } else {
            String unk_file_export_path = {};
            Path_join(&unk_file_export_path, export_path);
            Path_join(&unk_file_export_path, path);
            Path_ensure_parent_dirs(&unk_file_export_path);
            FILE *f = fopen(String_data(&unk_file_export_path), "wb");
            fwrite(mb->data + instance->offset, 1, instance->size, f);
            fclose(f);
            ADF_print_instance(lib, instance, instance_data, 0);
        }
        ADF_free_instance(lib, instance, instance_data);
    }

    ADF_free(&adf);
    String_free(&mesh_export_path);
    return output_node_id;
}

void export_ddsc(ArchiveManager *archive_manager, STI_TypeLibrary *lib, uint32 hash, MemoryBuffer *mb,
                 const String *path,
                 const String *export_path) {
    String texture_export_path = {};
    String texture_without_ext = {};
    Path_remove_extension(path, &texture_without_ext);
    Path_join(&texture_export_path, export_path);
    Path_join(&texture_export_path, &texture_without_ext);

    String tmp_check = {};
    String_copy_from(&tmp_check, &texture_export_path);
    String_append_cstr(&tmp_check, ".png");
    if (Path_exists(&tmp_check)) {
        String_free(&texture_export_path);
        String_free(&texture_without_ext);
        return;
    }
    Texture tex = {0};

    AVTXHeader header;
    mb->read(mb, &header, sizeof(header), NULL);
    if (strncmp(header.ident, "AVTX", 4) != 0) {
        printf("[ERROR]: Invalid AVTX texture format\n");
        exit(1);
    }
    if (header.version != 1) {
        printf("[ERROR]: Unsupported AVTX version: %d\n", header.version);
        exit(1);
    }
    uint32 actual_body_size = 0;
    uint8 *compressed_data;
    if (header.flags & 1) {
        String atx_path = {0};
        // highest mips are in atx<N> file
        for (int i = 5; i > 0; --i) {
            Path_remove_extension(path, &atx_path);
            String_append_format(&atx_path, ".atx%i", i);
            if (ArchiveManager_has_file(archive_manager, &atx_path)) {
                break;
            }
        }
        MemoryBuffer atx_buffer = {0};
        ArchiveManager_get_file(archive_manager, &atx_path, &atx_buffer);

        const int64 largest_mip_size = Texture_calculate_mip_size(0, header.width, header.height, header.format);
        atx_buffer.set_position(&atx_buffer, -largest_mip_size, BUFFER_ORIGIN_END);
        compressed_data = malloc(largest_mip_size);
        atx_buffer.read(&atx_buffer, compressed_data, largest_mip_size, &actual_body_size);
        if (actual_body_size < largest_mip_size) {
            printf("[ERROR]: Failed to read AVTX texture data, expected size: %u, actual size: %u\n", header.body_size,
                   actual_body_size);
            free(compressed_data);
            exit(1);
        }
        atx_buffer.close(&atx_buffer);
        String_free(&atx_path);
    } else {
        mb->set_position(mb, header.header_size, BUFFER_ORIGIN_START);
        compressed_data = malloc(header.body_size);
        mb->read(mb, compressed_data, header.body_size, &actual_body_size);
        if (actual_body_size != header.body_size) {
            printf("[ERROR]: Failed to read AVTX texture data, expected size: %u, actual size: %u\n", header.body_size,
                   actual_body_size);
            free(compressed_data);
            exit(1);
        }
    }
    Texture_from_dxgi(&tex, header.format, header.width, header.height, header.depth, compressed_data,
                      actual_body_size);
    free(compressed_data);

    Texture_save(&tex, &texture_export_path);
    String_free(&texture_export_path);
    String_free(&texture_without_ext);
    Texture_free(&tex);
}

GL_ID process_epe_node(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib, RuntimeNode *node,
                       uint32 path_hash,
                       const String *path, const String *export_path) {
    assert(context!=NULL && "context must be initialized");
    if (!RuntimeNode_has_prop(node, "_class")) {
        return INVALID_GL_ID;
    }
    String *node_name = RuntimeNode_get_prop_str(node, "name");
    String *node_name_hash = &node->name;
    GL_ID output_node = INVALID_GL_ID;

    const String *class_name = RuntimeNode_get_prop_str(node, "_class");
    if (class_name == NULL) {
        printf("[ERROR]: Failed to get _class property\n");
        exit(1);
    }
    if (String_cequals(class_name, "CCharacter")) {
        String *model_filename_hash = RuntimeNode_get_prop_by_hash_str(node, 0xE8129FE6);
        if (model_filename_hash == NULL) {
            printf("[ERROR]: Failed to get model property for CCharacter\n");
            return INVALID_GL_ID;
        }
        output_node = export_file(context, archive_manager, lib, model_filename_hash, hash_string(model_filename_hash),
                                  export_path);
    } else if (String_cequals(class_name, "CSecondaryMotionAttachment")) {
        String *model_filename = RuntimeNode_get_prop_str(node, "model");
        if (model_filename == NULL) {
            printf("[ERROR]: Failed to get model property for CSecondaryMotionAttachment\n");
            return INVALID_GL_ID;
        }
        output_node = export_file(context, archive_manager, lib, model_filename, hash_string(model_filename),
                                  export_path);
    } else if (String_cequals(class_name, "CRigidObject")) {
        uint32 model_filename_hash = RuntimeNode_get_prop_u32(node, "filename");
        if (model_filename_hash == 0) {
            printf("[ERROR]: Failed to get model property for CRigidObject\n");
            return INVALID_GL_ID;
        }
        output_node = export_file(context, archive_manager, lib, NULL, model_filename_hash, export_path);
    }

    if (!IS_VALID_GL_ID(output_node)) {
        output_node = GLTFContext_add_node(
            context, node_name != NULL ? String_data(node_name) : String_data(node_name_hash));
    }
    float32 *matrix = RuntimeNode_get_prop_mat4x4(node, "world");
    if (matrix != NULL)
        GLTFContext_node_set_matrix(context, output_node, matrix);
    String extra_data = {0};
    String_append_cstr(&extra_data, "{");
    DA_FORI(node->props, i) {
        RuntimeProp_emit_json(&node->props.items[i], &extra_data, 1);
        if (i + 1 < node->props.count)
            String_append_cstr(&extra_data, ", ");
    }
    String_append_cstr(&extra_data, "}");
    GLTFContext_node_set_extra(context, output_node, String_data(&extra_data));
    String_free(&extra_data);
    DA_FORI(node->children, i) {
        GL_ID child_node_id = process_epe_node(context, archive_manager, lib, DA_at(&node->children, i), path_hash,
                                               path, export_path);
        if (IS_VALID_GL_ID(child_node_id))
            GLTFContext_node_set_parent(context, child_node_id, output_node);
    }
    return output_node;
}

GL_ID export_epe(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib, RuntimeNode *root_node,
                 uint32 path_hash,
                 const String *path, const String *export_path) {
    assert(context!=NULL && "context must be initialized");
    if (path == NULL) {
        printf("[ERROR]: Path is NULL\n");
        exit(1);
    }
    String epe_export_path = {};
    Path_join(&epe_export_path, export_path);
    Path_join(&epe_export_path, path);
    Path_ensure_parent_dirs(&epe_export_path);
    String_append_cstr(&epe_export_path, ".gltf");
    GLTFContext_set_save_path(context, &epe_export_path);

    String epe_name = {};
    if (path != NULL) {
        Path_filename(path, &epe_name);
    } else {
        String_from_cstr(&epe_name, "epe_");
        String_append_format(&epe_name, "%08X", path_hash);
    }

    GL_ID epe_root_node_id = GLTFContext_add_node(context, "epe_root");

    DA_FORI(root_node->children, i) {
        GL_ID epe_node_id = process_epe_node(context, archive_manager, lib, DA_at(&root_node->children, i), path_hash,
                                             path, export_path);
        if (IS_VALID_GL_ID(epe_node_id))
            GLTFContext_node_set_parent(context, epe_node_id, epe_root_node_id);
    }
    return epe_root_node_id;
}

GL_ID export_file(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib, const String *path,
                  uint32 hash, const String *export_path) {
    assert(context!=NULL && "context must be initialized");

    MemoryBuffer mb = {0};
    if (!ArchiveManager_get_file_by_hash(archive_manager, hash, &mb)) {
        printf("File not found\n");
        return INVALID_GL_ID;
    }
    GL_ID output_node_id = INVALID_GL_ID;

    if (memcmp(mb.data, ADF_MAGIC, 4) == 0) {
        output_node_id = export_adf_file(context, archive_manager, lib, hash, path, &mb, export_path);
    } else if (memcmp(mb.data, AAF_MAGIC, 4) == 0) {
        AAFArchive aaf_archive = {0};
        AAFArchive_from_buffer(&aaf_archive, (Buffer *) &mb);
        MemoryBuffer *section_buffer = MemoryBuffer_new();
        if (!AAFArchive_get_data(&aaf_archive, section_buffer)) {
            printf("[ERROR]: Failed to get AAF section 0\n");
            return INVALID_GL_ID;
        }

        SArchive *sarc = SArchive_new((Buffer *) section_buffer); // sarc is now owner of buffer
        ArchiveManager_add(archive_manager, (Archive *) sarc);
        Archive_print_files((Archive *) sarc);
        AAFArchive_free(&aaf_archive);
    } else if (memcmp(mb.data, AVTX_MAGIC, 4) == 0) {
        export_ddsc(archive_manager, lib, hash, &mb, path, export_path);
    } else if (memcmp(mb.data, RTPC_MAGIC, 4) == 0) {
        RuntimeNode *root_node = RuntimeContainer_from_buffer((Buffer *) &mb);
        // RuntimeNode_print(root_node, stdout, 0);
        // RuntimeNode_emit_json(root_node, stdout, 0);
        output_node_id = export_epe(context, archive_manager, lib, root_node, hash, path, export_path);
        RuntimeNode_free(root_node);
    } else if (memcmp(mb.data + 4, "TAG0", 4) == 0) {
        TagFile tag_file={0};
        TagFile_from_buffer(&tag_file, (Buffer *) &mb);
        TagFile_free(&tag_file);
    } else {
        String unk_file_export_path = {};
        Path_join(&unk_file_export_path, export_path);
        Path_join(&unk_file_export_path, path);
        Path_ensure_parent_dirs(&unk_file_export_path);
        FILE *f = fopen(String_data(&unk_file_export_path), "wb");
        fwrite(mb.data, 1, mb.size, f);
        fclose(f);
        printf("Unhandled file \"%s\" been written to file: \"%s\"", String_data(path),
               String_data(&unk_file_export_path));
    }
    mb.close(&mb);
    // if (!IS_VALID_GL_ID(output_node_id)) {
    //     String path_stem = {};
    //     if (path != NULL) {
    //         Path_filename(path, &path_stem);
    //     }else {
    //         String_from_cstr(&path_stem, "file_");
    //         String_append_format(&path_stem, "%08X", hash);
    //     }
    //     output_node_id = GLTFContext_add_node(context, String_data(&path_stem));
    //     String_free(&path_stem);
    // }
    return output_node_id;
}

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        printf("USAGE: %s <path_to_game_root> <path_to_file> [extra_path]\n", argv[0]);
        return 0;
    }
    ArchiveManager manager = {0};
    ArchiveManager_init(&manager);

    STI_TypeLibrary lib = {0};
    STI_TypeLibrary_init(&lib);
    STI_ADF_TYPES_register_functions(&lib);


    String tmp = {0};
    String ar_path = {0};
    String game_root = {0};
    String_from_cstr(&tmp, argv[1]);
    Path_convert_to_wsl(&game_root, &tmp);
    TabArchives_init(&manager, &game_root);
    String export_path = {};
    String_from_cstr(&export_path, "../exported");
    String file_path = {};
    GLTFContext context = {0};
    GLTFContext_init(&context, "root");


    String_from_cstr(&file_path, argv[2]);
    String_from_cstr(&file_path, "editor/entities/characters/machines/dreadnought/drea_classb_load01.ee");
    export_file(&context, &manager, &lib, &file_path, hash_string(&file_path), &export_path);
    String_from_cstr(&file_path, "editor/entities/characters/machines/dreadnought/drea_classb_load01.epe");
    if (argc >=4) {
        String_from_cstr(&file_path, argv[3]);
        export_file(&context, &manager, &lib, &file_path, hash_string(&file_path), &export_path);
    }
    String_from_cstr(&file_path, "animations/skeletons/characters/dreadnought.bsk");
    export_file(&context, &manager, &lib, &file_path, hash_string(&file_path), &export_path);

    GLTFContext_write_and_free(&context);

    // String_from_cstr(&file_path, "models/characters/machines/hunter/hunter_base_prototype.modelc");
    // export_file(&manager, &lib, &file_path, hash_string(&file_path), &export_path);

    ArchiveManager_free(&manager);
    STI_TypeLibrary_free(&lib);
    String_free(&ar_path);
    String_free(&tmp);
    String_free(&game_root);
    String_free(&export_path);
    String_free(&file_path);
    return 0;
}
