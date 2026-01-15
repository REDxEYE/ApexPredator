// Created by RED on 12.01.2026.

#include "exporter/adf_export.h"

#include "zstd.h"
#include "apex/adf/adf.h"
#include "apex/adf/adf_types.h"
#include "exporter/amf_export.h"
#include "utils/path.h"
#include "utils/stb_image_write.h"
#include "utils/buffer/file_buffer.h"


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

void export_terrain_texture(const String *export_path, const TerrainTexture *texture, const char *type) {
    if (texture->Width == 0 || texture->Height == 0 || texture->Data.UncompressedSize == 0) {
        return;
    }

    String texture_path = {};
    String_copy_from(&texture_path, export_path);
    Path_ensure_dirs(&texture_path);
    Path_join_format(&texture_path, "%s.png", type);
    const float32 pixel_size = (float32) texture->Data.UncompressedSize / (
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

void export_terrain_patch(const String *export_path, TerrainPatch *patch, uint32 x, uint32 y, uint32 lod) {
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

GL_ID export_adf_file(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                              Havok_TypeLibrary *havok_lib, const String *path, const uint32 path_hash, const String *export_path) {
    if (!ArchiveManager_has_file_by_hash(archive_manager, path_hash)) {
        printf("File not found\n");
        return INVALID_GL_ID;
    }

    MemoryBuffer mb = {0};
    if (!ArchiveManager_get_file_by_hash(archive_manager, path_hash, &mb)) {
        printf("File not found\n");
        return INVALID_GL_ID;
    }

    const GL_ID result =  export_adf_file_from_buffer(context, archive_manager, lib, havok_lib, path_hash, path, &mb, export_path);
    mb.close(&mb);
    return result;
}

GL_ID export_adf_file_from_buffer(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                      Havok_TypeLibrary *havok_lib, const uint32 path_hash, const String *path, MemoryBuffer *mb, const String *export_path) {
    assert(context!=NULL && "context must be initialized");

    ADF adf = {0};
    ADF_from_buffer(&adf, (Buffer *) mb, lib);

    String mesh_export_path = {};
    String_copy_from(&mesh_export_path, export_path);
    Path_ensure_dirs(&mesh_export_path);

    uint32 tile_x = 0, tile_y = 0, lod = 0;

    GL_ID output_node_id = INVALID_GL_ID;

    for (int instanceId = 0; instanceId < adf.header.instance_count; instanceId++) {
        const ADFInstance *instance = DA_at(&adf.instances, instanceId);
        void *instance_data = ADF_read_instance(&adf, lib, instance, mb);

        if (instance->type_hash == STI_TYPE_HASH_StreamPatchFileHeader) {
            const StreamPatchFileHeader *ph = instance_data;
            tile_x = ph->PatchPositionX;
            tile_y = ph->PatchPositionZ;
            lod = ph->PatchLod;
        } else if (instance->type_hash == STI_TYPE_HASH_TerrainPatch) {
            export_terrain_patch(&mesh_export_path, instance_data, tile_x, tile_y, lod);
        } else if (instance->type_hash == STI_TYPE_HASH_AmfModel) {
            // ADF_print_instance(lib, instance, instance_data, 0);
            output_node_id = export_amf_model(context, archive_manager, lib, havok_lib, instance_data, path, path_hash,
                                              export_path);
        } else if (instance->type_hash == STI_TYPE_HASH_AmfMeshHeader) {
            instanceId++;
            const ADFInstance *mesh_buffers_instance = DA_at(&adf.instances, instanceId);
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
