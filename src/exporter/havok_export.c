// Created by RED on 12.01.2026.

#include "../../include/exporter/havok_export.h"

mat4 IDENTITY_MAT = GLM_MAT4_IDENTITY_INIT;

void build_matrix(mat4 out, const hkQsTransform *transform) {
    HavokVector4 translation = transform->translation;
    HavokVector4 rotation = transform->rotation.vec;
    HavokVector4 scale = transform->scale;
    glm_mat4_identity(out);
    glm_translate(out, &translation.x);
    glm_quat_rotate(out, &rotation.x, out);
    glm_scale(out, &scale.x);
}

GL_ID export_skeleton(GLTFContext *context, const hkaSkeleton *skeleton, Havok_TypeLibrary *havok_lib) {
    const GL_ID skeleton_root = GLTFContext_node_add(context, skeleton->name.stringAndFlag);
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
        const GL_ID bone_node = GLTFContext_node_add(context, bone->name.stringAndFlag);
        GLTFContext_skin_set_joint(context, skin_id, i, bone_node);
        DA_append(&bone_ids, &bone_node);
        const int16 bone_parent_id = skeleton->parentIndices.m_data[i];
        if (bone_parent_id >= 0) {
            GLTFContext_node_set_parent(context, bone_node, bone_ids.items[bone_parent_id]);
        } else {
            GLTFContext_node_set_parent(context, bone_node, skeleton_root);
        }
        const hkQsTransform *transform = &skeleton->referencePose.m_data[i];
        mat4 bone_matrix = {0};
        build_matrix(bone_matrix, transform);

        DA_append(&global_matrices, bone_matrix);
        if (bone_parent_id>=0) {
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
    return skin_id;
}
