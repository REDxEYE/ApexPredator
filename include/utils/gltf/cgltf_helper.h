// Created by RED on 27.09.2025.

#ifndef APEXPREDATOR_CGLTF_HELPER_H
#define APEXPREDATOR_CGLTF_HELPER_H

#include "cgltf.h"
#include "cgltf_write.h"

#include "int_def.h"
#include "utils/dynamic_array.h"

DYNAMIC_ARRAY_STRUCT(cgltf_mesh, cgltf_mesh);

DYNAMIC_ARRAY_STRUCT(cgltf_node, cgltf_node);

DYNAMIC_ARRAY_STRUCT(cgltf_accessor, cgltf_accessor);

DYNAMIC_ARRAY_STRUCT(cgltf_buffer, cgltf_buffer);

DYNAMIC_ARRAY_STRUCT(cgltf_buffer_view, cgltf_buffer_view);

DYNAMIC_ARRAY_STRUCT(cgltf_attribute, cgltf_attribute);

DYNAMIC_ARRAY_STRUCT(cgltf_material, cgltf_material);

DYNAMIC_ARRAY_STRUCT(uint32, rootNodeIds);

typedef struct GlTFContext {
    cgltf_data *data;
    cgltf_options options;

    DynamicArray_cgltf_mesh meshes;
    DynamicArray_cgltf_node nodes;
    DynamicArray_cgltf_accessor accessors;
    DynamicArray_cgltf_buffer buffers;
    DynamicArray_cgltf_buffer_view buffer_views;
    DynamicArray_rootNodeIds scene_node_ids;
    DynamicArray_cgltf_material materials;
    bool finalized;
} GlTFContext;

static inline void *gltf_tag_index(uint32 idx) { return (void *) (uintptr_t) (idx + 1u); }
static inline uint32 gltf_untag_index(void *p) { return (uint32) ((uintptr_t) p) - 1u; }

char *GLTFContext_dupe_cstring(const char *name);

void GlTFContext_init(GlTFContext *ctx, const char *name);

void GLTFContext_finalize(GlTFContext *ctx);

void GLTFContext_free(GlTFContext *ctx);

uint32 GLTFContext_create_buffer(GlTFContext *ctx, const void *data, uint32 data_size, char *name);

uint32 GLTFContext_create_buffer_and_view(
    GlTFContext *ctx, const void *data, uint32 data_size, char *name,
    cgltf_buffer_view_type type, uint32 stride, uint32 offset);

uint32 GLTFContext_create_accessor_raw(
    GlTFContext *ctx, uint32 buffer_view_id, cgltf_type type,
    cgltf_component_type component_type, uint32 count, uint32 offset,
    bool normalized, char *name);

uint32 GLTFContext_create_accessor_from_data(
    GlTFContext *ctx, const void *data, uint32 data_size,
    uint32 count, char *name, cgltf_type type,
    cgltf_component_type component_type, cgltf_buffer_view_type buffer_type,
    bool normalized, uint32 stride, uint32 offset);

uint32 GLTFContext_add_node(GlTFContext *ctx, const char *name_opt);

void GLTFContext_node_set_mesh(GlTFContext *ctx, uint32 node_id, uint32 mesh_id);

uint32 GLTFContext_add_mesh(GlTFContext *ctx, const char *name_opt, uint32 primitive_count);

cgltf_primitive *GLTFContext_mesh_get_primitive(GlTFContext *ctx, uint32 mesh_id, uint32 prim_index);

void GLTFContext_primitive_set_material(GlTFContext *ctx, uint32 mesh_id, uint32 primitive_id, uint32 material_id);

void GlTFContext_set_primitive_indices_accessor(GlTFContext *ctx, uint32 mesh_id, uint32 primitive_id,
                                                uint32 accessor_id);

void GlTFContext_primitive_init_attributes(GlTFContext *ctx, uint32 mesh_id, uint32 primitive_id,
                                           uint32 attribute_count);

void GlTFContext_primitive_set_attribute_accessor(GlTFContext *ctx, uint32 mesh_id, uint32 primitive_id,
                                                  uint32 attribute_id, uint32 accessor_id, const char *name);

bool GlTFContext_write_and_free(GlTFContext *ctx, const char *path);

uint32 GLTFContext_add_material(GlTFContext *ctx, const char *name_opt);

uint32 GlTFContext_find_material_by_name(GlTFContext *ctx, const char *name);

// Shortcuts
uint32 GlTFContext_create_indices_accessor_from_data(
    GlTFContext *ctx, const void *data, uint32 data_size,
    uint32 count, char *name, cgltf_component_type component_type,
    uint32 offset);

#endif //APEXPREDATOR_CGLTF_HELPER_H
