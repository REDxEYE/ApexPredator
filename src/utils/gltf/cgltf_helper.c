// Created by RED on 27.09.2025.

#include "utils/gltf/cgltf_helper.h"

#include "utils/base64.h"
#include "utils/string.h"

#include "assert.h"

static String buffer_uri = {0};

uint32 gltf_create_accessor_from_data(DynamicArray_cgltf_buffer *buffers, DynamicArray_cgltf_buffer_view *buffer_views,
                                      DynamicArray_cgltf_accessor *accessors, const void *data, uint32 data_size,
                                      uint32 count, char *name,
                                      cgltf_type type, cgltf_component_type component_type, bool normalized,
                                      uint32 stride, uint32 offset) {
    cgltf_accessor *accessor = DA_append_get(accessors);
    accessor->component_type = component_type;
    accessor->type = type;
    accessor->count = count;
    accessor->normalized = normalized;
    accessor->name = name;
    accessor->offset = offset;
    accessor->has_min = false;
    accessor->has_max = false;
    accessor->is_sparse = false;
    uint32 buffer_view_id = gltf_create_buffer_and_buffer_view(buffers, buffer_views, data, data_size, name,
                                                               cgltf_buffer_view_type_vertices, stride, 0);
    accessor->buffer_view = (void *) (uint64) buffer_view_id;
    return accessors->count - 1;
}

uint32 gltf_create_accessor(DynamicArray_cgltf_accessor *accessors, uint32 buffer_view_id,
                            cgltf_type type, cgltf_component_type component_type,
                            uint32 count, uint32 offset, bool normalized, char *name) {
    cgltf_accessor *accessor = DA_append_get(accessors);
    accessor->component_type = component_type;
    accessor->type = type;
    accessor->count = count;
    accessor->normalized = normalized;
    accessor->name = name;
    accessor->offset = offset;
    accessor->has_min = false;
    accessor->has_max = false;
    accessor->is_sparse = false;
    accessor->buffer_view = (void *) (uint64) buffer_view_id;
    return accessors->count - 1;
}

uint32 gltf_create_buffer_and_buffer_view(
    DynamicArray_cgltf_buffer *buffers, DynamicArray_cgltf_buffer_view *buffer_views,
    const void *data, uint32 data_size, char *name,
    cgltf_buffer_view_type type, uint32 stride, uint32 offset) {
    cgltf_buffer_view *buffer_view = DA_append_get(buffer_views);

    buffer_view->size = data_size;
    buffer_view->offset = offset;
    buffer_view->stride = stride;
    buffer_view->name = name;
    buffer_view->type = type;
    buffer_view->buffer = (void *) (uint64) gltf_create_buffer(buffers, data, data_size, name);


    return buffer_views->count - 1;
}

uint32 gltf_create_buffer(DynamicArray_cgltf_buffer *buffers, const void *data, uint32 data_size, char *name) {
    cgltf_buffer *buffer = DA_append_get(buffers);
    buffer->size = data_size;
    buffer->data = NULL;
    buffer->name = name;

    String_from_cstr(&buffer_uri, "data:application/octet-stream;base64,");

    size_t encoded_size = base64_encoded_size(data_size);
    char *base64_data = calloc(1, encoded_size + 1);
    size_t actual_size = base64_encode(data, data_size, base64_data);
    assert(actual_size <= encoded_size);
    String_append_cstr(&buffer_uri, base64_data);
    buffer->uri = String_detach(String_move(&buffer_uri));

    String_free(&buffer_uri);
    free(base64_data);

    return buffers->count - 1;
}

void gltf_process_referenced(cgltf_data *data,
                             DynamicArray_cgltf_node *gl_nodes,
                             DynamicArray_cgltf_buffer *gl_buffers,
                             DynamicArray_cgltf_buffer_view *gl_buffer_views,
                             DynamicArray_cgltf_accessor *gl_accessors
) {
    for (uint32 i = 0; i < data->nodes_count; i++) {
        cgltf_node *node = &data->nodes[i];
        if (node->mesh) {
            uint32 mesh_index = ((uint64) (node->mesh) & 0xFFFFFFFF) - 1;
            node->mesh = &data->meshes[mesh_index];
        }
    }

    for (uint32 i = 0; i < data->buffer_views_count; i++) {
        cgltf_buffer_view *buffer = &data->buffer_views[i];
        uint32 buffer_index = (uint64) (buffer->buffer) & 0xFFFFFFFF;
        buffer->buffer = DA_at(gl_buffers, buffer_index);
    }
    for (uint32 i = 0; i < data->accessors_count; i++) {
        cgltf_accessor *accessor = &data->accessors[i];
        uint32 buffer_index = (uint64) (accessor->buffer_view) & 0xFFFFFFFF;
        accessor->buffer_view = DA_at(gl_buffer_views, buffer_index);
    }

    for (uint32 i = 0; i < data->meshes_count; i++) {
        cgltf_mesh *mesh = &data->meshes[i];
        for (uint32 j = 0; j < mesh->primitives_count; j++) {
            cgltf_primitive *prim = &mesh->primitives[j];

            uint32 accessor_index = (uint64) (prim->indices) & 0xFFFFFFFF;
            prim->indices = DA_at(gl_accessors, accessor_index);
            if (prim->attributes_count) {
                for (uint32 k = 0; k < prim->attributes_count; k++) {
                    cgltf_attribute *attr = &prim->attributes[k];
                    attr->data = DA_at(gl_accessors, (uint64) (attr->data) & 0xFFFFFFFF);
                }
            }
        }
    }
}

void gltf_free_data(cgltf_data *gltf_data, DynamicArray_cgltf_mesh *gl_meshes, DynamicArray_cgltf_node *gl_nodes,
                    DynamicArray_cgltf_accessor *gl_accessors, DynamicArray_cgltf_buffer_view *gl_buffer_views,
                    DynamicArray_cgltf_buffer *gl_buffers) {
    DA_free_with_inner(gl_meshes, {
                       cgltf_mesh* mesh = it;
                       if (mesh->name!=NULL)free(mesh->name);
                       for (uint32 i =0;i<mesh->primitives_count;i++) {
                       cgltf_primitive* primitive = &mesh->primitives[i];
                       if (primitive->attributes) {
                       for (uint32 j=0;j<primitive->attributes_count;j++) {
                       cgltf_attribute* attribute = &primitive->attributes[j];
                       if (attribute->name!=NULL)free(attribute->name);
                       }
                       free(primitive->attributes);
                       }
                       }
                       free(mesh->primitives);});

    DA_free_with_inner(gl_nodes, {
                          cgltf_node* node = it;
                          if (node->name!=NULL)free(node->name);
    });
    DA_free(gl_accessors);
    DA_free_with_inner(gl_buffers, {
                       cgltf_buffer *buf = it;
                       if (buf->data != NULL) free(buf->data);
                       if (buf->uri != NULL) free(buf->uri);
                       });
    DA_free(gl_buffer_views);

    free(gltf_data->scene[0].nodes);
    free(gltf_data->scenes);
    free(gltf_data);
}
