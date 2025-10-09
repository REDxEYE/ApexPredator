// Created by RED on 27.09.2025.

#ifndef APEXPREDATOR_CGLTF_HELPER_H
#define APEXPREDATOR_CGLTF_HELPER_H

#include "cgltf.h"
#include "cgltf_write.h"

#include "int_def.h"
#include "utils/dynamic_array.h"
#include "utils/string.h"

DYNAMIC_ARRAY_STRUCT(cgltf_mesh, cgltf_mesh);

DYNAMIC_ARRAY_STRUCT(cgltf_node, cgltf_node);

DYNAMIC_ARRAY_STRUCT(cgltf_accessor, cgltf_accessor);

DYNAMIC_ARRAY_STRUCT(cgltf_buffer, cgltf_buffer);

DYNAMIC_ARRAY_STRUCT(cgltf_buffer_view, cgltf_buffer_view);

DYNAMIC_ARRAY_STRUCT(cgltf_attribute, cgltf_attribute);

DYNAMIC_ARRAY_STRUCT(cgltf_material, cgltf_material);

DYNAMIC_ARRAY_STRUCT(uint32, rootNodeIds);

typedef struct { uint32_t v; } GL_ID;
typedef struct { void* v; } TagGL_ID;

#define INVALID_GL_ID (GL_ID){UINT32_MAX}

#define IS_VALID_GL_ID(gl_id) ((gl_id).v != UINT32_MAX)

typedef struct GLTFContext {
    cgltf_data *data;
    cgltf_options options;

    String save_path;

    DynamicArray_cgltf_mesh meshes;
    DynamicArray_cgltf_node nodes;
    DynamicArray_cgltf_accessor accessors;
    DynamicArray_cgltf_buffer buffers;
    DynamicArray_cgltf_buffer_view buffer_views;
    DynamicArray_rootNodeIds scene_node_ids;
    DynamicArray_cgltf_material materials;
    bool finalized;
} GLTFContext;

static inline TagGL_ID gltf_tag_index(GL_ID idx) { return (TagGL_ID){(void*)(uintptr_t)(idx.v + 1u)}; }
static inline GL_ID gltf_untag_index(void *p) { return (GL_ID){((uintptr_t) p) - 1u}; }

char *GLTFContext_dupe_cstring(const char *name);

void GLTFContext_init(GLTFContext *ctx, const char *name);

void GLTFContext_set_save_path(GLTFContext *ctx, const String *path);

void GLTFContext_set_save_cpath(GLTFContext *ctx, const char *path);

void GLTFContext_finalize(GLTFContext *ctx);

void GLTFContext_free(GLTFContext *ctx);

GL_ID GLTFContext_create_buffer(GLTFContext *ctx, const void *data, uint32 data_size, char *name);

GL_ID GLTFContext_create_buffer_and_view(
    GLTFContext *ctx, const void *data, uint32 data_size, char *name,
    cgltf_buffer_view_type type, uint32 stride, uint32 offset);

GL_ID GLTFContext_create_accessor_raw(
    GLTFContext *ctx, GL_ID buffer_view_id, cgltf_type type,
    cgltf_component_type component_type, uint32 count, uint32 offset,
    bool normalized, char *name);

GL_ID GLTFContext_create_accessor_from_data(
    GLTFContext *ctx, const void *data, uint32 data_size,
    uint32 count, char *name, cgltf_type type,
    cgltf_component_type component_type, cgltf_buffer_view_type buffer_type,
    bool normalized, uint32 stride, uint32 offset);

GL_ID GLTFContext_add_node(GLTFContext *ctx, const char *name_opt);

void GLTFContext_node_set_mesh(GLTFContext *ctx, GL_ID node_id, GL_ID mesh_id);

void GLTFContext_node_set_parent(GLTFContext *ctx, GL_ID node_id, GL_ID parent_node_id);

void GLTFContext_node_set_matrix(GLTFContext *ctx, GL_ID node_id, const float *matrix_4x4);

GL_ID GLTFContext_add_mesh(GLTFContext *ctx, const char *name_opt, uint32 primitive_count);

cgltf_primitive *GLTFContext_mesh_get_primitive(GLTFContext *ctx, GL_ID mesh_id, uint32 prim_index);

void GLTFContext_primitive_set_material(GLTFContext *ctx, GL_ID mesh_id, uint32 prim_index, GL_ID material_id);

void GLTFContext_set_primitive_indices_accessor(GLTFContext *ctx, GL_ID mesh_id, uint32 prim_index,
                                                GL_ID accessor_id);

void GLTFContext_primitive_init_attributes(GLTFContext *ctx, GL_ID mesh_id, uint32 prim_index,
                                           uint32 attribute_count);

void GLTFContext_primitive_set_attribute_accessor(GLTFContext *ctx, GL_ID mesh_id, uint32 prim_index,
                                                  uint32 attribute_index, GL_ID accessor_id, const char *name);

bool GLTFContext_write_and_free(GLTFContext *ctx);

GL_ID GLTFContext_add_material(GLTFContext *ctx, const char *name_opt);

GL_ID GLTFContext_find_material_by_name(GLTFContext *ctx, const char *name);

void GLTFContext_node_set_extra(GLTFContext *ctx, GL_ID node_id, const char* data);

// Shortcuts
GL_ID GLTFContext_create_indices_accessor_from_data(
    GLTFContext *ctx, const void *data, uint32 data_size,
    uint32 count, char *name, cgltf_component_type component_type,
    uint32 offset);

#endif //APEXPREDATOR_CGLTF_HELPER_H
