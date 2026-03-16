// Created by RED on 27.09.2025.

#ifndef APEXPREDATOR_CGLTF_HELPER_H
#define APEXPREDATOR_CGLTF_HELPER_H

#include <string>

#include "cgltf.h"
#include "cgltf_write.h"

#include "platform/texture.h"

typedef struct GLTFContext {
    cgltf_data *data;
    cgltf_options options;
    bool merge_buffers;

    std::string save_path;

    // DynamicArray_cgltf_mesh meshes;
    // DynamicArray_cgltf_node nodes;
    // DynamicArray_cgltf_skin skins;
    // DynamicArray_cgltf_accessor accessors;
    // DynamicArray_cgltf_buffer buffers;
    // DynamicArray_cgltf_buffer_view buffer_views;
    // DynamicArray_rootNodeIds scene_node_ids;
    // DynamicArray_cgltf_material materials;
    // DynamicArray_cgltf_texture textures;
    // DynamicArray_cgltf_image images;
    // DynamicArray_cgltf_animation animations;
    //
    // DynamicArray_GL_ID skin_stack;
    //
    // DynamicArray_data_buffer raw_buffers;

    bool finalized;
} GLTFContext;

void GLTFContext_init(GLTFContext *ctx, const char *name);

bool GLTFContext_is_initialized(const GLTFContext *ctx);

// Check state and abort if NULL or uninitialized
#define CHECK_GLTF_STATE(state) \
    do { \
        if ((state) == NULL) { \
            GLog_Error("GLTFContext is NULL"); \
            abort(); \
        } \
        if (!GLTFContext_is_initialized(state)) { \
            GLog_Error("GLTFContext is not initialized"); \
            abort(); \
        } \
    } while (0)


void GLTFContext_set_save_path(GLTFContext *ctx, const std::string &path);

void GLTFContext_set_save_cpath(GLTFContext *ctx, const char *path);

void GLTFContext_finalize(GLTFContext *ctx);

void GLTFContext_free(GLTFContext *ctx);

const std::string &GLTFContext_data_path(const GLTFContext *ctx);

// GL_ID GLTFContext_create_buffer(GLTFContext *ctx, const void *data, uint32 data_size, const char *name);
//
// GL_ID GLTFContext_create_buffer_and_view(
//     GLTFContext *ctx, const void *data, uint32 data_size, const char *name,
//     cgltf_buffer_view_type type, uint32 stride, uint32 offset);
//
// GL_ID GLTFContext_accessor_add(
//     GLTFContext *ctx, GL_ID buffer_view_id, cgltf_type type,
//     cgltf_component_type component_type, uint32 count, uint32 offset,
//     bool normalized, const char *name);
//
// GL_ID GLTFContext_accessor_from_data(
//     GLTFContext *ctx, const void *data, uint32 data_size,
//     uint32 count, const char *name, cgltf_type type,
//     cgltf_component_type component_type, cgltf_buffer_view_type buffer_type,
//     bool normalized, uint32 stride, uint32 offset);
//
// GL_ID GLTFContext_node_add(const GLTFContext *ctx, const char *name_opt, bool copy_name);
//
// void GLTFContext_node_set_mesh(const GLTFContext *ctx, GL_ID node_id, GL_ID mesh_id);
//
// void GLTFContext_node_set_parent(const GLTFContext *ctx, GL_ID node_id, GL_ID parent_node_id);
//
// void GLTFContext_node_set_matrix(const GLTFContext *ctx, GL_ID node_id, const float *matrix_4x4);
//
// void GLTFContext_node_set_trs(const GLTFContext *ctx, GL_ID node_id, const vec3 pos, const versor rot, const vec3 scl);
//
// void GLTFContext_node_set_extra(const GLTFContext *ctx, GL_ID node_id, char *data, bool copy_data);
//
// GL_ID GLTFContext_node_find_by_name(const GLTFContext *ctx, const char *name);
//
// GL_ID GLTFContext_mesh_add(GLTFContext *ctx, const char *name_opt, uint32 primitive_count);
//
// cgltf_primitive *GLTFContext_mesh_get_primitive(const GLTFContext *ctx, GL_ID mesh_id, uint32 prim_index);
//
// void GLTFContext_node_set_skin(const GLTFContext *ctx, GL_ID node_id, GL_ID skin_id);
//
// void GLTFContext_primitive_set_material(const GLTFContext *ctx, GL_ID mesh_id, uint32 prim_index, GL_ID material_id);
//
// void GLTFContext_set_primitive_indices_accessor(const GLTFContext *ctx, GL_ID mesh_id, uint32 prim_index,
//                                                 GL_ID accessor_id);
//
// void GLTFContext_primitive_init_attributes(const GLTFContext *ctx, GL_ID mesh_id, uint32 prim_index,
//                                            uint32 attribute_count);
//
// void GLTFContext_accessor_set_minmax(const GLTFContext *ctx, GL_ID accessor_id, const float *min_values,
//                                      const float *max_values);
//
// void GLTFContext_primitive_set_attribute_accessor(const GLTFContext *ctx, GL_ID mesh_id, uint32 prim_index,
//                                                   uint32 attribute_index, GL_ID accessor_id,
//                                                   const char *name, cgltf_attribute_type type);
//
// bool GLTFContext_write_and_free(GLTFContext *ctx);
//
// GL_ID GLTFContext_image_new(GLTFContext *ctx, const char *name_opt);
//
// GL_ID GLTFContext_image_from_data(GLTFContext *ctx, const String *original_path, const Texture *texture);
//
// void GLTFContext_image_set_mimetype(const GLTFContext *ctx, GL_ID image_id, const char *mimetype);
//
// // void GLTFContext_image_set_base64_data(GLTFContext *ctx, GL_ID image_id, const void *data, uint32 data_size);
//
// // void GLTFContext_image_set_url(GLTFContext *ctx, GL_ID image_id, const char *url);
//
// void GLTFContext_image_set_buffer_view(const GLTFContext *ctx, GL_ID image_id, GL_ID buffer_view_id);
//
// // void GLTFContext_image_set_data(const GLTFContext *ctx,  GL_ID image_id, D_ID data_id);
//
// GL_ID GLTFContext_texture_new(GLTFContext *ctx, const char *name_opt);
//
// GL_ID GLTFContext_texture_new_with_image(GLTFContext *ctx, const char *name_opt, GL_ID image_id);
//
// GL_ID GLTFContext_material_new(GLTFContext *ctx, const char *name_opt);
//
// bool GLTFContext_material_diffuse_present(const GLTFContext *ctx, GL_ID material_id);
//
// void GLTFContext_material_set_diffuse_texture_from_data(GLTFContext *ctx, StringView original_path,
//                                                         GL_ID material_id,
//                                                         const Texture *texture);
//
// void GLTFContext_material_set_normal_from_data(GLTFContext *ctx, StringView original_path,
//                                                GL_ID material_id,
//                                                const Texture *texture);
//
// void GLTFContext_material_set_roughness_metallic_from_data(GLTFContext *ctx, StringView original_path,
//                                                            GL_ID material_id,
//                                                            const Texture *texture);
//
//
// void GLTFContext_material_set_emissive_from_data(GLTFContext *ctx, StringView original_path,
//                                                  GL_ID material_id,
//                                                  const Texture *texture);
//
// GL_ID GLTFContext_material_find_by_name(const GLTFContext *ctx, const char *name);
//
// GL_ID GLTFContext_skin_new(GLTFContext *context, const char *name, uint32 joint_count);
//
// void GLTFContext_skin_set_joint_inverse_matrix(GLTFContext *context, GL_ID skin_id, uint32 joint_index,
//                                                const float *matrix_4x4);
//
// void GLTFContext_skin_set_joint_inverse_matrices(GLTFContext *context, GL_ID skin_id, DynamicArray_mat4 *matrices);
//
// void GLTFContext_skin_set_skeleton(const GLTFContext *context, GL_ID skin_id, GL_ID skeleton_node_id);
//
// void GLTFContext_skin_set_joint(const GLTFContext *context, GL_ID skin_id, uint32 joint_index, GL_ID joint_node_id);
//
// GL_ID GLTFContext_skin_find_bone_by_name(const GLTFContext *context, GL_ID skin_id, const char *bone_name);
//
// void GLTFContext_push_skin(GLTFContext *context, GL_ID skin_id);
//
// void GLTFContext_pop_skin(GLTFContext *context);
//
// GL_ID GLTFContext_current_skin(const GLTFContext *context);
//
// GL_ID GLTFContext_animation_new(GLTFContext *context, const char *name);
//
// GL_ID GLTFContext_animation_sampler_new(const GLTFContext *context, GL_ID animation_id,
//                                         cgltf_interpolation_type interpolation,
//                                         GL_ID input_accessor_id,
//                                         GL_ID output_accessor_id
// );
//
// GL_ID GLTFContext_animation_channel_new(const GLTFContext *context, GL_ID animation_id,
//                                         GL_ID sampler_id,
//                                         GL_ID target_node_id,
//                                         cgltf_animation_path_type path_type
// );
//
// // Shortcuts
// GL_ID GLTFContext_create_indices_accessor_from_data(
//     GLTFContext *ctx, const void *data, uint32 data_size,
//     uint32 count, const char *name, cgltf_component_type component_type,
//     uint32 offset);

#endif //APEXPREDATOR_CGLTF_HELPER_H
