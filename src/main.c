#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cgltf_write.h"
#include "zstd.h"
#include "utils/dynamic_array.h"
#include "apex/adf/adf.h"
#include "apex/adf/adf_types.h"
#include "apex/package/archive.h"
#include "apex/package/archives.h"
#include "utils/string.h"
#include "utils/path.h"
#include "utils/buffer/buffer.h"
#include "utils/buffer/file_buffer.h"
#include "utils/base64.h"
#include "utils/gltf/cgltf_helper.h"

#include "utils/stb_image_write.h"
#include "utils/ply_writer.h"

#ifdef WIN32

void find_tab_files(const char *dir, DynamicArray_String *tab_files) {
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", dir);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;

        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s\\%s", dir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            find_tab_files(full_path, tab_files);
        } else {
            const char *ext = strrchr(fd.cFileName, '.');
            if (ext && strcmp(ext, ".tab") == 0) {
                String *tmp = DA_append_get(tab_files);
                String_from_cstr(tmp, full_path);
            }
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
}

#else
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

static int is_dir_path(const char *fullpath, const struct dirent *ent) {



// Use d_type if available and reliable; otherwise lstat
#ifdef DT_DIR
if (ent&& ent->d_type!= DT_UNKNOWN) {
        return ent->d_type == DT_DIR;
    }
#endif
struct stat st;
    if (lstat(fullpath, &st)== 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

void find_tab_files(const char *dir, DynamicArray_String *tab_files) {
    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    char full_path[PATH_MAX];

    while ((ent = readdir(d)) != NULL) {
        const char *name = ent->d_name;

        if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
            continue;

        int n = snprintf(full_path, sizeof full_path, "%s/%s", dir, name);
        if (n < 0 || (size_t) n >= sizeof full_path)
            continue; // path too long, skip

        if (is_dir_path(full_path, ent)) {
            find_tab_files(full_path, tab_files);
        } else {
            const char *ext = strrchr(name, '.');
            if (ext && strcmp(ext, ".tab") == 0) {
                String_from_cstr(DA_append_get(tab_files), full_path);
            }
        }
    }
    closedir(d);
}

#endif

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
    float32 pixel_size = ((float32) texture->Data.UncompressedSize) / ((float32) texture->Width * texture->Height);
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
            stbi_write_png(String_data(&texture_path), texture->Width, texture->Height, pixel_size,
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

DYNAMIC_ARRAY_STRUCT(float32, float32);

DYNAMIC_ARRAY_STRUCT(uint32, uint32);

DYNAMIC_ARRAY_STRUCT(uint8, uint8);


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

void export_terrain_mesh(String *export_path, TerrainMesh *mesh, uint32 tile_x, uint32 tile_y, uint32 lod) {
    if (lod < 9) {
        printf("LOD %i not supported\n", lod);
        exit(1);
    }
    float x_width = mesh->BoundingBox[3] - mesh->BoundingBox[0];
    float y_width = mesh->BoundingBox[5] - mesh->BoundingBox[2];
    float z_width = mesh->BoundingBox[4] - mesh->BoundingBox[1];

    float32 lod_size = lod_sizes[lod];

    float x_offset = tile_x * lod_size * lod_offsets[lod];
    float y_offset = tile_y * lod_size * lod_offsets[lod];
    float z_offset = 0;

    printf("Tile at %02i/%02i has lod: %i offset %+10.3f, %+10.3f, %+10.3f and size %+10.3f, %+10.3f, %+10.3f\n",
           tile_x, tile_y, lod, x_offset, y_offset, z_offset, x_width, y_width, z_width);

    DynamicArray_uint32 indices = {0};
    DynamicArray_float32 positions = {0};
    DynamicArray_float32 uvs = {0};
    DynamicArray_uint8 colors = {0};
    String mesh_export_path = {};
    Path_ensure_dirs(export_path);

    FileBuffer file_buffer = {0};
    uint32 vertex_count = 0; {
        MemoryBuffer indices_buffer = {0};
        MemoryBuffer vertices_buffer = {0};
        MemoryBuffer vertices2_buffer = {0};
        MemoryBuffer quads_buffer = {0};
        MemoryBuffer triangle_indices_buffer = {0};
        MemoryBuffer group_tri_indices_buffer = {0};
        decompress_data(&mesh->Indices, &indices_buffer);
        decompress_data(&mesh->Vertices, &vertices_buffer);
        decompress_data(&mesh->Vertices2, &vertices2_buffer);
        decompress_data(&mesh->TriangleIndices, &triangle_indices_buffer);
        decompress_data(&mesh->GroupTriIndices, &group_tri_indices_buffer);
        decompress_data(&mesh->QuadInfos, &quads_buffer);
        vertex_count = vertices_buffer.size / 8;
        assert(vertices_buffer.size%8==0);
        assert(vertices2_buffer.size%12==0);
        assert(indices_buffer.size%6==0);

        DA_init(&positions, float32, vertex_count*3);
        DA_init(&uvs, float32, vertex_count*2);
        DA_init(&colors, uint8, vertex_count*4);
        DA_init(&indices, uint32, indices_buffer.size/2);

        VertexID_UV *iduv = (VertexID_UV *) vertices_buffer.data;
        VertexPosNorm *pos_norm = (VertexPosNorm *) vertices2_buffer.data;
        for (int i = 0; i < indices_buffer.size / 2; ++i) {
            uint16 index = ((uint16 *) indices_buffer.data)[i];
            assert(index<vertex_count);
            *(uint32 *) (DA_append_get(&indices)) = index;
        }

        for (int i = 0; i < vertex_count; ++i) {
            uint32 vertex_id = iduv[i].vertex_id;
            uint16 *pos = pos_norm[vertex_id].pos;
            float32 x = (x_offset) + (float32) pos[0] / lod_size;
            float32 y = (y_offset) + (float32) pos[2] / lod_size;
            float32 z = z_offset + (float32) pos[1] / lod_size;
            *(float32 *) (DA_append_get(&positions)) = x;
            *(float32 *) (DA_append_get(&positions)) = y;
            *(float32 *) (DA_append_get(&positions)) = z;
            *(float32 *) (DA_append_get(&uvs)) = (float) iduv[i].UV[0] / (INT16_MAX) * 64 + 0.0009f;
            *(float32 *) (DA_append_get(&uvs)) = (1.f - (float) iduv[i].UV[1] / (INT16_MAX) * 164) - 0.0009f;
            *(uint8 *) (DA_append_get(&colors)) = pos_norm[vertex_id].mask_a;
            *(uint8 *) (DA_append_get(&colors)) = ((int16) pos_norm[vertex_id].curvature + 127) / 2;
            *(uint8 *) (DA_append_get(&colors)) = pos_norm[vertex_id].mask_c;
            *(uint8 *) (DA_append_get(&colors)) = pos_norm[vertex_id].mask_d;
        }

        indices_buffer.close(&indices_buffer);
        vertices_buffer.close(&vertices_buffer);
        vertices2_buffer.close(&vertices2_buffer);
        quads_buffer.close(&quads_buffer);
        triangle_indices_buffer.close(&triangle_indices_buffer);
        group_tri_indices_buffer.close(&group_tri_indices_buffer);
    }

    String_copy_from(&mesh_export_path, export_path);
    Path_join_format(&mesh_export_path, "mesh_%i_%i_%i.ply", tile_x, tile_y, lod);
    if (FileBuffer_open_write(&file_buffer, String_data(&mesh_export_path)) != BUFFER_SUCCESS) {
        printf("Failed to open %s for writing\n", String_data(&mesh_export_path));
        exit(1);
    }

    ply_write_tri_mesh_binary_buf((Buffer *) &file_buffer, positions.items,NULL, uvs.items, colors.items, 4,
                                  vertex_count,
                                  indices.items, indices.count);
    file_buffer.close(&file_buffer);

    cgltf_options gltf_options = {0};
    cgltf_data *gltf_data = malloc(sizeof(cgltf_data));
    memset(gltf_data, 0, sizeof(cgltf_data));
    gltf_data->asset.generator = "ApexPredator via cgltf";
    gltf_data->asset.version = "2.0";

    String_copy_from(&mesh_export_path, export_path);
    Path_join_format(&mesh_export_path, "mesh_%i_%i_%i.gltf", tile_x, tile_y, lod);

    DynamicArray_cgltf_mesh gl_meshes = {};
    DynamicArray_cgltf_node gl_nodes = {};
    DynamicArray_cgltf_accessor gl_accessors = {};
    DynamicArray_cgltf_buffer gl_buffers = {};
    DynamicArray_cgltf_buffer_view gl_buffer_views = {};
    DA_init(&gl_meshes, cgltf_mesh, 1);
    DA_init(&gl_nodes, cgltf_node, 1);
    DA_init(&gl_accessors, cgltf_accessor, 1);
    DA_init(&gl_buffers, cgltf_buffer, 1);
    DA_init(&gl_buffer_views, cgltf_buffer_view, 1);

    cgltf_node *gl_node = DA_append_get(&gl_nodes);
    gl_node->name = "root";

    cgltf_mesh *gl_mesh = DA_append_get(&gl_meshes);
    gl_node->mesh = (void *) (uint64) (gl_meshes.count - 1);
    gl_mesh->primitives = calloc(1, sizeof(cgltf_primitive));
    gl_mesh->primitives_count = 1;
    cgltf_primitive *gl_prim = &gl_mesh->primitives[0];
    gl_prim->type = cgltf_primitive_type_triangles;

    gl_prim->attributes = calloc(3, sizeof(cgltf_attribute));
    gl_prim->attributes_count = 3;

    cgltf_attribute *position_attribute = &gl_prim->attributes[0];
    cgltf_attribute *uv_attribute = &gl_prim->attributes[1];
    cgltf_attribute *vcolor_attribute = &gl_prim->attributes[2];

    position_attribute->name = "POSITION";
    position_attribute->type = cgltf_attribute_type_position;

    uv_attribute->name = "TEXCOORD_0";
    uv_attribute->type = cgltf_attribute_type_texcoord;
    uv_attribute->index = 0;

    vcolor_attribute->name = "_COLOR_0";
    vcolor_attribute->type = cgltf_attribute_type_color;
    vcolor_attribute->index = 0;

    position_attribute->data = (void *) (uint64) gltf_create_accessor_from_data(
        &gl_buffers, &gl_buffer_views, &gl_accessors,
        positions.items,
        positions.count * positions.item_size,
        positions.count / 3,
        "positions",
        cgltf_type_vec3,
        cgltf_component_type_r_32f,
        false, 3 * sizeof(float32), 0);

    uv_attribute->data = (void *) (uint64) gltf_create_accessor_from_data(
        &gl_buffers, &gl_buffer_views, &gl_accessors,
        uvs.items, uvs.count * uvs.item_size,
        uvs.count / 2,
        "uvs",
        cgltf_type_vec2,
        cgltf_component_type_r_32f,
        false, 2 * sizeof(float32), 0);

    vcolor_attribute->data = (void *) (uint64) gltf_create_accessor_from_data(
        &gl_buffers, &gl_buffer_views, &gl_accessors,
        colors.items,
        colors.count * colors.item_size,
        colors.count / 4,
        "colors",
        cgltf_type_vec4,
        cgltf_component_type_r_8u,
        true, 4 * sizeof(uint8), 0);

    gl_prim->indices = (void *) (uint64) gltf_create_accessor_from_data(
        &gl_buffers, &gl_buffer_views, &gl_accessors,
        indices.items, indices.count * indices.item_size,
        indices.count,
        "indices",
        cgltf_type_scalar,
        cgltf_component_type_r_32u,
        false, 0, 0);

    gltf_data->meshes_count = gl_meshes.count;
    gltf_data->meshes = DA_get_buffer(&gl_meshes);
    gltf_data->nodes_count = gl_nodes.count;
    gltf_data->nodes = DA_get_buffer(&gl_nodes);
    gltf_data->accessors_count = gl_accessors.count;
    gltf_data->accessors = DA_get_buffer(&gl_accessors);
    gltf_data->buffers_count = gl_buffers.count;
    gltf_data->buffers = DA_get_buffer(&gl_buffers);

    gltf_data->buffer_views_count = gl_buffer_views.count;
    gltf_data->buffer_views = DA_get_buffer(&gl_buffer_views);
    gltf_data->scenes = malloc(sizeof(cgltf_scene));
    gltf_data->scenes_count = 1;
    gltf_data->scenes[0].nodes = calloc(1, sizeof(cgltf_node *));
    gltf_data->scenes[0].nodes[0] = gl_node;
    gltf_data->scenes[0].nodes_count = 1;
    gltf_data->scene = &gltf_data->scenes[0];
    gltf_data->scene->name = "model";
    gltf_data->scene->extras.data = NULL;

    gltf_process_referenced(gltf_data, &gl_nodes, &gl_buffers, &gl_buffer_views, &gl_accessors);

    gltf_options.type = cgltf_file_type_gltf;
    cgltf_write_file(&gltf_options, String_data(&mesh_export_path), gltf_data);


    String_free(&mesh_export_path);
    DA_free_with_inner(&gl_meshes, {
                       cgltf_mesh* mesh = it;
                       for (uint32 i =0;i<mesh->primitives_count;i++) {
                       cgltf_primitive* primitive = &mesh->primitives[i];
                       if (primitive->attributes) free(primitive->attributes);
                       }
                       free(mesh->primitives);});

    DA_free(&gl_nodes);
    DA_free(&gl_accessors);
    DA_free_with_inner(&gl_buffers, {
                       cgltf_buffer *buf = it;
                       if (buf->data != NULL) free(buf->data);
                       if (buf->uri != NULL) free(buf->uri);
                       });
    DA_free(&gl_buffer_views);
    DA_free(&colors);
    DA_free(&positions);
    DA_free(&indices);
    DA_free(&uvs);
    free(gltf_data->scene->nodes);
    free(gltf_data->scenes);
    free(gltf_data);
}

void export_terrain_patch(String *export_path, TerrainPatch *patch, uint32 x, uint32 y, uint32 lod) {
    String tile_export_path = {};
    String_copy_from(&tile_export_path, export_path);
    // Path_join_format(&tile_export_path, "terrain_patch_%d_%d", x, y);
    export_terrain_mesh(&tile_export_path, &patch->TerrainMesh, x, y, lod);
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

void export_amf_mesh(Archives *ars, STI_TypeLibrary *lib, String *export_path, uint32 hash, AmfMeshHeader *header,
                     AmfMeshBuffers *mesh_buffers) {
    printf("");

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
            Archives_find_path(ars, &hi_res_path, &hi_res_buffer);
            ADF hi_res_adf = {0};
            ADF_from_buffer(&hi_res_adf, (Buffer *) &hi_res_buffer, lib);
            ADFInstance *instance = ADF_get_instance(&hi_res_adf, 0);
            if (instance->type_hash == STI_TYPE_HASH_AmfMeshBuffers) {
                hi_res_buffers = ADF_read_instance(&hi_res_adf, lib, instance, &hi_res_buffer);
                hi_res_free_fn = ((STI_ObjectMethods *) DM_get(&lib->object_functions, instance->type_hash))->free;
            }
            ADF_free(&hi_res_adf);
            String_free(&hi_res_path);
        }
        hi_res_buffer.close(&hi_res_buffer);
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


    cgltf_options gltf_options = {0};
    cgltf_data *gltf_data = malloc(sizeof(cgltf_data));
    memset(gltf_data, 0, sizeof(cgltf_data));
    gltf_data->asset.generator = "ApexPredator via cgltf";
    gltf_data->asset.version = "2.0";
    String mesh_export_path = {0};
    String_copy_from(&mesh_export_path, export_path);
    Path_join_format(&mesh_export_path, "mesh_%08X.gltf", hash);

    DynamicArray_cgltf_mesh gl_meshes = {};
    DynamicArray_cgltf_node gl_nodes = {};
    DynamicArray_cgltf_accessor gl_accessors = {};
    DynamicArray_cgltf_buffer gl_buffers = {};
    DynamicArray_cgltf_buffer_view gl_buffer_views = {};
    DA_init(&gl_meshes, cgltf_mesh, 1);
    DA_init(&gl_nodes, cgltf_node, 1);
    DA_init(&gl_accessors, cgltf_accessor, 1);
    DA_init(&gl_buffers, cgltf_buffer, 1);
    DA_init(&gl_buffer_views, cgltf_buffer_view, 1);

    String lod_name = {0};
    for (int lod_id = header->LodGroups.count-1; lod_id < header->LodGroups.count; ++lod_id) {
        cgltf_node *gl_node = DA_append_get(&gl_nodes);
        String_from_cstr(&lod_name, "lod_");
        String_append_format(&lod_name, "%i", lod_id);
        gl_node->name = String_detach(String_move(&lod_name));


        AmfLodGroup *lod_group = DA_at(&header->LodGroups, lod_id);
        for (int mesh_id = 0; mesh_id < lod_group->Meshes.count; ++mesh_id) {
            AmfMesh *mesh = DA_at(&lod_group->Meshes, mesh_id);
            String *mesh_type_name = DM_get(&lib->hash_strings, mesh->MeshTypeId);
            uint32 index_count = mesh->IndexCount;
            uint32 vertex_count = mesh->VertexCount;
            uint32 index_buffer_index = mesh->IndexBufferIndex;
            uint32 index_buffer_stride = mesh->IndexBufferStride;
            uint32 index_buffer_offset = mesh->IndexBufferOffset;
            DynamicArray_STI_uint8 *vertex_buffer_indices = &mesh->VertexBufferIndices;
            DynamicArray_STI_uint8 *vertex_buffer_strides = &mesh->VertexStreamStrides;
            DynamicArray_STI_uint32 *vertex_buffer_offsets = &mesh->VertexStreamOffsets;
            DynamicArray_STI_int16 *bone_lookup = &mesh->BoneIndexLookup;
            DynamicArray_AmfStreamAttribute *amf_attributes = &mesh->StreamAttributes;


            AmfBuffer *usedIndexBuffer = DA_at(&all_index_buffer, index_buffer_index);


            cgltf_mesh *gl_mesh = DA_append_get(&gl_meshes);
            gl_node->mesh = (void *) (uint64) (gl_meshes.count);

            String gl_mesh_name = {0};
            String_copy_from(&gl_mesh_name, mesh_type_name);
            gl_mesh->name = String_detach(String_move(&gl_mesh_name));
            cgltf_primitive *primitives = calloc(mesh->SubMeshes.count, sizeof(cgltf_primitive));
            gl_mesh->primitives = primitives;
            gl_mesh->primitives_count = mesh->SubMeshes.count;
            for (int subMeshId = 0; subMeshId < mesh->SubMeshes.count; ++subMeshId) {
                AmfSubMesh *sub_mesh = DA_at(&mesh->SubMeshes, subMeshId);
                String *material_name = DM_get(&lib->hash_strings, sub_mesh->SubMeshId);

                uint32 sub_index_buffer_view_id = gltf_create_buffer_and_buffer_view(&gl_buffers, &gl_buffer_views,
                    usedIndexBuffer->Data.items + index_buffer_offset + sub_mesh->IndexStreamOffset,
                    sub_mesh->IndexCount * index_buffer_stride,
                    NULL, cgltf_buffer_view_type_indices, 0, 0);

                cgltf_primitive *primitive = &primitives[subMeshId];
                primitive->type = cgltf_primitive_type_triangles;
                primitive->indices = (void *) (uint64) gltf_create_accessor(
                    &gl_accessors, sub_index_buffer_view_id, cgltf_type_scalar,
                    index_buffer_stride == 2 ? cgltf_component_type_r_16u : cgltf_component_type_r_32u,
                    sub_mesh->IndexCount, sub_mesh->IndexStreamOffset, false, NULL);

                primitive->attributes = calloc(amf_attributes->count, sizeof(cgltf_attribute));
                primitive->attributes_count = amf_attributes->count;
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

                            attribute_data = malloc(vertex_count * 3 * 4);
                            float32 *positions_data = attribute_data;
                            switch (amf_attribute->Format) {
                                case AmfFormat_R16G16B16_SNORM: {
                                    for (int i = 0; i < vertex_count; ++i) {
                                        int16 *vertex_data = (int16 *) (raw_vertex_buffer_data + i * stream_stride);
                                        positions_data[i * 3 + 0] = (float32) vertex_data[0] / 32767.0f;
                                        positions_data[i * 3 + 1] = (float32) vertex_data[1] / 32767.0f;
                                        positions_data[i * 3 + 2] = (float32) vertex_data[2] / 32767.0f;
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

                            attribute_data = malloc(vertex_count * 2 * 8);
                            float32 *uv_data = attribute_data;
                            switch (amf_attribute->Format) {
                                case AmfFormat_R16G16_SNORM: {
                                    for (int i = 0; i < vertex_count; ++i) {
                                        int16 *vertex_data = (int16 *) (raw_vertex_buffer_data + i * stream_stride);
                                        uv_data[i * 2 + 0] = (float32) vertex_data[0] / 32767.0f;
                                        uv_data[i * 2 + 1] = 1.f - (float32) vertex_data[1] / 32767.0f;
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
                    uint32 buffer_view_id = gltf_create_buffer_and_buffer_view(&gl_buffers, &gl_buffer_views,
                                                                               attribute_data,
                                                                               vertex_count * stride,
                                                                               NULL, cgltf_buffer_view_type_vertices,
                                                                               stride, 0);

                    cgltf_attribute *gl_attribute = &primitive->attributes[attr_id];
                    gl_attribute->name = String_detach(String_move(&attr_name));
                    gl_attribute->data = (void *) (uint64) gltf_create_accessor(
                        &gl_accessors, buffer_view_id, data_type, comp_type, vertex_count,
                        0, normalized, NULL);
                    free(attribute_data);
                }
            }
        }
    }

    gltf_data->meshes_count = gl_meshes.count;
    gltf_data->meshes = DA_get_buffer(&gl_meshes);
    gltf_data->nodes_count = gl_nodes.count;
    gltf_data->nodes = DA_get_buffer(&gl_nodes);
    gltf_data->accessors_count = gl_accessors.count;
    gltf_data->accessors = DA_get_buffer(&gl_accessors);
    gltf_data->buffers_count = gl_buffers.count;
    gltf_data->buffers = DA_get_buffer(&gl_buffers);

    gltf_data->buffer_views_count = gl_buffer_views.count;
    gltf_data->buffer_views = DA_get_buffer(&gl_buffer_views);
    gltf_data->scenes = malloc(sizeof(cgltf_scene));
    gltf_data->scenes_count = 1;
    gltf_data->scenes[0].nodes = calloc(gl_nodes.count, sizeof(cgltf_node *));

    for (int i = 0; i < gl_nodes.count; ++i) {
        gltf_data->scenes[0].nodes[i] = &gltf_data->nodes[i];
    }
    gltf_data->scenes[0].nodes_count = 1;
    gltf_data->scene = &gltf_data->scenes[0];
    gltf_data->scene->name = "model";
    gltf_data->scene->extras.data = NULL;

    gltf_process_referenced(gltf_data, &gl_nodes, &gl_buffers, &gl_buffer_views, &gl_accessors);

    gltf_options.type = cgltf_file_type_gltf;
    cgltf_write_file(&gltf_options, String_data(&mesh_export_path), gltf_data);

    gltf_free_data(gltf_data, &gl_meshes, &gl_nodes, &gl_accessors, &gl_buffer_views, &gl_buffers);

    String_free(&mesh_export_path);

    if (hi_res_buffers != NULL) {
        hi_res_free_fn(hi_res_buffers, lib);
        free(hi_res_buffers);
    }
    DA_free(&all_index_buffer);
    DA_free(&all_vertex_buffer);
}

void export_adf_file(Archives *archives, STI_TypeLibrary *lib, uint32 hash) {
    MemoryBuffer mb = {0};
    if (!Archives_find_hash(archives, hash, &mb)) {
        printf("File not found\n");
        return;
    }

    if (mb.data[0] != ' ' || mb.data[1] != 'F' || mb.data[2] != 'D' || mb.data[3] != 'A') {
        return;
    }


    ADF adf = {};
    ADF_from_buffer(&adf, (Buffer *) &mb, lib);

    String export_path = {};
    String_from_cstr(&export_path, "./../exported");
    Path_ensure_dirs(&export_path);

    uint32 tile_x = 0, tile_y = 0, lod = 0;

    for (int instanceId = 0; instanceId < adf.header.instance_count; instanceId++) {
        ADFInstance *instance = DA_at(&adf.instances, instanceId);
        void *instance_data = ADF_read_instance(&adf, lib, instance, &mb);

        if (instance->type_hash == STI_TYPE_HASH_StreamPatchFileHeader) {
            StreamPatchFileHeader *ph = instance_data;
            tile_x = ph->PatchPositionX;
            tile_y = ph->PatchPositionZ;
            lod = ph->PatchLod;
        } /*else if (
            // instance->type_hash == STI_TYPE_HASH_InstanceDataPatch ||
            instance->type_hash == STI_TYPE_HASH_WorldAudioPatchData ||
            instance->type_hash == STI_TYPE_HASH_WorldAudioPatchNormalData
        ) {
            object_methods->free(instance_data);
            instance_memory.close(&instance_memory);
            free(instance_data);
            continue;
        }*/ else if (instance->type_hash == STI_TYPE_HASH_TerrainPatch) {
            export_terrain_patch(&export_path, instance_data, tile_x, tile_y, lod);
        } else if (instance->type_hash == STI_TYPE_HASH_AmfMeshHeader) {
            instanceId++;
            ADFInstance *mesh_buffers_instance = DA_at(&adf.instances, instanceId);
            AmfMeshBuffers *mesh_buffers = ADF_read_instance(&adf, lib, mesh_buffers_instance, &mb);

            ADF_print_instance(lib, instance, instance_data, 0);
            ADF_print_instance(lib, mesh_buffers_instance, mesh_buffers, 0);

            export_amf_mesh(archives, lib, &export_path, hash, instance_data, mesh_buffers);
            ADF_free_instance(lib, mesh_buffers_instance, mesh_buffers);
        } else {
            ADF_print_instance(lib, instance, instance_data, 0);
        }
        ADF_free_instance(lib, instance, instance_data);
    }

    ADF_free(&adf);
    String_free(&export_path);
    mb.close(&mb);
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        printf("USAGE: %s <path_to_game_root>\n", argv[0]);
        return 0;
    }

    String tmp = {0};
    String ar_path = {0};
    String game_root = {0};
    STI_TypeLibrary lib = {0};
    STI_TypeLibrary_init(&lib);
    STI_ADF_TYPES_register_functions(&lib);
    String_from_cstr(&tmp, argv[1]);
    Archives ars = {0};
    Path_convert_to_wsl(&game_root, &tmp);
    Archives_init(&ars, &game_root);

    // export_adf_terrain(&ar, &lib, 0x2EE38766);
    export_adf_file(&ars, &lib, path_hash("models/characters/machines/dreadnought/dreadnought_frontplate_damaged.meshc"));

    Archives_free(&ars);
    STI_TypeLibrary_free(&lib);
    String_free(&ar_path);
    String_free(&tmp);
    String_free(&game_root);
    return 0;
}
