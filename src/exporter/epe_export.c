// Created by RED on 12.01.2026.

#include "exporter/epe_export.h"

#include "apex/hashes.h"
#include "exporter/havok_export.h"
#include "exporter/adf_export.h"
#include "exporter/common_export.h"
#include "platform/logger.h"
#include "utils/path.h"

void add_extras(const GLTFContext *context, const RuntimeNode *node, const GL_ID output_node) {
    String extra_data = {0};
    String_reserve(&extra_data, 8192);
    String_append_cstr(&extra_data, "{");
    DA_FORI(node->props, i) {
        RuntimeProp_emit_json(&node->props.items[i], &extra_data, 1);
        if (i + 1 < node->props.count)
            String_append_cstr(&extra_data, ", ");
    }
    String_append_cstr(&extra_data, "}");
    GLTFContext_node_set_extra(context, output_node, String_cstr(&extra_data));
    String_free(&extra_data);
}

void get_node_matrix(const cgltf_node *target_node, mat4 out) {
    if (target_node->has_matrix) {
        memcpy(out, target_node->matrix, sizeof(float) * 16);
    }
    else {
        glm_mat4_identity(out);
        if (target_node->has_translation) {
            vec3 tmp = {0};
            tmp[0] = target_node->translation[0];
            tmp[1] = target_node->translation[1];
            tmp[2] = target_node->translation[2];
            glm_translate(out, tmp);
        }
        if (target_node->has_rotation) {
            versor tmp = {0};
            tmp[0] = target_node->rotation[0];
            tmp[1] = target_node->rotation[1];
            tmp[2] = target_node->rotation[2];
            tmp[3] = target_node->rotation[3];
            glm_quat_rotate(out, tmp, out);
        }
        if (target_node->has_scale) {
            vec3 tmp = {0};
            tmp[0] = target_node->scale[0];
            tmp[1] = target_node->scale[1];
            tmp[2] = target_node->scale[2];
            glm_scale(out, tmp);
        }
    }
}

void calculate_global_node_matrix(const GLTFContext *context, const GL_ID target_id, mat4 out) {
    if (!IS_VALID_GL_ID(target_id)) {
        glm_mat4_identity(out);
        return;
    }
    const cgltf_node *target_node = &context->nodes.items[target_id.v];
    if (target_node->parent != NULL) {
        mat4 parent_matrix = {0};
        glm_mat4_identity(parent_matrix);
        const GL_ID parent_id = gltf_untag_index(target_node->parent);
        calculate_global_node_matrix(context, parent_id, parent_matrix);
        mat4 local_matrix = {0};
        glm_mat4_identity(local_matrix);
        get_node_matrix(target_node, local_matrix);
        glm_mat4_mul(parent_matrix, local_matrix, out);
    }
    else {
        get_node_matrix(target_node, out);
    }
}

void process_children(AppState *app_state,
                      RuntimeNode *node, const uint32 path_hash, const StringView path,
                      const GL_ID parent_gltf_node) {
    DA_FORI(node->children, i) {
        process_epe_node(app_state, DA_at(&node->children, i),
                         path_hash,
                         path, parent_gltf_node);
    }
}

void set_world_matrix(GLTFContext *context, const GL_ID gltf_node, RuntimeNode *node) {
    if (!RuntimeNode_has_prop(node, "world"))
        return;
    const float32 *matrix = RuntimeNode_get_prop_mat4x4(node, "world");
    if (matrix != NULL && memcmp(IDENTITY_MAT, matrix, sizeof(mat4)) != 0)
        GLTFContext_node_set_matrix(context, gltf_node, matrix);
}

void handle_CCharacter(AppState *app_state,
                       RuntimeNode *node, const uint32 path_hash, const StringView path,
                       const GL_ID parent_gltf_node) {
    GLTFContext *context = &app_state->gltf_context;

    const StringView model_filename = RuntimeNode_get_prop_by_hash_str(node, 0xE8129FE6);
    const StringView skeleton_filename = RuntimeNode_get_prop_by_hash_str(node, 0x26FA86FE);
    if (sv_is_null(model_filename)) {
        GLog_Error("Failed to get model property for CCharacter");
        return;
    }

    String skeleton_bsk_name = {0};
    Path_replace_extension_sv(skeleton_filename, "bsk", &skeleton_bsk_name);

    const GL_ID skin_id = export_file(app_state, as_sv(&skeleton_bsk_name), hash_string(&skeleton_bsk_name));
    if (!IS_VALID_GL_ID(skin_id)) {
        GLog_Error("Failed to export skeleton for CCharacter: %s", String_cstr(&skeleton_bsk_name));
        exit(1);
    }
    String_free(&skeleton_bsk_name);
    const cgltf_skin *skin = &context->skins.items[skin_id.v];
    const GL_ID root_bone = skin->joints[0] != NULL ? gltf_untag_index(skin->joints[0]) : INVALID_GL_ID;
    if (IS_VALID_GL_ID(root_bone)) {
        GLTFContext_node_set_parent(context, parent_gltf_node, root_bone);
    }

    GLTFContext_push_skin(context, skin_id);
    const GL_ID output_node = export_adf_file(app_state, model_filename, hash_vstring(model_filename));

    add_extras(context, node, output_node);
    set_world_matrix(context, output_node, node);
    if (IS_VALID_GL_ID(parent_gltf_node))
        GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
    else {
        GLog_Warning("Invalid parent setup: 0x%08X", node->name_hash);
    }

    process_children(app_state, node, path_hash, path, output_node);
    GLTFContext_pop_skin(context);
}

void handle_CSecondaryMotionAttachment(AppState *app_state,
                                       RuntimeNode *node, const uint32 path_hash, const StringView path,
                                       const GL_ID parent_gltf_node) {
    GLTFContext *context = &app_state->gltf_context;
    const StringView model_filename = RuntimeNode_get_prop_str(node, "model");
    const StringView skeleton_filename = RuntimeNode_get_prop_by_hash_str(node, 0x26FA86FE);
    if (sv_is_null(model_filename)) {
        GLog_Error("Failed to get model property for CSecondaryMotionAttachment");
        return;
    }
    String skeleton_bsk_name = {0};
    Path_replace_extension_sv(skeleton_filename, "bsk", &skeleton_bsk_name);
    const GL_ID skin_id = export_file(app_state, as_sv(&skeleton_bsk_name), hash_string(&skeleton_bsk_name));
    String_free(&skeleton_bsk_name);

    GLTFContext_push_skin(context, skin_id);
    const GL_ID output_node = export_adf_file(app_state, model_filename, hash_vstring(model_filename));
    set_world_matrix(context, output_node, node);
    add_extras(context, node, output_node);
    if (IS_VALID_GL_ID(parent_gltf_node))
        GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
    else {
        GLog_Warning("Invalid parent setup: 0x%08X", node->name_hash);
    }
    process_children(app_state, node, path_hash, path, output_node);
    GLTFContext_pop_skin(context);
}

void handle_CDamageableCharacterPart(AppState *app_state,
                                     RuntimeNode *node, const uint32 path_hash, const StringView path,
                                     const GL_ID parent_gltf_node) {
    GLTFContext *context = &app_state->gltf_context;

    const StringView node_name = RuntimeNode_get_prop_str(node, "name");
    const StringView node_name_hash = find_name32(node->name_hash);

    GL_ID output_node;
    if (sv_is_not_null(node_name)) {
        output_node = GLTFContext_node_add(context, StringView_cstr(node_name));
    }
    else if (sv_is_not_null(node_name_hash)) {
        output_node = GLTFContext_node_add(context, StringView_cstr(node_name_hash));
    }
    else {
        char generated_name[64];
        snprintf(generated_name, sizeof(generated_name), "node_%08X", node->name_hash);
        output_node = GLTFContext_node_add(context, generated_name);
    }

    set_world_matrix(context, output_node, node);
    add_extras(context, node, output_node);

    const GL_ID current_skin = GLTFContext_current_skin(context);
    if (context->skins.count > 0 && IS_VALID_GL_ID(current_skin)) {
        const StringView parent_bone_name = RuntimeNode_get_prop_by_hash_str(node, 0x4d67eec5);
        mat4 node_global_matrix = {0};
        const GL_ID parent_bone_id = GLTFContext_skin_find_bone_by_name(context, current_skin,
                                                                        StringView_cstr(parent_bone_name));

        calculate_global_node_matrix(context, parent_bone_id, node_global_matrix);
        glm_mat4_inv(node_global_matrix, node_global_matrix);
        GLTFContext_node_set_matrix(context, output_node, (float *) node_global_matrix);
        GLTFContext_node_set_parent(context, output_node, parent_bone_id);
    }
    else if (IS_VALID_GL_ID(parent_gltf_node))
        GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
    else {
        GLog_Warning("Invalid parent setup: %s", StringView_cstr(node_name_hash));
    }

    process_children(app_state, node, path_hash, path, output_node);
}

void handle_CRigidObject(AppState *app_state,
                         RuntimeNode *node, const uint32 path_hash, const StringView path,
                         const GL_ID parent_gltf_node) {
    GLTFContext *context = &app_state->gltf_context;

    const uint32 model_filename_hash = RuntimeNode_get_prop_u32(node, "filename");
    const StringView model_filename = find_name32(model_filename_hash);
    if (model_filename_hash == 0) {
        GLog_Error("Failed to get model property for CRigidObject");
        return;
    }
    GL_ID output_node = export_adf_file(app_state, model_filename, model_filename_hash);
    if (IS_VALID_GL_ID(output_node)) {
        set_world_matrix(context, output_node, node);

        if (IS_VALID_GL_ID(parent_gltf_node))
            GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
        else {

            GLog_Warning("Invalid parent setup: 0x%08X", node->name_hash);
        }
    }
    else {
        const StringView node_name = find_name32(node->name_hash);
        if (sv_is_not_null(model_filename)) {
            output_node = GLTFContext_node_add(context, StringView_cstr(model_filename));
        }
        else if (sv_is_not_null(node_name)) {
            output_node = GLTFContext_node_add(context, StringView_cstr(node_name));
        }
        else {
            char generated_name[64];
            snprintf(generated_name, sizeof(generated_name), "node_%08X", node->name_hash);
            output_node = GLTFContext_node_add(context, generated_name);
        }

        set_world_matrix(context, output_node, node);
        add_extras(context, node, output_node);
    }
    process_children(app_state, node, path_hash, path, output_node);
}

void handle_CSkeletalAnimatedObject(AppState *app_state,
                                    RuntimeNode *node, const uint32 path_hash, const StringView path,
                                    const GL_ID parent_gltf_node) {
    GLTFContext *context = &app_state->gltf_context;

    const StringView model_filename = RuntimeNode_get_prop_by_hash_str(node, 0x0f94740b);
    const StringView skeleton_filename = RuntimeNode_get_prop_by_hash_str(node, 0x26fa86fe);

    if (sv_is_null(model_filename)) {
        GLog_Error("Failed to get model property for CSkeletalAnimatedObject");
        return;
    }

    if (sv_is_null(skeleton_filename)) {
        GLog_Error("Failed to get skeleton property for CSkeletalAnimatedObject");
        return;
    }

    String skeleton_bsk_name = {0};
    Path_replace_extension_sv(skeleton_filename, "bsk", &skeleton_bsk_name);

    const GL_ID skin_id = export_file(app_state, as_sv(&skeleton_bsk_name), hash_string(&skeleton_bsk_name));
    const cgltf_skin *skin = &context->skins.items[skin_id.v];
    const GL_ID root_bone = skin->joints[0] != NULL ? gltf_untag_index(skin->joints[0]) : INVALID_GL_ID;
    if (IS_VALID_GL_ID(root_bone)) {
        GLTFContext_node_set_parent(context, parent_gltf_node, root_bone);
    }

    GLTFContext_push_skin(context, skin_id);
    const GL_ID output_node = export_adf_file(app_state, model_filename, hash_vstring(model_filename));

    add_extras(context, node, output_node);
    set_world_matrix(context, output_node, node);
    if (IS_VALID_GL_ID(parent_gltf_node))
        GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
    else {
        GLog_Warning("Invalid parent setup: 0x%08X", node->name_hash);
    }

    process_children(app_state, node, path_hash, path, output_node);
    GLTFContext_pop_skin(context);
}

void handle_CBoneAttachment(AppState *app_state,
                            RuntimeNode *node, const uint32 path_hash, const StringView path,
                            const GL_ID parent_gltf_node) {
    GLTFContext *context = &app_state->gltf_context;

    const StringView node_name = RuntimeNode_get_prop_str(node, "name");
    const StringView node_hash_name = find_name32(node->name_hash);

    GL_ID output_node;
    if (sv_is_not_null(node_name)) {
        output_node = GLTFContext_node_add(context, StringView_cstr(node_name));
    }
    else if (sv_is_not_null(node_hash_name)) {
        output_node = GLTFContext_node_add(context, StringView_cstr(node_hash_name));
    }
    else {
        char generated_name[64];
        snprintf(generated_name, sizeof(generated_name), "node_%08X", node->name_hash);
        output_node = GLTFContext_node_add(context, generated_name);
    }

    set_world_matrix(context, output_node, node);
    add_extras(context, node, output_node);

    const GL_ID current_skin = GLTFContext_current_skin(context);
    if (context->skins.count > 0 && IS_VALID_GL_ID(current_skin)) {
        const StringView parent_bone_name = RuntimeNode_get_prop_by_hash_str(node, 0x87becf63);
        // mat4 node_global_matrix = {0};
        const GL_ID parent_bone_id = GLTFContext_skin_find_bone_by_name(context, current_skin,
                                                                        StringView_cstr(parent_bone_name));
        if (IS_VALID_GL_ID(parent_bone_id)) {
            // calculate_global_node_matrix(context, parent_bone_id, node_global_matrix);
            // glm_mat4_inv(node_global_matrix, node_global_matrix);
            // GLTFContext_node_set_matrix(context, output_node, (float *) node_global_matrix);
            GLTFContext_node_set_parent(context, output_node, parent_bone_id);
        }
        else {
            GLog_Warning("Parent bone not found: %s", StringView_cstr(parent_bone_name));
            GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
        }
    }
    else if (IS_VALID_GL_ID(parent_gltf_node))
        GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
    else {
        GLog_Warning("Invalid parent setup: 0x%08X", node->name_hash);
    }

    process_children(app_state, node, path_hash, path, output_node);
}

void handle_default(AppState *app_state,
                    RuntimeNode *node, const uint32 path_hash, const StringView path,
                    const GL_ID parent_gltf_node) {
    GLTFContext *context = &app_state->gltf_context;

    const StringView node_name = RuntimeNode_get_prop_str(node, "name");
    const StringView node_name_hash = find_name32(node->name_hash);

    GL_ID output_node;
    if (sv_is_not_null(node_name)) {
        output_node = GLTFContext_node_add(context, StringView_cstr(node_name));
    }
    else if (sv_is_not_null(node_name_hash)) {
        output_node = GLTFContext_node_add(context, StringView_cstr(node_name_hash));
    }
    else {
        char generated_name[64];
        snprintf(generated_name, sizeof(generated_name), "node_%08X", node->name_hash);
        output_node = GLTFContext_node_add(context, generated_name);
    }

    add_extras(context, node, output_node);
    set_world_matrix(context, output_node, node);

    if (IS_VALID_GL_ID(parent_gltf_node))
        GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
    else {
        GLog_Warning("Invalid parent setup: 0x%08X", node->name_hash);
    }

    DA_FORI(node->children, i) {
        process_epe_node(app_state, DA_at(&node->children, i),
                         path_hash,
                         path,
                         output_node);
    }
}

void process_epe_node(AppState *app_state,
                      RuntimeNode *node, const uint32 path_hash, const StringView path,
                      const GL_ID parent_gltf_node) {
    CHECK_APP_STATE(app_state);
    CHECK_GLTF_STATE(&app_state->gltf_context);

    if (!RuntimeNode_has_prop(node, "_class")) {
        return;
    }
    const StringView class_name = RuntimeNode_get_prop_str(node, "_class");
    if (sv_is_null(class_name)) {
        GLog_Error("Failed to get _class property");
        exit(1);
    }
    if (StringView_cequals(class_name, "CCharacter")) {
        handle_CCharacter(app_state, node, path_hash, path, parent_gltf_node);
    }
    else if (StringView_cequals(class_name, "CSecondaryMotionAttachment")) {
        handle_CSecondaryMotionAttachment(app_state, node, path_hash, path, parent_gltf_node);
    }
    else if (StringView_cequals(class_name, "CRigidObject")) {
        handle_CRigidObject(app_state, node, path_hash, path, parent_gltf_node);
    }
    else if (StringView_cequals(class_name, "CDamageableCharacterPart")) {
        handle_CDamageableCharacterPart(app_state, node, path_hash, path, parent_gltf_node);
    }
    else if (StringView_cequals(class_name, "CSkeletalAnimatedObject")) {
        handle_CSkeletalAnimatedObject(app_state, node, path_hash, path, parent_gltf_node);
    }
    else if (StringView_cequals(class_name, "CBoneAttachment")) {
        handle_CBoneAttachment(app_state, node, path_hash, path, parent_gltf_node);
    }
    else {
        handle_default(app_state, node, path_hash, path, parent_gltf_node);
    }
}

GL_ID export_epe(AppState *app_state, RuntimeNode *root_node, uint32 path_hash, const StringView path) {
    CHECK_APP_STATE(app_state);
    CHECK_GLTF_STATE(&app_state->gltf_context);
    GLTFContext *context = &app_state->gltf_context;

    if (sv_is_null(path)) {
        GLog_Error("Path is NULL");
        exit(1);
    }
    String epe_export_path = {};
    Path_join(&epe_export_path, &app_state->export_path);
    Path_join_sv(&epe_export_path, path);
    Path_ensure_parent_dirs(&epe_export_path);
    String_append_cstr(&epe_export_path, ".gltf");
    GLTFContext_set_save_path(context, &epe_export_path);
    String_free(&epe_export_path);

    const GL_ID epe_root_node_id = GLTFContext_node_add(context, "epe_root");

    DA_FORI(root_node->children, i) {
        process_epe_node(app_state, DA_at(&root_node->children, i), path_hash,
                         path,
                         epe_root_node_id);
    }
    return epe_root_node_id;
}
