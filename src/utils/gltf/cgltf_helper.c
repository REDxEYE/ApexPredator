// Created by RED on 27.09.2025.

#include "utils/gltf/cgltf_helper.h"

#include "utils/base64.h"
#include "utils/string.h"

#include "assert.h"

char *GLTFContext_dupe_cstring(const char *name) {
    if (name == NULL) return NULL;
    size_t len = strlen(name);
    char *dup = malloc(len + 1);
    memcpy(dup, name, len + 1);
    return dup;
}

void GlTFContext_init(GlTFContext *ctx, const char *name) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->data = calloc(1, sizeof(cgltf_data));
    ctx->data->asset.generator = "ApexPredator via cgltf";
    ctx->data->asset.version = "2.0";

    DA_init(&ctx->meshes, cgltf_mesh, 1);
    DA_init(&ctx->nodes, cgltf_node, 1);
    DA_init(&ctx->accessors, cgltf_accessor, 1);
    DA_init(&ctx->buffers, cgltf_buffer, 1);
    DA_init(&ctx->buffer_views, cgltf_buffer_view, 1);
    DA_init(&ctx->scene_node_ids, uint32, 1);
    DA_init(&ctx->materials, cgltf_material, 1);

    ctx->data->scenes = calloc(1, sizeof(cgltf_scene));
    ctx->data->scenes_count = 1;
    ctx->data->scenes[0].nodes = NULL; // fill later
    ctx->data->scenes[0].nodes_count = 0;
    ctx->data->scene = &ctx->data->scenes[0];
    ctx->data->scene->name = name ? GLTFContext_dupe_cstring(name) : "model";

    ctx->finalized = false;
}

void GLTFContext_finalize(GlTFContext *ctx) {
    // move arrays into cgltf_data
    if (ctx->finalized) {
        printf("[ERROR]: GLTFContext_finalize: already finalized\n");
        exit(1);
    }
    ctx->finalized = true;
    ctx->data->meshes_count = ctx->meshes.count;
    ctx->data->meshes = DA_get_buffer(&ctx->meshes);
    ctx->data->nodes_count = ctx->nodes.count;
    ctx->data->nodes = DA_get_buffer(&ctx->nodes);
    ctx->data->accessors_count = ctx->accessors.count;
    ctx->data->accessors = DA_get_buffer(&ctx->accessors);
    ctx->data->buffers_count = ctx->buffers.count;
    ctx->data->buffers = DA_get_buffer(&ctx->buffers);
    ctx->data->buffer_views_count = ctx->buffer_views.count;
    ctx->data->buffer_views = DA_get_buffer(&ctx->buffer_views);
    ctx->data->materials_count = ctx->materials.count;
    ctx->data->materials = DA_get_buffer(&ctx->materials);

    // fix-up handles → real pointers (no realloc after this point)
    for (uint32 i = 0; i < ctx->data->nodes_count; ++i) {
        cgltf_node *node = &ctx->data->nodes[i];
        if (node->mesh) {
            uint32 idx = gltf_untag_index(node->mesh);
            node->mesh = &ctx->data->meshes[idx];
        }
    }
    for (uint32 i = 0; i < ctx->data->buffer_views_count; ++i) {
        cgltf_buffer_view *bv = &ctx->data->buffer_views[i];
        if (bv->buffer) {
            uint32 idx = gltf_untag_index(bv->buffer);
            bv->buffer = &ctx->data->buffers[idx];
        }
    }
    for (uint32 i = 0; i < ctx->data->accessors_count; ++i) {
        cgltf_accessor *acc = &ctx->data->accessors[i];
        if (acc->buffer_view) {
            uint32 idx = gltf_untag_index(acc->buffer_view);
            acc->buffer_view = &ctx->data->buffer_views[idx];
        }
    }
    for (uint32 i = 0; i < ctx->data->meshes_count; ++i) {
        cgltf_mesh *mesh = &ctx->data->meshes[i];
        for (uint32 j = 0; j < mesh->primitives_count; ++j) {
            cgltf_primitive *prim = &mesh->primitives[j];
            if (prim->material) {
                uint32 idx = gltf_untag_index(prim->material);
                prim->material = &ctx->data->materials[idx];
            }
            if (prim->indices) {
                uint32 idx = gltf_untag_index(prim->indices);
                prim->indices = &ctx->data->accessors[idx];
            }
            for (uint32 k = 0; k < prim->attributes_count; ++k) {
                cgltf_attribute *attr = &prim->attributes[k];
                if (attr->data) {
                    uint32 idx = gltf_untag_index(attr->data);
                    attr->data = &ctx->data->accessors[idx];
                }
            }
        }
    }
    // Untag all node parents
    for (uint32 i = 0; i < ctx->data->nodes_count; ++i) {
        cgltf_node *node = &ctx->data->nodes[i];
        if (node->parent) {
            uint32 idx = gltf_untag_index(node->parent);
            node->parent = &ctx->data->nodes[idx];
        }
    }

    // finalize scene nodes
    if (ctx->scene_node_ids.count > 0) {
        ctx->data->scene->nodes = calloc(ctx->scene_node_ids.count, sizeof(cgltf_node *));
        ctx->data->scene->nodes_count = ctx->scene_node_ids.count;
        for (uint32 i = 0; i < ctx->scene_node_ids.count; ++i) {
            uint32 node_id = ctx->scene_node_ids.items[i];
            assert(node_id < ctx->data->nodes_count);
            ctx->data->scene->nodes[i] = &ctx->data->nodes[node_id];
        }
    } else {
        // if no root nodes specified, use all nodes without parents
        uint32 root_count = 0;
        for (uint32 i = 0; i < ctx->data->nodes_count; ++i) {
            cgltf_node *node = &ctx->data->nodes[i];
            if (node->parent == NULL) {
                root_count++;
            }
        }
        ctx->data->scene->nodes = malloc(sizeof(cgltf_node *) * root_count);
        ctx->data->scene->nodes_count = root_count;
        uint32 root_index = 0;
        for (uint32 i = 0; i < ctx->data->nodes_count; ++i) {
            cgltf_node *node = &ctx->data->nodes[i];
            if (node->parent == NULL) {
                ctx->data->scene->nodes[root_index++] = node;
            }
        }
        assert(root_index == root_count);
    }
}

void GLTFContext_free(GlTFContext *ctx) {
    DA_free_with_inner(&ctx->meshes, {
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

    DA_free_with_inner(&ctx->nodes, {
                       cgltf_node* node = it;
                       if (node->name!=NULL)free(node->name);
                       });
    DA_free(&ctx->accessors);
    DA_free_with_inner(&ctx->buffers, {
                       cgltf_buffer *buf = it;
                       if (buf->data != NULL) free(buf->data);
                       if (buf->uri != NULL) free(buf->uri);
                       });
    DA_free(&ctx->buffer_views);
    DA_free(&ctx->scene_node_ids);

    free(ctx->data->scene[0].nodes);
    free(ctx->data->scenes);
    free(ctx->data);
}

uint32 GLTFContext_create_buffer(GlTFContext *ctx, const void *data, uint32 data_size, char *name) {
    cgltf_buffer *buffer = DA_append_get(&ctx->buffers);
    buffer->size = data_size;
    buffer->data = NULL;
    buffer->name = name;

    String buffer_uri = {0};
    String_from_cstr(&buffer_uri, "data:application/octet-stream;base64,");
    size_t encoded_size = base64_encoded_size(data_size);
    char *base64_data = calloc(1, encoded_size + 1);
    size_t actual_size = base64_encode(data, data_size, base64_data);
    assert(actual_size <= encoded_size);
    String_append_cstr(&buffer_uri, base64_data);
    buffer->uri = String_detach(String_move(&buffer_uri));
    String_free(&buffer_uri);
    free(base64_data);

    return ctx->buffers.count - 1;
}

uint32 GLTFContext_create_buffer_and_view(GlTFContext *ctx, const void *data, uint32 data_size, char *name,
                                          cgltf_buffer_view_type type, uint32 stride, uint32 offset) {
    cgltf_buffer_view *view = DA_append_get(&ctx->buffer_views);
    view->size = data_size;
    view->offset = offset;
    view->stride = stride;
    view->name = name;
    view->type = type;

    uint32 buf_id = GLTFContext_create_buffer(ctx, data, data_size, name);
    view->buffer = gltf_tag_index(buf_id);
    return ctx->buffer_views.count - 1;
}

uint32 GLTFContext_create_accessor_raw(GlTFContext *ctx, uint32 buffer_view_id, cgltf_type type,
                                       cgltf_component_type component_type, uint32 count, uint32 offset,
                                       bool normalized, char *name) {
    cgltf_accessor *acc = DA_append_get(&ctx->accessors);
    acc->component_type = component_type;
    acc->type = type;
    acc->count = count;
    acc->normalized = normalized;
    acc->name = name;
    acc->offset = offset;
    acc->has_min = false;
    acc->has_max = false;
    acc->is_sparse = false;
    acc->buffer_view = gltf_tag_index(buffer_view_id);
    return ctx->accessors.count - 1;
}

uint32 GLTFContext_create_accessor_from_data(GlTFContext *ctx, const void *data, uint32 data_size, uint32 count,
                                             char *name, cgltf_type type, cgltf_component_type component_type,
                                             cgltf_buffer_view_type buffer_type, bool normalized, uint32 stride,
                                             uint32 offset) {
    uint32 view_id = GLTFContext_create_buffer_and_view(ctx, data, data_size, name,
                                                        buffer_type, stride, offset);
    return GLTFContext_create_accessor_raw(ctx, view_id, type, component_type, count, offset, normalized, name);
}

uint32 GLTFContext_add_node(GlTFContext *ctx, const char *name_opt) {
    cgltf_node *n = DA_append_get(&ctx->nodes);
    memset(n, 0, sizeof(*n));
    if (name_opt) {
        n->name = GLTFContext_dupe_cstring(name_opt);
    }
    return ctx->nodes.count - 1;
}

uint32 GLTFContext_add_mesh(GlTFContext *ctx, const char *name_opt, uint32 primitive_count) {
    cgltf_mesh *m = DA_append_get(&ctx->meshes);
    memset(m, 0, sizeof(*m));
    if (name_opt) {
        m->name = GLTFContext_dupe_cstring(name_opt);
    }
    m->primitives = calloc(primitive_count, sizeof(cgltf_primitive));
    m->primitives_count = primitive_count;
    return ctx->meshes.count - 1;
}

void GLTFContext_node_set_mesh(GlTFContext *ctx, uint32 node_id, uint32 mesh_id) {
    cgltf_node *n = &ctx->nodes.items[node_id];
    n->mesh = gltf_tag_index(mesh_id);
}

cgltf_primitive *GLTFContext_mesh_get_primitive(GlTFContext *ctx, uint32 mesh_id, uint32 prim_index) {
    cgltf_mesh *m = &ctx->meshes.items[mesh_id];
    assert(prim_index < m->primitives_count);
    return &m->primitives[prim_index];
}

void GLTFContext_primitive_set_material(GlTFContext *ctx, uint32 mesh_id, uint32 primitive_id, uint32 material_id) {
    cgltf_mesh *m = &ctx->meshes.items[mesh_id];
    assert(primitive_id < m->primitives_count);
    cgltf_primitive *prim = &m->primitives[primitive_id];
    prim->material = gltf_tag_index(material_id);
}

void GlTFContext_set_primitive_indices_accessor(GlTFContext *ctx, uint32 mesh_id, uint32 primitive_id,
                                                uint32 accessor_id) {
    cgltf_mesh *m = &ctx->meshes.items[mesh_id];
    assert(primitive_id < m->primitives_count);
    cgltf_primitive *prim = &m->primitives[primitive_id];
    prim->indices = gltf_tag_index(accessor_id);
}

void GlTFContext_primitive_init_attributes(GlTFContext *ctx, uint32 mesh_id, uint32 primitive_id,
                                           uint32 attribute_count) {
    cgltf_mesh *m = &ctx->meshes.items[mesh_id];
    assert(primitive_id < m->primitives_count);
    cgltf_primitive *prim = &m->primitives[primitive_id];
    prim->attributes = calloc(attribute_count, sizeof(cgltf_attribute));
    prim->attributes_count = attribute_count;
}

void GlTFContext_primitive_set_attribute_accessor(GlTFContext *ctx, uint32 mesh_id, uint32 primitive_id,
                                                  uint32 attribute_id, uint32 accessor_id, const char *name) {
    cgltf_mesh *m = &ctx->meshes.items[mesh_id];
    assert(primitive_id < m->primitives_count);
    cgltf_primitive *prim = &m->primitives[primitive_id];
    assert(attribute_id < prim->attributes_count);
    cgltf_attribute *attr = &prim->attributes[attribute_id];
    attr->data = gltf_tag_index(accessor_id);
    attr->name = GLTFContext_dupe_cstring(name);
}

bool GlTFContext_write_and_free(GlTFContext *ctx, const char *path) {
    GLTFContext_finalize(ctx);
    ctx->options.type = cgltf_file_type_gltf;
    bool ok = (cgltf_write_file(&ctx->options, path, ctx->data) == cgltf_result_success);

    // free (reusing your free helpers)
    GLTFContext_free(ctx);
    memset(ctx, 0, sizeof(*ctx));
    return ok;
}

uint32 GLTFContext_add_material(GlTFContext *ctx, const char *name_opt) {
    cgltf_material *mat = DA_append_get(&ctx->materials);
    memset(mat, 0, sizeof(*mat));
    if (name_opt) {
        mat->name = GLTFContext_dupe_cstring(name_opt);
    }
    return ctx->materials.count - 1;
}

uint32 GlTFContext_find_material_by_name(GlTFContext *ctx, const char *name) {
    for (uint32 i = 0; i < ctx->materials.count; ++i) {
        cgltf_material *mat = &ctx->materials.items[i];
        if (mat->name != NULL && strcmp(mat->name, name) == 0) {
            return i;
        }
    }
    return UINT32_MAX;
}

uint32 GlTFContext_create_indices_accessor_from_data(GlTFContext *ctx, const void *data, uint32 data_size, uint32 count,
                                                     char *name, cgltf_component_type component_type, uint32 offset) {
    return GLTFContext_create_accessor_from_data(ctx, data, data_size, count, name,
        cgltf_type_scalar, component_type, cgltf_buffer_view_type_indices, false, 0, offset);
}
