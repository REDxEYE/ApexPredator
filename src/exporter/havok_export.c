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

void export_spline_compressed_animation(const hkaSplineCompressedAnimation *spline_animation, const hkaAnimationBinding* binding) {
    hkaSplineDecompressor decompressor = {};
    hkaSplineDecompressor_assign(&decompressor, spline_animation);
    const TransformSplineBlock* block = &decompressor.blocks.items[0];
    QTransform transform = {0};
    TransformSplineBlock_get_value(block, 1, 0.3f, &transform);

    hkaSplineDecompressor_free(&decompressor);
}

void export_animation(const hkaAnimationBinding *binding) {
    const hkaAnimation* animation = binding->animation.ptr;
    if (animation->type_info_->hash == hkaAnimation_HASH) {
        GLog_Warning("Raw hkaAnimation cannot be exported");
    }
    else if (animation->type_info_->hash == hkaSplineCompressedAnimation_HASH) {
        export_spline_compressed_animation((hkaSplineCompressedAnimation *) animation, binding);
    }
}

GL_ID export_animation_container(GLTFContext *context, const hkaAnimationContainer *animation_container) {
    GL_ID skeleton_id = INVALID_GL_ID;
    for (int i = 0; i < animation_container->skeletons.m_size; ++i) {
        const hkaSkeleton *skeleton = animation_container->skeletons.m_data[i].ptr;
        skeleton_id = export_skeleton(context, skeleton);
    }
    for (int i = 0; i < animation_container->bindings.m_size; ++i) {
        const hkaAnimationBinding *binding = animation_container->bindings.m_data[i].ptr;
        export_animation(binding);
    }
    return skeleton_id;
}

GL_ID export_havok_file(GLTFContext *context, const TagFile *tag_file, const String *path, const String *export_path) {
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
                output_node_id = export_animation_container(context, animation_container);
            }
        }
        String bsk_export_path = {};
        Path_join(&bsk_export_path, export_path);
        Path_join(&bsk_export_path, path);
        Path_ensure_parent_dirs(&bsk_export_path);
        String_append_cstr(&bsk_export_path, ".gltf");
        GLTFContext_set_save_path(context, &bsk_export_path);
        String_free(&bsk_export_path);
    }
    // else {
    //     GLog_Warning("Unhandled havok type: %s", String_cstr(&hk_tag_type->stable_name));

        JsonContext tmp;
        String unk_file_export_path = {0};
        Path_join(&unk_file_export_path, export_path);
        Path_join(&unk_file_export_path, path);
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

GL_ID export_skeleton(GLTFContext *context, const hkaSkeleton *skeleton) {
    const GL_ID skeleton_root = GLTFContext_node_add(context, skeleton->name.m_data);
    const GL_ID skin_id = GLTFContext_create_skin(context, "root", skeleton->bones.m_size);
    GLTFContext_skin_set_skeleton(context, skin_id, skeleton_root);

    DynamicArray_GL_ID bone_ids = {0};
    DynamicArray_mat4 inverse_matrices = {0};
    DynamicArray_mat4 global_matrices = {0};
    DA_init(&bone_ids, GL_ID, skeleton->bones.m_size);
    DA_init(&inverse_matrices, mat4, skeleton->bones.m_size);
    DA_init(&global_matrices, mat4, skeleton->bones.m_size);

    for (int i = 0; i < skeleton->bones.m_size; ++i) {
        const hkaBone *bone = &skeleton->bones.m_data[i];
        const GL_ID bone_node = GLTFContext_node_add(context, bone->name.m_data);
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
            GLTFContext_node_set_matrix(context, bone_node, (float *) bone_matrix);
        }
    }
    for (int i = 0; i < skeleton->bones.m_size; ++i) {
        mat4 inverse_matrix = {0};
        glm_mat4_inv(global_matrices.items[i], inverse_matrix);
        DA_append(&inverse_matrices, inverse_matrix);
    }

    GLTFContext_skin_set_joint_inverse_matrices(context, skin_id, &inverse_matrices);
    DA_free(&bone_ids);
    DA_free(&inverse_matrices);
    DA_free(&global_matrices);
    return skin_id;
}
