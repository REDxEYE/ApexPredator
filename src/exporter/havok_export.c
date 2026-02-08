// Created by RED on 12.01.2026.

#include "exporter/havok_export.h"

#include "platform/logger.h"
#include "utils/hash_helper.h"
#include "utils/path.h"

#include "cglm/cglm.h"
#include "havok/animations/animation.h"
#include "havok/animations/spline.h"


typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

mat4 IDENTITY_MAT = GLM_MAT4_IDENTITY_INIT;

DYNAMIC_ARRAY_STRUCT(vec3, vec3);

DYNAMIC_ARRAY_STRUCT(versor, versor);

void export_spline_compressed_animation(AppState* app_state, const hkaSplineCompressedAnimation *spline_animation,
                                        const hkaAnimationBinding *binding, const hkaSkeleton *skeleton,
                                        const StringView animation_name) {
    CHECK_APP_STATE(app_state);
    CHECK_GLTF_STATE(&app_state->gltf_context);
    GLTFContext *context = &app_state->gltf_context;

    hkaSplineDecompressor decompressor = {};
    hkaSplineDecompressor_assign(&decompressor, spline_animation);
    const float32 frame_duration = spline_animation->frameDuration;

    const GL_ID animation_id = GLTFContext_animation_new(context, StringView_cstr(animation_name));

    DynamicArray_float32 timestamps = {0};
    DynamicArray_vec3 positions = {0};
    DynamicArray_versor rotations = {0};
    DynamicArray_vec3 scales = {0};

    DA_init(&timestamps, float32, spline_animation->numFrames);

    for (int frame_id = 0; frame_id < spline_animation->numFrames; ++frame_id) {
        const float32 time = (float32) frame_id * frame_duration;
        DA_append(&timestamps, &time);
    }

    const GL_ID timestamps_accessor = GLTFContext_accessor_from_data(context, DA_get_buffer(&timestamps),
                                                                     sizeof(float32) * timestamps.count,
                                                                     timestamps.count,
                                                                     "spline_animation_timestamps",
                                                                     cgltf_type_scalar,
                                                                     cgltf_component_type_r_32f,
                                                                     cgltf_buffer_view_type_invalid,
                                                                     false, 0, 0);
    const float t_min = 0;
    const float t_max = spline_animation->duration;
    GLTFContext_accessor_set_minmax(context, timestamps_accessor, &t_min, &t_max);

    for (int track_id = 0; track_id < binding->transformTrackToBoneIndices.m_size; ++track_id) {
        const uint32 bone_id = binding->transformTrackToBoneIndices.m_data[track_id];
        if (track_id != 0 && bone_id == 0) {
            continue;
        }
        const hkaBone *bone = &skeleton->bones.m_data[bone_id];

        const GL_ID bone_node = GLTFContext_node_find_by_name(context, bone->name.m_data);
        if (!IS_VALID_GL_ID(bone_node)) {
            continue;
        }

        DA_init(&positions, vec3, spline_animation->numFrames);
        DA_init(&rotations, versor, spline_animation->numFrames);
        DA_init(&scales, vec3, spline_animation->numFrames);

        // vec3 position_min = {999999.f}, position_max = {-999999.f};
        // versor rotation_min = {999999.f}, rotation_max = {-999999.f};
        // vec3 scale_min = {0999999.f}, scale_max = {-999999.f};

        for (int frame_id = 0; frame_id < spline_animation->numFrames; ++frame_id) {
            uint32 block_id = frame_id/spline_animation->maxFramesPerBlock;

            if (block_id >= decompressor.blocks.count) {
                block_id = decompressor.blocks.count - 1;
            }
            uint32 local_frame = frame_id % spline_animation->maxFramesPerBlock;

            const TransformSplineBlock *block = &decompressor.blocks.items[block_id];
            QTransform transform = {0};
            TransformSplineBlock_get_value(block, track_id, local_frame, &transform);

            // if (transform.translation[0] > position_max[0]) position_max[0] = transform.translation[0];
            // if (transform.translation[1] > position_max[1]) position_max[1] = transform.translation[1];
            // if (transform.translation[2] > position_max[2]) position_max[2] = transform.translation[2];
            //
            // if (transform.translation[0] < position_min[0]) position_min[0] = transform.translation[0];
            // if (transform.translation[1] < position_min[1]) position_min[1] = transform.translation[1];
            // if (transform.translation[2] < position_min[2]) position_min[2] = transform.translation[2];
            //
            // if (transform.rotation[0] > rotation_max[0]) rotation_max[0] = transform.rotation[0];
            // if (transform.rotation[1] > rotation_max[1]) rotation_max[1] = transform.rotation[1];
            // if (transform.rotation[2] > rotation_max[2]) rotation_max[2] = transform.rotation[2];
            // if (transform.rotation[3] > rotation_max[3]) rotation_max[3] = transform.rotation[3];
            //
            // if (transform.rotation[0] < rotation_min[0]) rotation_min[0] = transform.rotation[0];
            // if (transform.rotation[1] < rotation_min[1]) rotation_min[1] = transform.rotation[1];
            // if (transform.rotation[2] < rotation_min[2]) rotation_min[2] = transform.rotation[2];
            // if (transform.rotation[3] < rotation_min[3]) rotation_min[3] = transform.rotation[3];
            //
            // if (transform.scale[0] > scale_max[0]) scale_max[0] = transform.scale[0];
            // if (transform.scale[1] > scale_max[1]) scale_max[1] = transform.scale[1];
            // if (transform.scale[2] > scale_max[2]) scale_max[2] = transform.scale[2];
            //
            // if (transform.scale[0] < scale_min[0]) scale_min[0] = transform.scale[0];
            // if (transform.scale[1] < scale_min[1]) scale_min[1] = transform.scale[1];
            // if (transform.scale[2] < scale_min[2]) scale_min[2] = transform.scale[2];

            DA_append(&positions, &transform.translation);
            DA_append(&rotations, &transform.rotation);
            DA_append(&scales, &transform.scale);
        }

        const GL_ID positions_accessor = GLTFContext_accessor_from_data(context, DA_get_buffer(&positions),
                                                                        sizeof(vec3) * positions.count,
                                                                        positions.count,
                                                                        "spline_animation_positions",
                                                                        cgltf_type_vec3,
                                                                        cgltf_component_type_r_32f,
                                                                        cgltf_buffer_view_type_invalid,
                                                                        false, 0, 0);
        const GL_ID rotations_accessor = GLTFContext_accessor_from_data(context, DA_get_buffer(&rotations),
                                                                        sizeof(versor) * rotations.count,
                                                                        rotations.count,
                                                                        "spline_animation_rotations",
                                                                        cgltf_type_vec4,
                                                                        cgltf_component_type_r_32f,
                                                                        cgltf_buffer_view_type_invalid,
                                                                        false, 0, 0);
        const GL_ID scales_accessor = GLTFContext_accessor_from_data(context, DA_get_buffer(&scales),
                                                                     sizeof(vec3) * scales.count,
                                                                     scales.count,
                                                                     "spline_animation_scales",
                                                                     cgltf_type_vec3,
                                                                     cgltf_component_type_r_32f,
                                                                     cgltf_buffer_view_type_invalid,
                                                                     false, 0, 0);

        DA_free(&positions);
        DA_free(&rotations);
        DA_free(&scales);

        // GLTFContext_accessor_set_minmax(context, positions_accessor, position_min, position_max);
        // GLTFContext_accessor_set_minmax(context, rotations_accessor, rotation_min, rotation_max);
        // GLTFContext_accessor_set_minmax(context, scales_accessor, scale_min, scale_max);

        const GL_ID positions_sampler = GLTFContext_animation_sampler_new(context, animation_id,
                                                                          cgltf_interpolation_type_linear,
                                                                          timestamps_accessor,
                                                                          positions_accessor);
        const GL_ID rotations_sampler = GLTFContext_animation_sampler_new(context, animation_id,
                                                                          cgltf_interpolation_type_linear,
                                                                          timestamps_accessor,
                                                                          rotations_accessor);
        const GL_ID scales_sampler = GLTFContext_animation_sampler_new(context, animation_id,
                                                                       cgltf_interpolation_type_linear,
                                                                       timestamps_accessor,
                                                                       scales_accessor);


        GLTFContext_animation_channel_new(context, animation_id, positions_sampler, bone_node,
                                          cgltf_animation_path_type_translation);
        GLTFContext_animation_channel_new(context, animation_id, rotations_sampler, bone_node,
                                          cgltf_animation_path_type_rotation);
        GLTFContext_animation_channel_new(context, animation_id, scales_sampler, bone_node,
                                          cgltf_animation_path_type_scale);
    }

    DA_free(&timestamps);
    hkaSplineDecompressor_free(&decompressor);
}

void export_animation(AppState* app_state, const hkaAnimationBinding *binding, const hkaSkeleton *skeleton,
                      const StringView animation_name) {
    CHECK_APP_STATE(app_state);

    export_skeleton(app_state, skeleton);

    const hkaAnimation *animation = binding->animation.ptr;
    if (animation->type_info_->hash == hkaAnimation_HASH) {
        GLog_Warning("Raw hkaAnimation cannot be exported");
    }
    else if (animation->type_info_->hash == hkaSplineCompressedAnimation_HASH) {
        export_spline_compressed_animation(app_state, (hkaSplineCompressedAnimation *) animation, binding, skeleton,
                                           animation_name);
    }
}

GL_ID export_animation_container(AppState* app_state, const hkaAnimationContainer *animation_container) {
    CHECK_APP_STATE(app_state);

    GL_ID skeleton_id = INVALID_GL_ID;
    for (int i = 0; i < animation_container->skeletons.m_size; ++i) {
        const hkaSkeleton *skeleton = animation_container->skeletons.m_data[i].ptr;
        skeleton_id = export_skeleton(app_state, skeleton);
    }
    // for (int i = 0; i < animation_container->bindings.m_size; ++i) {
    //     const hkaAnimationBinding *binding = animation_container->bindings.m_data[i].ptr;
    //     export_animation(binding);
    // }
    return skeleton_id;
}

GL_ID export_havok_file(AppState* app_state, const TagFile *tag_file, const StringView path) {
    CHECK_APP_STATE(app_state);
        CHECK_GLTF_STATE(&app_state->gltf_context);
    GLTFContext *context = &app_state->gltf_context;

    const HKItem *item = &tag_file->items.items[1];
    HKTagType *hk_tag_type = &tag_file->types.items[item->type];
    const uint32 type_hash = hash_string(HKTagType_stable_name(hk_tag_type));
    const HavokTypeInfo *type_info = *(HavokTypeInfo **) DM_get(&HAVOK_TYPES_type_info, type_hash);
    void *item_obj = mp_malloc(type_info->size);
    type_info->init(item_obj);
    type_info->read(item_obj, tag_file, &tag_file->data.items[item->offset]);

    GL_ID output_node_id = INVALID_GL_ID;

    if (String_cequals(&hk_tag_type->stable_name, "hkRootLevelContainer")) {
        const hkRootLevelContainer *root_level_container = (hkRootLevelContainer *) item_obj;
        for (int i = 0; i < root_level_container->namedVariants.m_size; ++i) {
            const hkRootLevelContainer__NamedVariant *variant = &root_level_container->namedVariants.m_data[i];
            if (strcmp(variant->className.m_data, "hkaAnimationContainer") == 0) {
                const hkaAnimationContainer *animation_container = (hkaAnimationContainer *) variant->variant.ptr;
                output_node_id = export_animation_container(app_state, animation_container);
            }
        }
        String bsk_export_path = {};
        Path_join(&bsk_export_path, &app_state->export_path);
        Path_join_sv(&bsk_export_path, path);
        Path_ensure_parent_dirs(&bsk_export_path);
        String_append_cstr(&bsk_export_path, ".gltf");
        GLTFContext_set_save_path(context, &bsk_export_path);
        String_free(&bsk_export_path);
    }
    // else {
    //     GLog_Warning("Unhandled havok type: %s", String_cstr(&hk_tag_type->stable_name));

    JsonContext tmp;
    String unk_file_export_path = {0};
    Path_join(&unk_file_export_path, &app_state->export_path);
    Path_join_sv(&unk_file_export_path, path);
    Path_ensure_parent_dirs(&unk_file_export_path);
    String json_output = {0};
    Path_replace_extension(&unk_file_export_path, "json", &json_output);
    FILE *f = fopen(String_cstr(&json_output), "wb");
    jsonInit(&tmp, f);
    jsonBeginObject(&tmp);
    ((TypedPtr *) item_obj)->type_info_->print(item_obj, &tmp);
    jsonEndObject(&tmp);
    fclose(f);
    String_free(&unk_file_export_path);
    String_free(&json_output);
    // }

    type_info->free(item_obj);
    mp_free(item_obj);

    return output_node_id;
}

void build_matrix(mat4 out, const hkQsTransform *transform) {
    hkVector4f translation = transform->translation;
    hkVector4f rotation = transform->rotation.vec;
    hkVector4f scale = transform->scale;
    glm_mat4_identity(out);
    glm_translate(out, &translation.x);
    glm_quat_rotate(out, &rotation.x, out);
    glm_scale(out, &scale.x);
}

GL_ID export_skeleton(AppState* app_state, const hkaSkeleton *skeleton) {
    CHECK_APP_STATE(app_state);
        CHECK_GLTF_STATE(&app_state->gltf_context);
    GLTFContext *context = &app_state->gltf_context;

    const GL_ID skeleton_root = GLTFContext_node_add(context, skeleton->name.m_data, true);
    const GL_ID skin_id = GLTFContext_skin_new(context, "root", skeleton->bones.m_size);
    GLTFContext_skin_set_skeleton(context, skin_id, skeleton_root);

    DynamicArray_GL_ID bone_ids = {0};
    DynamicArray_mat4 inverse_matrices = {0};
    DynamicArray_mat4 global_matrices = {0};
    DA_init(&bone_ids, GL_ID, skeleton->bones.m_size);
    DA_init(&inverse_matrices, mat4, skeleton->bones.m_size);
    DA_init(&global_matrices, mat4, skeleton->bones.m_size);

    for (int i = 0; i < skeleton->bones.m_size; ++i) {
        const hkaBone *bone = &skeleton->bones.m_data[i];
        const GL_ID bone_node = GLTFContext_node_add(context, bone->name.m_data, true);
        GLTFContext_skin_set_joint(context, skin_id, i, bone_node);
        DA_append(&bone_ids, &bone_node);
        const int16 bone_parent_id = skeleton->parentIndices.m_data[i];
        if (bone_parent_id >= 0) {
            GLTFContext_node_set_parent(context, bone_node, bone_ids.items[bone_parent_id]);
        }
        else {
            GLTFContext_node_set_parent(context, bone_node, skeleton_root);
        }
        const hkQsTransform *transform = &skeleton->referencePose.m_data[i];
        mat4 bone_matrix = {0};
        build_matrix(bone_matrix, transform);

        DA_append(&global_matrices, bone_matrix);
        if (bone_parent_id >= 0) {
            glm_mat4_mul(global_matrices.items[bone_parent_id], bone_matrix, global_matrices.items[i]);
        }

        if (memcmp(IDENTITY_MAT, bone_matrix, sizeof(mat4)) != 0) {
            // GLTFContext_node_set_matrix(context, bone_node, (float *) bone_matrix);
            GLTFContext_node_set_trs(context, bone_node,
                                     &transform->translation.x,
                                     &transform->rotation.vec.x,
                                     &transform->scale.x
            );
        }
    }
    for (int i = 0; i < skeleton->bones.m_size; ++i) {
        mat4 inverse_matrix = {0};
        glm_mat4_inv(global_matrices.items[i], inverse_matrix);
        inverse_matrix[3][3] = 1.0f;
        DA_append(&inverse_matrices, inverse_matrix);
    }

    GLTFContext_skin_set_joint_inverse_matrices(context, skin_id, &inverse_matrices);
    DA_free(&bone_ids);
    DA_free(&inverse_matrices);
    DA_free(&global_matrices);
    return skin_id;
}
