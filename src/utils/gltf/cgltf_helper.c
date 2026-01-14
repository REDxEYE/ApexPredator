// Created by RED on 27.09.2025.

#include "utils/gltf/cgltf_helper.h"

#include <sys/stat.h>

#include "utils/base64.h"
#include "utils/string.h"

#include "assert.h"
#include "platform/texture.h"
#include "utils/hash_helper.h"
#include "utils/path.h"

char *GLTFContext_dupe_cstring(const char *name) {
    if (name == NULL) return NULL;
    const size_t len = strlen(name);
    char *dup = malloc(len + 1);
    if (dup == NULL) {
        fprintf(stderr, "[ERROR]: GLTFContext_dupe_cstring: failed to allocate memory, bailing out!\n");
        exit(1);
    }
    memcpy(dup, name, len + 1);
    return dup;
}

void GLTFContext_init(GLTFContext *ctx, const char *name) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->data = calloc(1, sizeof(cgltf_data));
    ctx->data->asset.generator = "ApexPredator via cgltf";
    ctx->data->asset.version = "2.0";
    String_init(&ctx->save_path, 256);
    DA_init(&ctx->meshes, cgltf_mesh, 1);
    DA_init(&ctx->nodes, cgltf_node, 1);
    DA_init(&ctx->accessors, cgltf_accessor, 1);
    DA_init(&ctx->buffers, cgltf_buffer, 1);
    DA_init(&ctx->buffer_views, cgltf_buffer_view, 1);
    DA_init(&ctx->scene_node_ids, uint32, 1);
    DA_init(&ctx->materials, cgltf_material, 1);
    DA_init(&ctx->textures, cgltf_texture, 1);
    DA_init(&ctx->images, cgltf_image, 1);
    DA_init(&ctx->skins, cgltf_skin, 1);
    DA_init(&ctx->skin_stack, GL_ID, MAX_GLTFCONTEXT_SKIN_STACK_DEPTH);
    DA_init(&ctx->raw_buffers, DynamicArray_uint8, 1);

    ctx->data->scenes = calloc(1, sizeof(cgltf_scene));
    ctx->data->scenes_count = 1;
    ctx->data->scenes[0].nodes = NULL; // fill later
    ctx->data->scenes[0].nodes_count = 0;
    ctx->data->scene = &ctx->data->scenes[0];
    ctx->data->scene->name = name ? GLTFContext_dupe_cstring(name) : GLTFContext_dupe_cstring("model");

    ctx->finalized = false;
}

void GLTFContext_set_save_path(GLTFContext *ctx, const String *path) {
    if (ctx->save_path.size != 0) {
        return;
    }
    String_copy_from(&ctx->save_path, path);
}

void GLTFContext_set_save_cpath(GLTFContext *ctx, const char *path) {
    if (ctx->save_path.size != 0) {
        return;
    }
    String_from_cstr(&ctx->save_path, path);
}

void GLTFContext_finalize(GLTFContext *ctx) {
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
    ctx->data->textures_count = ctx->textures.count;
    ctx->data->textures = DA_get_buffer(&ctx->textures);
    ctx->data->images_count = ctx->images.count;
    ctx->data->images = DA_get_buffer(&ctx->images);
    ctx->data->skins = DA_get_buffer(&ctx->skins);
    ctx->data->skins_count = ctx->skins.count;

    // fix-up handles → real pointers (no realloc after this point)
    for (uint32 i = 0; i < ctx->data->nodes_count; ++i) {
        cgltf_node *node = &ctx->data->nodes[i];
        if (node->mesh) {
            GL_ID idx = gltf_untag_index(node->mesh);
            node->mesh = &ctx->data->meshes[idx.v];
        }
    }
    for (uint32 i = 0; i < ctx->data->buffer_views_count; ++i) {
        cgltf_buffer_view *bv = &ctx->data->buffer_views[i];
        if (bv->buffer) {
            GL_ID idx = gltf_untag_index(bv->buffer);
            bv->buffer = &ctx->data->buffers[idx.v];
        }
    }
    for (uint32 i = 0; i < ctx->data->accessors_count; ++i) {
        cgltf_accessor *acc = &ctx->data->accessors[i];
        if (acc->buffer_view) {
            GL_ID idx = gltf_untag_index(acc->buffer_view);
            acc->buffer_view = &ctx->data->buffer_views[idx.v];
        }
    }
    for (int i = 0; i < ctx->data->textures_count; ++i) {
        cgltf_texture *tex = &ctx->data->textures[i];
        if (tex->image) {
            GL_ID idx = gltf_untag_index(tex->image);
            tex->image = &ctx->data->images[idx.v];
        }
    }
    for (int i = 0; i < ctx->data->images_count; ++i) {
        cgltf_image *img = &ctx->data->images[i];
        if (img->buffer_view) {
            GL_ID idx = gltf_untag_index(img->buffer_view);
            img->buffer_view = &ctx->data->buffer_views[idx.v];
        }
    }
    for (uint32 i = 0; i < ctx->data->meshes_count; ++i) {
        cgltf_mesh *mesh = &ctx->data->meshes[i];
        for (uint32 j = 0; j < mesh->primitives_count; ++j) {
            cgltf_primitive *prim = &mesh->primitives[j];
            if (prim->material) {
                GL_ID idx = gltf_untag_index(prim->material);
                prim->material = &ctx->data->materials[idx.v];
            }
            if (prim->indices) {
                GL_ID idx = gltf_untag_index(prim->indices);
                prim->indices = &ctx->data->accessors[idx.v];
            }
            for (uint32 k = 0; k < prim->attributes_count; ++k) {
                cgltf_attribute *attr = &prim->attributes[k];
                if (attr->data) {
                    GL_ID idx = gltf_untag_index(attr->data);
                    attr->data = &ctx->data->accessors[idx.v];
                }
            }
        }
    }
    for (int i = 0; i < ctx->skins.count; ++i) {
        cgltf_skin *skin = &ctx->data->skins[i];
        for (uint32 j = 0; j < skin->joints_count; ++j) {
            GL_ID idx = gltf_untag_index(skin->joints[j]);
            skin->joints[j] = &ctx->data->nodes[idx.v];
        }
        if (skin->skeleton) {
            GL_ID idx = gltf_untag_index(skin->skeleton);
            skin->skeleton = &ctx->data->nodes[idx.v];
        }
        //Untag inverse matrices
        if (skin->inverse_bind_matrices) {
            GL_ID idx = gltf_untag_index(skin->inverse_bind_matrices);
            skin->inverse_bind_matrices = &ctx->data->accessors[idx.v];
        }
    }
    for (int i = 0; i < ctx->data->materials_count; ++i) {
        cgltf_material *mat = &ctx->data->materials[i];
        if (mat->has_pbr_metallic_roughness) {
            cgltf_pbr_metallic_roughness *pbr = &mat->pbr_metallic_roughness;
            if (pbr->base_color_texture.texture) {
                GL_ID idx = gltf_untag_index(pbr->base_color_texture.texture);
                pbr->base_color_texture.texture = &ctx->data->textures[idx.v];
            }
            if (pbr->metallic_roughness_texture.texture) {
                GL_ID idx = gltf_untag_index(pbr->metallic_roughness_texture.texture);
                pbr->metallic_roughness_texture.texture = &ctx->data->textures[idx.v];
            }
        }
        if (mat->normal_texture.texture) {
            GL_ID idx = gltf_untag_index(mat->normal_texture.texture);
            mat->normal_texture.texture = &ctx->data->textures[idx.v];
        }
        if (mat->occlusion_texture.texture) {
            GL_ID idx = gltf_untag_index(mat->occlusion_texture.texture);
            mat->occlusion_texture.texture = &ctx->data->textures[idx.v];
        }
        if (mat->emissive_texture.texture) {
            GL_ID idx = gltf_untag_index(mat->emissive_texture.texture);
            mat->emissive_texture.texture = &ctx->data->textures[idx.v];
        }
    }

    // Untag all node parents
    for (uint32 i = 0; i < ctx->data->nodes_count; ++i) {
        cgltf_node *node = &ctx->data->nodes[i];
        if (node->parent) {
            const GL_ID idx = gltf_untag_index(node->parent);
            node->parent = &ctx->data->nodes[idx.v];
        }
        //Untag child list
        if (node->children) {
            for (uint32 j = 0; j < node->children_count; ++j) {
                const GL_ID idx = gltf_untag_index(node->children[j]);
                node->children[j] = &ctx->data->nodes[idx.v];
            }
        }
        //Untag skin
        if (node->skin) {
            const GL_ID idx = gltf_untag_index(node->skin);
            node->skin = &ctx->data->skins[idx.v];
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

bool GLTFContext_write_and_free(GLTFContext *ctx) {
    bool ok;
    if (ctx->meshes.count > 0 || ctx->nodes.count > 0) {
        if (ctx->save_path.size == 0) {
            printf("[ERROR]: GLTFContext_write_and_free: no save path set\n");
            exit(1);
        }
        GLTFContext_finalize(ctx);
        bool has_big_buffers = false;
        for (int i = 0; i < ctx->data->buffers_count; ++i) {
            cgltf_buffer *buffer = &ctx->data->buffers[i];
            const D_ID raw_data_id = gltf_untag_data_id(buffer->data);
            if (IS_VALID_D_ID(raw_data_id)) {
                const DynamicArray_uint8 *raw_data = &ctx->raw_buffers.items[raw_data_id.v];
                if (raw_data->count > 100 * 1024) {
                    has_big_buffers = true;
                    break;
                }
            }
        }
        if (has_big_buffers) {
            String data_dir = {0};
            Path_get_parent(&ctx->save_path, &data_dir);
            String gltf_name = {0};
            Path_filename(&ctx->save_path, &gltf_name);
            String_append_format(&data_dir, "/%s_data", String_data(&gltf_name));
            Path_ensure_dirs(&data_dir);

            // Loop over buffers, named(.bin extension) buffer or png files larger 100kb get written into "<gltf name>_data" subdirectory next to exported gltf, other get base64 encoded into URI
            for (int i = 0; i < ctx->data->buffers_count; ++i) {
                cgltf_buffer *buffer = &ctx->data->buffers[i];

                const D_ID raw_data_id = gltf_untag_data_id(buffer->data);
                if (IS_VALID_D_ID(raw_data_id)) {
                    String buffer_name = {0};
                    String_from_cstr(&buffer_name, buffer->name);
                    const DynamicArray_uint8 *raw_data = &ctx->raw_buffers.items[raw_data_id.v];
                    if (raw_data->count <= 100 * 1024 || (
                            !String_cends_with(&buffer_name, ".png") && !String_cends_with(&buffer_name, ".bin"))) {
                        String buffer_uri = {0};
                        String_from_cstr(&buffer_uri, "data:application/octet-stream;base64,");
                        const size_t encoded_size = base64_encoded_size(raw_data->count);
                        char *base64_data = calloc(1, encoded_size + 1);
                        const size_t actual_size = base64_encode(DA_get_buffer(raw_data), raw_data->count, base64_data);
                        assert(actual_size <= encoded_size);
                        String_append_cstr(&buffer_uri, base64_data);
                        buffer->uri = String_detach(String_move(&buffer_uri));
                        String_free(&buffer_uri);
                        free(base64_data);
                    } else {
                        String bin_name = {0};
                        String_from_cstr(&bin_name, buffer->name);
                        Path_replace_invalid_fs_chars(&bin_name, '_');
                        String buffer_path = {0};
                        Path_join(&buffer_path, &data_dir);
                        Path_join(&buffer_path, &bin_name);
                        Path_ensure_parent_dirs(&buffer_path);

                        FILE *f = fopen(String_data(&buffer_path), "wb");
                        if (f == NULL) {
                            printf("[ERROR]: GLTFContext_write_and_free: failed to open buffer file for writing: %s\n",
                                   String_data(&buffer_path));
                            exit(1);
                        }
                        fwrite(DA_get_buffer(raw_data), 1, raw_data->count, f);
                        fclose(f);
                        String_prepend_format(&bin_name, "%s_data/", String_data(&gltf_name));
                        buffer->uri = String_detach(String_move(&bin_name));
                        String_free(&buffer_path);
                        String_free(&bin_name);
                    }
                    buffer->data = NULL; // cleanup after dispathing
                    String_free(&buffer_name);
                }
            }
        }
        ctx->options.type = cgltf_file_type_gltf;
        printf("[INFO]: GLTF save path: %s\n", String_data(&ctx->save_path));
        ok = (cgltf_write_file(&ctx->options, String_data(&ctx->save_path), ctx->data) == cgltf_result_success);
    } else {
        ok = true;
    }
    GLTFContext_free(ctx);
    memset(ctx, 0, sizeof(*ctx));
    return ok;
}

void GLTFContext_free(GLTFContext *ctx) {
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
                       if (node->extras.data!=NULL)free(node->extras.data);
                       });
    DA_free(&ctx->accessors);
    DA_free_with_inner(&ctx->buffers, {
                       const cgltf_buffer *buf = it;
                       if (buf->data != NULL) free(buf->data);
                       if (buf->uri != NULL) free(buf->uri);
                       });
    DA_free(&ctx->buffer_views);
    DA_free(&ctx->scene_node_ids);
    DA_free_with_inner(&ctx->materials, {
                       cgltf_material* mat = it;
                       if (mat->name!=NULL)free(mat->name);
                       });
    DA_free_with_inner(&ctx->textures, {
                       cgltf_texture* tex = it;
                       if (tex->name!=NULL)free(tex->name);
                       });
    DA_free_with_inner(&ctx->images, {
                       cgltf_image* img = it;
                       if (img->name!=NULL)free(img->name);
                       if (img->uri!=NULL)free(img->uri);
                       });
    DA_free_with_inner(&ctx->skins, {
                       cgltf_skin* skin = it;
                       if (skin->joints!=NULL)free(skin->joints);
                       if (skin->name!=NULL)free(skin->name);
                       });

    DA_free(&ctx->skin_stack);
    DA_free_with_inner(&ctx->raw_buffers, {
                       DynamicArray_uint8* buf = it;
                       DA_free(buf);
                       });

    String_free(&ctx->save_path);
    for (int i = 0; i < ctx->data->scenes_count; ++i) {
        if (ctx->data->scenes[i].nodes != NULL) free(ctx->data->scenes[i].nodes);
        if (ctx->data->scenes[i].name != NULL) free(ctx->data->scenes[i].name);
    }
    free(ctx->data->scenes);
    free(ctx->data);
}

GL_ID GLTFContext_create_buffer(GLTFContext *ctx, const void *data, uint32 data_size, const char *name) {
    cgltf_buffer *buffer = DA_append_get(&ctx->buffers);
    buffer->size = data_size;
    buffer->data = NULL;
    buffer->name = GLTFContext_dupe_cstring(name);

    DynamicArray_uint8 *raw_buffer = DA_append_get(&ctx->raw_buffers);
    D_ID raw_buffer_id = {ctx->raw_buffers.count - 1};

    DA_init(raw_buffer, uint8, data_size);
    DA_reserve(raw_buffer, data_size);
    memcpy(DA_get_buffer(raw_buffer), data, data_size);
    raw_buffer->count = data_size;

    buffer->data = gltf_tag_data_id(raw_buffer_id).v;
    return (GL_ID){ctx->buffers.count - 1};
}

GL_ID GLTFContext_create_buffer_and_view(GLTFContext *ctx, const void *data, const uint32 data_size, const char *name,
                                         const cgltf_buffer_view_type type, const uint32 stride, const uint32 offset) {
    cgltf_buffer_view *view = DA_append_get(&ctx->buffer_views);
    view->size = data_size;
    view->offset = offset;
    view->stride = stride;
    view->name = GLTFContext_dupe_cstring(name);
    view->type = type;

    GL_ID buf_id = GLTFContext_create_buffer(ctx, data, data_size, name);
    view->buffer = gltf_tag_index(buf_id).v;
    return (GL_ID){ctx->buffer_views.count - 1};
}

GL_ID GLTFContext_accessor_add(GLTFContext *ctx, GL_ID buffer_view_id, cgltf_type type,
                               cgltf_component_type component_type, uint32 count, uint32 offset,
                               bool normalized, const char *name) {
    cgltf_accessor *acc = DA_append_get(&ctx->accessors);
    acc->component_type = component_type;
    acc->type = type;
    acc->count = count;
    acc->normalized = normalized;
    acc->name = GLTFContext_dupe_cstring(name);
    acc->offset = offset;
    acc->has_min = false;
    acc->has_max = false;
    acc->is_sparse = false;
    acc->buffer_view = gltf_tag_index(buffer_view_id).v;
    return (GL_ID){ctx->accessors.count - 1};
}

GL_ID GLTFContext_accessor_from_data(GLTFContext *ctx, const void *data, uint32 data_size, uint32 count,
                                     const char *name, cgltf_type type, cgltf_component_type component_type,
                                     cgltf_buffer_view_type buffer_type, bool normalized, uint32 stride,
                                     uint32 offset) {
    const GL_ID view_id = GLTFContext_create_buffer_and_view(ctx, data, data_size, name,
                                                             buffer_type, stride, offset);
    return GLTFContext_accessor_add(ctx, view_id, type, component_type, count, offset, normalized, name);
}

GL_ID GLTFContext_node_add(GLTFContext *ctx, const char *name_opt) {
    cgltf_node *n = DA_append_get(&ctx->nodes);
    memset(n, 0, sizeof(*n));
    if (name_opt) {
        n->name = GLTFContext_dupe_cstring(name_opt);
    }
    return (GL_ID){ctx->nodes.count - 1};
}

void GLTFContext_node_set_mesh(GLTFContext *ctx, GL_ID node_id, GL_ID mesh_id) {
    cgltf_node *n = &ctx->nodes.items[node_id.v];
    n->mesh = gltf_tag_index(mesh_id).v;
}

void GLTFContext_node_set_parent(GLTFContext *ctx, const GL_ID node_id, const GL_ID parent_node_id) {
    cgltf_node *n = &ctx->nodes.items[node_id.v];
    n->parent = gltf_tag_index(parent_node_id).v;

    cgltf_node *parent = &ctx->nodes.items[parent_node_id.v];
    if (parent->children == NULL) {
        parent->children = malloc(sizeof(cgltf_node *));
        parent->children_count = 1;
        parent->children[0] = gltf_tag_index(node_id).v;
    } else {
        parent->children = realloc(parent->children, sizeof(cgltf_node *) * (parent->children_count + 1));
        parent->children[parent->children_count] = gltf_tag_index(node_id).v;
        parent->children_count += 1;
    }
}

void GLTFContext_node_set_matrix(GLTFContext *ctx, GL_ID node_id, const float *matrix_4x4) {
    cgltf_node *n = &ctx->nodes.items[node_id.v];
    memcpy(n->matrix, matrix_4x4, sizeof(n->matrix));
    n->has_matrix = true;
}

GL_ID GLTFContext_mesh_add(GLTFContext *ctx, const char *name_opt, uint32 primitive_count) {
    cgltf_mesh *m = DA_append_get(&ctx->meshes);
    memset(m, 0, sizeof(*m));
    if (name_opt) {
        m->name = GLTFContext_dupe_cstring(name_opt);
    }
    m->primitives = calloc(primitive_count, sizeof(cgltf_primitive));
    m->primitives_count = primitive_count;
    return (GL_ID){ctx->meshes.count - 1};
}

cgltf_primitive *GLTFContext_mesh_get_primitive(GLTFContext *ctx, GL_ID mesh_id, uint32 prim_index) {
    cgltf_mesh *m = &ctx->meshes.items[mesh_id.v];
    assert(prim_index < m->primitives_count);
    return &m->primitives[prim_index];
}

void GLTFContext_node_set_skin(GLTFContext *ctx, GL_ID node_id, GL_ID skin_id) {
    cgltf_node *n = &ctx->nodes.items[node_id.v];
    n->skin = gltf_tag_index(skin_id).v;
}

void GLTFContext_primitive_set_material(GLTFContext *ctx, GL_ID mesh_id, uint32 primitive_id, GL_ID material_id) {
    cgltf_mesh *m = &ctx->meshes.items[mesh_id.v];
    assert(primitive_id < m->primitives_count);
    cgltf_primitive *prim = &m->primitives[primitive_id];
    prim->material = gltf_tag_index(material_id).v;
}

void GLTFContext_set_primitive_indices_accessor(GLTFContext *ctx, GL_ID mesh_id, uint32 primitive_id,
                                                GL_ID accessor_id) {
    cgltf_mesh *m = &ctx->meshes.items[mesh_id.v];
    assert(primitive_id < m->primitives_count);
    cgltf_primitive *prim = &m->primitives[primitive_id];
    prim->indices = gltf_tag_index(accessor_id).v;
}

void GLTFContext_primitive_init_attributes(GLTFContext *ctx, GL_ID mesh_id, uint32 prim_index,
                                           uint32 attribute_count) {
    cgltf_mesh *m = &ctx->meshes.items[mesh_id.v];
    assert(prim_index < m->primitives_count);
    cgltf_primitive *prim = &m->primitives[prim_index];
    prim->attributes = calloc(attribute_count, sizeof(cgltf_attribute));
    prim->attributes_count = attribute_count;
}

void GLTFContext_accessor_set_minmax(GLTFContext *ctx, const GL_ID accessor_id, const float *min_values,
                                     const float *max_values) {
    cgltf_accessor *acc = &ctx->accessors.items[accessor_id.v];
    uint32 component_count = 0;
    switch (acc->type) {
        case cgltf_type_scalar: component_count = 1;
            break;
        case cgltf_type_vec2: component_count = 2;
            break;
        case cgltf_type_vec3: component_count = 3;
            break;
        case cgltf_type_vec4:
        case cgltf_type_mat2: component_count = 4;
            break;
        case cgltf_type_mat3: component_count = 9;
            break;
        case cgltf_type_mat4: component_count = 16;
            break;
        default: component_count = 0;
            break;
    }
    if (component_count > 0) {
        memcpy(acc->min, min_values, sizeof(float) * component_count);
        memcpy(acc->max, max_values, sizeof(float) * component_count);
        acc->has_min = true;
        acc->has_max = true;
    }
}

void GLTFContext_primitive_set_attribute_accessor(GLTFContext *ctx, const GL_ID mesh_id, const uint32 primitive_id,
                                                  const uint32 attribute_id, const GL_ID accessor_id,
                                                  const char *name) {
    const cgltf_mesh *m = &ctx->meshes.items[mesh_id.v];
    assert(primitive_id < m->primitives_count);
    const cgltf_primitive *prim = &m->primitives[primitive_id];
    assert(attribute_id < prim->attributes_count);
    cgltf_attribute *attr = &prim->attributes[attribute_id];
    attr->data = gltf_tag_index(accessor_id).v;
    attr->name = GLTFContext_dupe_cstring(name);
}


GL_ID GLTFContext_image_new(GLTFContext *ctx, const char *name_opt) {
    cgltf_image *img = DA_append_get(&ctx->images);
    memset(img, 0, sizeof(*img));
    if (name_opt) {
        img->name = GLTFContext_dupe_cstring(name_opt);
    }
    return (GL_ID){ctx->images.count - 1};
}

void GLTFContext_image_set_mimetype(const GLTFContext *ctx, const GL_ID image_id, const char *mimetype) {
    cgltf_image *img = &ctx->images.items[image_id.v];
    img->mime_type = GLTFContext_dupe_cstring(mimetype);
}

void GLTFContext_image_set_buffer_view(const GLTFContext *ctx, const GL_ID image_id, const GL_ID buffer_view_id) {
    cgltf_image *img = &ctx->images.items[image_id.v];
    img->buffer_view = gltf_tag_index(buffer_view_id).v;
}

// void GLTFContext_image_set_data(const GLTFContext *ctx, const GL_ID image_id, const D_ID data_id) {
//     cgltf_image *img = &ctx->images.items[image_id.v];
//     img->uri = gltf_tag_data_id(data_id);
// }

// void GLTFContext_image_set_base64_data(const GLTFContext *ctx, const GL_ID image_id, const void *data, const uint32 data_size) {
//     cgltf_image *img = &ctx->images.items[image_id.v];
//
//     String buffer_uri = {0};
//     String_from_cstr(&buffer_uri, "data:");
//     String_append_cstr(&buffer_uri, img->mime_type);
//     String_append_cstr(&buffer_uri, ";base64,");
//     const size_t encoded_size = base64_encoded_size(data_size);
//     char *base64_data = calloc(1, encoded_size + 1);
//     const size_t actual_size = base64_encode(data, data_size, base64_data);
//     assert(actual_size <= encoded_size);
//     String_append_cstr(&buffer_uri, base64_data);
//     img->uri = String_detach(String_move(&buffer_uri));
//     String_free(&buffer_uri);
//     free(base64_data);
// }

// void GLTFContext_image_set_url(GLTFContext *ctx, const GL_ID image_id, const char *url) {
//     cgltf_image *img = &ctx->images.items[image_id.v];
//     img->uri = GLTFContext_dupe_cstring(url);
// }
//
// void GLTFContext_image_set_buffer_view(GLTFContext *ctx, GL_ID image_id, GL_ID buffer_view_id) {
//     cgltf_image *img = &ctx->images.items[image_id.v];
//     img->buffer_view = gltf_tag_index(buffer_view_id).v;
// }

GL_ID GLTFContext_texture_new(GLTFContext *ctx, const char *name_opt) {
    cgltf_texture *tex = DA_append_get(&ctx->textures);
    memset(tex, 0, sizeof(*tex));
    if (name_opt) {
        tex->name = GLTFContext_dupe_cstring(name_opt);
    }
    return (GL_ID){ctx->textures.count - 1};
}

GL_ID GLTFContext_texture_new_with_image(GLTFContext *ctx, const char *name_opt, GL_ID image_id) {
    cgltf_texture *tex = DA_append_get(&ctx->textures);
    memset(tex, 0, sizeof(*tex));
    if (name_opt) {
        tex->name = GLTFContext_dupe_cstring(name_opt);
    }
    tex->image = gltf_tag_index(image_id).v;
    return (GL_ID){ctx->textures.count - 1};
}

GL_ID GLTFContext_material_new(GLTFContext *ctx, const char *name_opt) {
    cgltf_material *mat = DA_append_get(&ctx->materials);
    memset(mat, 0, sizeof(*mat));
    if (name_opt) {
        mat->name = GLTFContext_dupe_cstring(name_opt);
    }
    return (GL_ID){ctx->materials.count - 1};
}

bool GLTFContext_material_diffuse_present(const GLTFContext *ctx, const GL_ID material_id) {
    const cgltf_material *mat = &ctx->materials.items[material_id.v];
    return mat->pbr_metallic_roughness.base_color_texture.texture != NULL;
}

GL_ID gltf_texture_from_texture(GLTFContext *ctx, const String *original_path, const Texture *texture) {
    const uint32 hash = hash_string(original_path);
    String tex_name = {0};
    Path_filename(original_path, &tex_name);

    String unique_name = {0};
    String_format(&unique_name, "%s_%08X.png", String_data(&tex_name), hash);

    size_t data_size = 0;
    uint32 channel_count = 0;
    uint8 *data = Texture_write_png_to_memory(texture, &channel_count, &data_size);

    const GL_ID image_id = GLTFContext_image_new(ctx, String_data(&unique_name));
    GLTFContext_image_set_mimetype(ctx, image_id, "image/png");
    const GL_ID buffer_view = GLTFContext_create_buffer_and_view(ctx, data, data_size, String_data(&unique_name),
                                                                 cgltf_buffer_view_type_invalid, 0, 0);
    free(data);
    GLTFContext_image_set_buffer_view(ctx, image_id, buffer_view);
    const GL_ID tex_id = GLTFContext_texture_new_with_image(ctx, String_data(&unique_name), image_id);
    String_free(&unique_name);
    return tex_id;
}

void GLTFContext_material_set_diffuse_texture_from_data(GLTFContext *ctx, const String *original_path,
                                                        const GL_ID material_id,
                                                        const Texture *texture) {
    if (texture->channel_count <= 3 || texture->channel_count > 4) {
        printf("Invalid texture channel count for diffuse map!\n");
        return;
    }

    const GL_ID tex_id = gltf_texture_from_texture(ctx, original_path, texture);
    cgltf_material *mat = &ctx->materials.items[material_id.v];
    mat->pbr_metallic_roughness.base_color_texture.texture = gltf_tag_index(tex_id).v;
}

void GLTFContext_material_set_normal_from_data(GLTFContext *ctx, const String *original_path, GL_ID material_id,
                                               const Texture *texture) {
    if (texture->channel_count <= 3 || texture->channel_count > 4) {
        printf("Invalid texture channel count for normal map!\n");
        return;
    }

    const GL_ID tex_id = gltf_texture_from_texture(ctx, original_path, texture);
    cgltf_material *mat = &ctx->materials.items[material_id.v];
    mat->normal_texture.texture = gltf_tag_index(tex_id).v;
}

void GLTFContext_material_set_roughness_metallic_from_data(GLTFContext *ctx, const String *original_path,
                                                           GL_ID material_id,
                                                           const Texture *texture) {
    if (texture->channel_count < 3) {
        printf("Invalid texture channel count for roughness/metallic map!\n");
        return;
    }
    const GL_ID tex_id = gltf_texture_from_texture(ctx, original_path, texture);
    cgltf_material *mat = &ctx->materials.items[material_id.v];
    mat->pbr_metallic_roughness.metallic_roughness_texture.texture = gltf_tag_index(tex_id).v;
}

void GLTFContext_material_set_emissive_from_data(GLTFContext *ctx, const String *original_path, GL_ID material_id,
    const Texture *texture) {
    // if (texture->channel_count) {
        // printf("Invalid texture channel count for emissive map!\n");
        // return;
    // }
    const GL_ID tex_id = gltf_texture_from_texture(ctx, original_path, texture);
    cgltf_material *mat = &ctx->materials.items[material_id.v];
    mat->emissive_texture.texture = gltf_tag_index(tex_id).v;
}

GL_ID GLTFContext_material_find_by_name(GLTFContext *ctx, const char *name) {
    for (uint32 i = 0; i < ctx->materials.count; ++i) {
        cgltf_material *mat = &ctx->materials.items[i];
        if (mat->name != NULL && strcmp(mat->name, name) == 0) {
            return (GL_ID){i};
        }
    }
    return INVALID_GL_ID;
}

void GLTFContext_node_set_extra(const GLTFContext *ctx, const GL_ID node_id, const char *data) {
    cgltf_node *n = &ctx->nodes.items[node_id.v];
    if (n->extras.data != NULL) {
        free(n->extras.data);
    }
    if (data != NULL) {
        n->extras.data = GLTFContext_dupe_cstring(data);
    } else {
        n->extras.data = NULL;
    }
}

GL_ID GLTFContext_node_find_by_name(const GLTFContext *ctx, const char *name) {
    for (uint32 i = 0; i < ctx->nodes.count; ++i) {
        cgltf_node *n = &ctx->nodes.items[i];
        if (n->name != NULL && strcmp(n->name, name) == 0) {
            return (GL_ID){i};
        }
    }
    return INVALID_GL_ID;
}

GL_ID GLTFContext_create_skin(GLTFContext *context, const char *name, uint32 joint_count) {
    cgltf_skin *skin = DA_append_get(&context->skins);
    memset(skin, 0, sizeof(*skin));
    if (name) {
        skin->name = GLTFContext_dupe_cstring(name);
    } else {
        skin->name = GLTFContext_dupe_cstring("skin");
    }
    skin->joints = calloc(joint_count, sizeof(cgltf_node *));
    skin->inverse_bind_matrices = gltf_tag_index(INVALID_GL_ID).v;
    skin->joints_count = joint_count;
    return (GL_ID){context->skins.count - 1};
}

void GLTFContext_skin_set_joint_inverse_matrix(GLTFContext *context, const GL_ID skin_id, const uint32 joint_index,
                                               const float *matrix_4x4) {
    cgltf_skin *skin = &context->skins.items[skin_id.v];
    if (skin->inverse_bind_matrices == gltf_tag_index(INVALID_GL_ID).v) {
        // create inverse bind matrices accessor
        const uint32 matrix_size = sizeof(float) * 16;
        const uint32 total_size = matrix_size * skin->joints_count;
        float *matrices_data = calloc(1, total_size);
        memcpy(&matrices_data[joint_index * 16], matrix_4x4, sizeof(float) * 16);
        const GL_ID accessor_id = GLTFContext_accessor_from_data(
            context, matrices_data, total_size, skin->joints_count,
            "inverse_bind_matrices", cgltf_type_mat4, cgltf_component_type_r_32f,
            cgltf_buffer_view_type_invalid, false, 0, 0);
        skin->inverse_bind_matrices = gltf_tag_index(accessor_id).v;
        free(matrices_data);
    } else {
        // update existing inverse bind matrices accessor
        const GL_ID accessor_id = gltf_untag_index(skin->inverse_bind_matrices);
        const cgltf_accessor *accessor = &context->accessors.items[accessor_id.v];
        assert(joint_index < accessor->count);
        uint8 *buffer_data = (uint8 *) accessor->buffer_view->buffer->data;
        const uint32 offset = accessor->buffer_view->offset + accessor->offset + joint_index * sizeof(float) * 16;
        memcpy(buffer_data + offset, matrix_4x4, sizeof(float) * 16);
    }
}

void GLTFContext_skin_set_joint_inverse_matrices(GLTFContext *context, const GL_ID skin_id,
                                                 DynamicArray_mat4 *matrices) {
    cgltf_skin *skin = &context->skins.items[skin_id.v];
    assert(matrices->count == skin->joints_count);
    const uint32 matrix_size = sizeof(float) * 16;
    const uint32 total_size = matrix_size * matrices->count;
    float *matrices_data = calloc(1, total_size);
    for (uint32 i = 0; i < matrices->count; ++i) {
        memcpy(&matrices_data[i * 16], matrices->items[i], sizeof(float) * 16);
    }
    const GL_ID accessor_id = GLTFContext_accessor_from_data(context, matrices_data, total_size,
                                                             matrices->count,
                                                             "inverse_bind_matrices", cgltf_type_mat4,
                                                             cgltf_component_type_r_32f,
                                                             cgltf_buffer_view_type_invalid, false, 0,
                                                             0);
    skin->inverse_bind_matrices = gltf_tag_index(accessor_id).v;
    free(matrices_data);
}

void GLTFContext_skin_set_skeleton(GLTFContext *context, const GL_ID skin_id, const GL_ID skeleton_node_id) {
    cgltf_skin *skin = &context->skins.items[skin_id.v];
    skin->skeleton = gltf_tag_index(skeleton_node_id).v;
}

void GLTFContext_skin_set_joint(GLTFContext *context, const GL_ID skin_id, const uint32 joint_index,
                                const GL_ID joint_node_id) {
    const cgltf_skin *skin = &context->skins.items[skin_id.v];
    assert(joint_index < skin->joints_count);
    skin->joints[joint_index] = gltf_tag_index(joint_node_id).v;
}

GL_ID GLTFContext_skin_find_bone_by_name(const GLTFContext *context, const GL_ID skin_id, const char *bone_name) {
    if (!IS_VALID_GL_ID(skin_id)) {
        return INVALID_GL_ID;
    }
    const cgltf_skin *skin = &context->skins.items[skin_id.v];
    for (uint32 i = 0; i < skin->joints_count; ++i) {
        const uint32 joint_id = gltf_untag_index(skin->joints[i]).v;
        const cgltf_node *joint_node = &context->nodes.items[joint_id];
        if (joint_node->name != NULL && strcmp(joint_node->name, bone_name) == 0) {
            return (GL_ID){joint_id};
        }
    }
    return INVALID_GL_ID;
}

void GLTFContext_push_skin(GLTFContext *context, const GL_ID skin_id) {
    if (context->skin_stack.count >= MAX_GLTFCONTEXT_SKIN_STACK_DEPTH) {
        printf("[ERROR]: GLTFContext_push_skin: skin stack overflow\n");
        exit(1);
    }
    context->skin_stack.items[context->skin_stack.count++] = skin_id;
}

void GLTFContext_pop_skin(GLTFContext *context) {
    if (context->skin_stack.count == 0) {
        printf("[ERROR]: GLTFContext_pop_skin: skin stack underflow\n");
        exit(1);
    }
    context->skin_stack.count--;
}

GL_ID GLTFContext_current_skin(const GLTFContext *context) {
    if (context->skin_stack.count == 0) {
        return INVALID_GL_ID;
    }
    return context->skin_stack.items[context->skin_stack.count - 1];
}

GL_ID GLTFContext_create_indices_accessor_from_data(GLTFContext *ctx, const void *data, uint32 data_size, uint32 count,
                                                    char *name, cgltf_component_type component_type, uint32 offset) {
    return GLTFContext_accessor_from_data(ctx, data, data_size, count, name,
                                          cgltf_type_scalar, component_type, cgltf_buffer_view_type_indices,
                                          false, 0, offset);
}
