#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "zstd.h"
#include "apex/package/tab.h"
#include "utils/dynamic_array.h"
#include "apex/adf/adf.h"
#include "apex/adf/adf_types.h"
#include "apex/package/archive.h"
#include "utils/string.h"
#include "utils/path.h"
#include "utils/buffer/buffer.h"
#include "utils/buffer/file_buffer.h"
#include "utils/lookup3.h"


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

    float x_offset = tile_x * lod_size * lod_offsets[lod]; //mesh->BoundingBox[3];
    float y_offset = tile_y * lod_size * lod_offsets[lod]; //mesh->BoundingBox[5];
    float z_offset = 0; //-lod_size*lod_offsets[lod]; //mesh->BoundingBox[4];

    printf("Tile at %02i/%02i has lod: %i offset %+10.3f, %+10.3f, %+10.3f and size %+10.3f, %+10.3f, %+10.3f\n",
           tile_x, tile_y, lod, x_offset, y_offset, z_offset, x_width, y_width, z_width);

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
    uint32 vertex_count = vertices_buffer.size / 8;
    assert(vertices_buffer.size%8==0);
    assert(vertices2_buffer.size%12==0);
    assert(indices_buffer.size%6==0);


    String mesh_export_path = {};
    Path_ensure_dirs(export_path);
    FileBuffer file_buffer = {0};
    // String_copy_from(&mesh_export_path, export_path);
    // Path_join_format(&mesh_export_path, "mesh.indices");
    //
    // assert(FileBuffer_open_write(&file_buffer, String_data(&mesh_export_path))==BUFFER_SUCCESS);
    // file_buffer.write(&file_buffer, indices_buffer.data, indices_buffer.size, NULL);
    // file_buffer.close(&file_buffer);
    //
    // String_copy_from(&mesh_export_path, export_path);
    // Path_join_format(&mesh_export_path, "mesh.vertices");
    //
    // assert(FileBuffer_open_write(&file_buffer, String_data(&mesh_export_path))==BUFFER_SUCCESS);
    // file_buffer.write(&file_buffer, vertices_buffer.data, vertices_buffer.size, NULL);
    // file_buffer.close(&file_buffer);
    //
    // String_copy_from(&mesh_export_path, export_path);
    // Path_join_format(&mesh_export_path, "mesh.vertices2");
    //
    // assert(FileBuffer_open_write(&file_buffer, String_data(&mesh_export_path))==BUFFER_SUCCESS);
    // file_buffer.write(&file_buffer, vertices2_buffer.data, vertices2_buffer.size, NULL);
    // file_buffer.close(&file_buffer);
    //
    // String_copy_from(&mesh_export_path, export_path);
    // Path_join_format(&mesh_export_path, "mesh.quads");
    //
    // assert(FileBuffer_open_write(&file_buffer, String_data(&mesh_export_path))==BUFFER_SUCCESS);
    // file_buffer.write(&file_buffer, quads_buffer.data, quads_buffer.size, NULL);
    // file_buffer.close(&file_buffer);
    //
    //
    // String_copy_from(&mesh_export_path, export_path);
    // Path_join_format(&mesh_export_path, "mesh.tri_ind");
    //
    // assert(FileBuffer_open_write(&file_buffer, String_data(&mesh_export_path))==BUFFER_SUCCESS);
    // file_buffer.write(&file_buffer, triangle_indices_buffer.data, triangle_indices_buffer.size, NULL);
    // file_buffer.close(&file_buffer);
    //
    //
    // String_copy_from(&mesh_export_path, export_path);
    // Path_join_format(&mesh_export_path, "mesh.tri_group_ind");
    //
    // assert(FileBuffer_open_write(&file_buffer, String_data(&mesh_export_path))==BUFFER_SUCCESS);
    // file_buffer.write(&file_buffer, group_tri_indices_buffer.data, group_tri_indices_buffer.size, NULL);
    // file_buffer.close(&file_buffer);

    DynamicArray_uint32 indices = {0};
    DynamicArray_float32 positions = {0};
    DynamicArray_float32 uvs = {0};
    DynamicArray_uint8 colors = {0};
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
    // if (lod==10) {
    //     printf("asd");
    // }


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
    String_free(&mesh_export_path);
    indices_buffer.close(&indices_buffer);
    vertices_buffer.close(&vertices_buffer);
    vertices2_buffer.close(&vertices2_buffer);
    quads_buffer.close(&quads_buffer);
    triangle_indices_buffer.close(&triangle_indices_buffer);
    group_tri_indices_buffer.close(&group_tri_indices_buffer);

    DA_free(&colors);
    DA_free(&positions);
    DA_free(&indices);
    DA_free(&uvs);
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
}

void export_adf_terrain(Archive *archive, STI_TypeLibrary *lib, uint32 hash) {
    MemoryBuffer mb = {0};
    if (!Archive_get_data(archive, hash, &mb)) {
        printf("File not found\n");
        return;
    }
    char tmp[512];
    snprintf(tmp, 512, "stream_%08X.avtx", hash);
    FILE *f = fopen(tmp, "wb");
    fwrite(mb.data, mb.size, 1, f);
    fclose(f);

    if (mb.data[0] != ' ' || mb.data[1] != 'F' || mb.data[2] != 'D' || mb.data[3] != 'A') {
        return;
    }


    ADF adf = {};
    ADF_from_buffer(&adf, (Buffer *) &mb, lib);

    String export_path = {};
    String_from_cstr(&export_path, "./../exported");
    Path_ensure_dirs(&export_path);

    uint32 tile_x, tile_y, lod;

    MemoryBuffer instance_memory = {};

    for (int i = 0; i < adf.header.instance_count; i++) {
        ADFInstance *instance = DA_at(&adf.instances, i);
        STI_Type *type = DM_get(&lib->types, instance->type_hash);
        printf("Instance: %s, type %s\n", String_data(&adf.strings.items[instance->name_id]),
               type ? String_data(&type->name) : "UNKNOWN");
        if (type == NULL) {
            printf("Unknown type hash %08X for instance %s\n", instance->type_hash,
                   String_data(&adf.strings.items[instance->name_id]));
            continue;
        }


        STI_ObjectMethods *object_methods = DM_get(&lib->object_functions, instance->type_hash);
        if (object_methods == NULL) {
            printf("No read function for type hash %08X (%s)\n", instance->type_hash, String_data(&type->name));
            continue;
        }
        MemoryBuffer_allocate(&instance_memory, instance->size);
        memcpy(instance_memory.data, mb.data + instance->offset, instance->size);

        void *instance_data = malloc(object_methods->size);

        if (!object_methods->read((Buffer *) &instance_memory, lib, instance_data)) {
            printf("Failed to read instance %s of type %s\n", String_data(&adf.strings.items[instance->name_id]),
                   String_data(&type->name));
            object_methods->free(instance_data, lib);
            instance_memory.close(&instance_memory);
            free(instance_data);
            continue;
        }

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
        } else {
            object_methods->print(instance_data, lib, stdout, 0);
        }
        object_methods->free(instance_data, lib);
        instance_memory.close(&instance_memory);
        free(instance_data);
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
    Archive ar = {0};
    String ar_path = {0};
    STI_TypeLibrary lib = {0};
    STI_TypeLibrary_init(&lib);
    STI_ADF_TYPES_register_functions(&lib);

    String_from_cstr(
        &tmp,
        "C:\\Program Files (x86)\\Steam\\steamapps\\common\\GenerationZero\\archives_win64\\initial\\game0.tab");
    Path_convert_to_wsl(&ar_path, &tmp);
    String_free(&tmp);

    Archive_open(&ar, &ar_path);

    export_adf_terrain(&ar, &lib, 0x2EE38766);

    Archive_free(&ar);
    STI_TypeLibrary_free(&lib);
    String_free(&ar_path);
    String_free(&tmp);
    return 0;
}
