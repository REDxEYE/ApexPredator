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

uint32 gltf_create_accessor_from_data(DynamicArray_cgltf_buffer *buffers, DynamicArray_cgltf_buffer_view *buffer_views,
                                      DynamicArray_cgltf_accessor *accessors,
                                      const void *data, uint32 data_size,
                                      uint32 count, char *name,
                                      cgltf_type type,
                                      cgltf_component_type component_type,
                                      bool normalized, uint32 stride, uint32 offset);

uint32 gltf_create_accessor(DynamicArray_cgltf_accessor *accessors, uint32 buffer_view_id,
                            cgltf_type type, cgltf_component_type component_type,
                            uint32 count, uint32 offset, bool normalized, char *name);

uint32 gltf_create_buffer_and_buffer_view(DynamicArray_cgltf_buffer *buffers,
                                          DynamicArray_cgltf_buffer_view *buffer_views,
                                          const void *data, uint32 data_size,
                                          char *name, cgltf_buffer_view_type type,
                                          uint32 stride, uint32 offset);


uint32 gltf_create_buffer(DynamicArray_cgltf_buffer *buffers, const void *data, uint32 data_size, char *name);

uint32 gltf_create_buffer_view(DynamicArray_cgltf_buffer_view *buffer_views, uint32 buffer_id, char *name,
                               uint32 offset, uint32 size, uint32 stride, cgltf_buffer_view_type type);

void gltf_process_referenced(cgltf_data *data,
                             DynamicArray_cgltf_node *gl_nodes,
                             DynamicArray_cgltf_buffer *gl_buffers,
                             DynamicArray_cgltf_buffer_view *gl_buffer_views,
                             DynamicArray_cgltf_accessor *gl_accessors);

void gltf_free_data(cgltf_data *gltf_data,
                    DynamicArray_cgltf_mesh *gl_meshes,
                    DynamicArray_cgltf_node *gl_nodes,
                    DynamicArray_cgltf_accessor *gl_accessors,
                    DynamicArray_cgltf_buffer_view *gl_buffer_views,
                    DynamicArray_cgltf_buffer *gl_buffers);

#endif //APEXPREDATOR_CGLTF_HELPER_H
