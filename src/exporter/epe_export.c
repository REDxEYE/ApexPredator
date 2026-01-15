// Created by RED on 12.01.2026.

#include "exporter/epe_export.h"

#include "apex/hashes.h"
#include "exporter/havok_export.h"
#include "exporter/adf_export.h"
#include "exporter/common_export.h"
#include "utils/path.h"

void add_extras(const GLTFContext *context, const RuntimeNode *node, const GL_ID output_node) {
    String extra_data = {0};
    String_append_cstr(&extra_data, "{");
    DA_FORI(node->props, i) {
        RuntimeProp_emit_json(&node->props.items[i], &extra_data, 1);
        if (i + 1 < node->props.count)
            String_append_cstr(&extra_data, ", ");
    }
    String_append_cstr(&extra_data, "}");
    GLTFContext_node_set_extra(context, output_node, String_data(&extra_data));
    String_free(&extra_data);
}

void calculate_global_node_matrix(const GLTFContext *context, const GL_ID target_id, mat4 out) {
    if (!IS_VALID_GL_ID(target_id)) {
        glm_mat4_identity(out);
        return;
    }
    const cgltf_node *target_node = &context->nodes.items[target_id.v];
    if (target_node->parent != NULL) {
        mat4 parent_matrix = {0};
        const GL_ID parent_id = gltf_untag_index(target_node->parent);
        calculate_global_node_matrix(context, parent_id, parent_matrix);
        mat4 local_matrix = {0};
        if (target_node->has_matrix) {
            memcpy(local_matrix, target_node->matrix, sizeof(float) * 16);
        } else {
            glm_mat4_identity(local_matrix);
        }
        glm_mat4_mul(parent_matrix, local_matrix, out);
    } else {
        if (target_node->has_matrix) {
            memcpy(out, target_node->matrix, sizeof(float) * 16);
        } else {
            glm_mat4_identity(out);
        }
    }
}

void process_children(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                      Havok_TypeLibrary *havok_lib, RuntimeNode *node, const uint32 path_hash, const String *path,
                      const String *export_path, const GL_ID parent_gltf_node) {
    DA_FORI(node->children, i) {
        process_epe_node(context, archive_manager, lib, havok_lib,
                         DA_at(&node->children, i),
                         path_hash,
                         path, export_path, parent_gltf_node);
    }
}

void set_world_matrix(GLTFContext *context, const GL_ID gltf_node, RuntimeNode *node) {
    if (!RuntimeNode_has_prop(node, "world"))
        return;
    const float32 *matrix = RuntimeNode_get_prop_mat4x4(node, "world");
    if (matrix != NULL && memcmp(IDENTITY_MAT, matrix, sizeof(mat4)) != 0)
        GLTFContext_node_set_matrix(context, gltf_node, matrix);
}

void handle_CCharacter(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                      Havok_TypeLibrary *havok_lib, RuntimeNode *node, const uint32 path_hash, const String *path,
                      const String *export_path, const GL_ID parent_gltf_node) {
    const String *model_filename = RuntimeNode_get_prop_by_hash_str(node, 0xE8129FE6);
    const String *skeleton_filename = RuntimeNode_get_prop_by_hash_str(node, 0x26FA86FE);
    const String *node_name_hash = &node->name;
    if (model_filename == NULL) {
        printf("[ERROR]: Failed to get model property for CCharacter\n");
        return;
    }

    String skeleton_bsk_name = {0};
    Path_replace_extension(skeleton_filename, "bsk", &skeleton_bsk_name);

    const GL_ID skin_id = export_file(context, archive_manager, lib, havok_lib, &skeleton_bsk_name,
                                      hash_string(&skeleton_bsk_name),
                                      export_path);
    const cgltf_skin *skin = &context->skins.items[skin_id.v];
    const GL_ID root_bone = skin->joints[0] != NULL ? gltf_untag_index(skin->joints[0]) : INVALID_GL_ID;
    if (IS_VALID_GL_ID(root_bone)) {
        GLTFContext_node_set_parent(context, parent_gltf_node, root_bone);
    }

    GLTFContext_push_skin(context, skin_id);
    const GL_ID output_node = export_adf_file(context, archive_manager, lib, havok_lib, model_filename,
                              hash_string(model_filename),
                              export_path);

    add_extras(context, node, output_node);
    set_world_matrix(context, output_node, node);
    if (IS_VALID_GL_ID(parent_gltf_node))
        GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
    else {
        printf("Invalid parent setup: %s\n", String_data(node_name_hash));
    }

    process_children(context, archive_manager, lib, havok_lib, node, path_hash, path, export_path, output_node);
    GLTFContext_pop_skin(context);
}

void handle_CSecondaryMotionAttachment(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                    Havok_TypeLibrary *havok_lib, RuntimeNode *node, const uint32 path_hash, const String *path,
                    const String *export_path, const GL_ID parent_gltf_node) {
    const String *model_filename = RuntimeNode_get_prop_str(node, "model");
    const String *skeleton_filename = RuntimeNode_get_prop_by_hash_str(node, 0x26FA86FE);
    if (model_filename == NULL) {
        printf("[ERROR]: Failed to get model property for CSecondaryMotionAttachment\n");
        return;
    }
    String skeleton_bsk_name = {0};
    Path_replace_extension(skeleton_filename, "bsk", &skeleton_bsk_name);
    const GL_ID skin_id = export_file(context, archive_manager, lib, havok_lib, &skeleton_bsk_name,
                                hash_string(&skeleton_bsk_name),
                                export_path);
    GLTFContext_push_skin(context, skin_id);
    const GL_ID  output_node = export_adf_file(context, archive_manager, lib, havok_lib, model_filename, hash_string(model_filename),
                              export_path);
    set_world_matrix(context, output_node, node);
    add_extras(context, node, output_node);
    if (IS_VALID_GL_ID(parent_gltf_node))
        GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
    else {
        const String *node_name_hash = &node->name;
        printf("Invalid parent setup: %s\n", String_data(node_name_hash));
    }
    process_children(context, archive_manager, lib, havok_lib, node, path_hash, path, export_path, output_node);
    GLTFContext_pop_skin(context);
}

void handle_CDamageableCharacterPart(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                                     Havok_TypeLibrary *havok_lib, RuntimeNode *node, const uint32 path_hash, const String *path,
                                     const String *export_path, const GL_ID parent_gltf_node) {
    const String *node_name = RuntimeNode_get_prop_str(node, "name");
    const String *node_name_hash = &node->name;


    const GL_ID output_node = GLTFContext_node_add(
        context, node_name != NULL ? String_data(node_name) : String_data(node_name_hash));

    set_world_matrix(context, output_node, node);
    add_extras(context, node, output_node);

    const GL_ID current_skin = GLTFContext_current_skin(context);
    if (context->skins.count > 0 && IS_VALID_GL_ID(current_skin)) {
        const String *parent_bone_name = RuntimeNode_get_prop_by_hash_str(node, 0x4d67eec5);
        mat4 node_global_matrix = {0};
        const GL_ID parent_bone_id = GLTFContext_skin_find_bone_by_name(context, current_skin,
                                                                        String_data(parent_bone_name));

        calculate_global_node_matrix(context, parent_bone_id, node_global_matrix);
        glm_mat4_inv(node_global_matrix, node_global_matrix);
        GLTFContext_node_set_matrix(context, output_node, (float *) node_global_matrix);
        GLTFContext_node_set_parent(context, output_node, parent_bone_id);
    } else if (IS_VALID_GL_ID(parent_gltf_node))
        GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
    else {
        printf("Invalid parent setup: %s\n", String_data(node_name_hash));
    }

    process_children(context, archive_manager, lib, havok_lib, node, path_hash, path, export_path, output_node);
}

void handle_CRigidObject(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                         Havok_TypeLibrary *havok_lib, RuntimeNode *node, const uint32 path_hash, const String *path,
                         const String *export_path, const GL_ID parent_gltf_node) {
    const uint32 model_filename_hash = RuntimeNode_get_prop_u32(node, "filename");
    const String* model_filename = find_name32(model_filename_hash); // Could be null
    const String *node_name_hash = &node->name;
    if (model_filename_hash == 0) {
        printf("[ERROR]: Failed to get model property for CRigidObject\n");
        return;
    }
    GL_ID output_node = export_adf_file(context, archive_manager, lib, havok_lib, model_filename, model_filename_hash, export_path);
    if (IS_VALID_GL_ID(output_node)) {
        set_world_matrix(context, output_node, node);

        if (IS_VALID_GL_ID(parent_gltf_node))
            GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
        else {
            printf("Invalid parent setup: %s\n", String_data(node_name_hash));
        }
    }else {
        output_node = GLTFContext_node_add(
            context, String_data(node_name_hash));
        set_world_matrix(context, output_node, node);
        add_extras(context, node, output_node);
    }
    process_children(context, archive_manager, lib, havok_lib, node, path_hash, path, export_path, output_node);
}

void handle_CSkeletalAnimatedObject(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                                    Havok_TypeLibrary *havok_lib, RuntimeNode *node, const uint32 path_hash, const String *path,
                                    const String *export_path, const GL_ID parent_gltf_node) {
    const String* model_filename = RuntimeNode_get_prop_by_hash_str(node, 0x0f94740b);
    const String* skeleton_filename = RuntimeNode_get_prop_by_hash_str(node, 0x26fa86fe);

    if (model_filename == NULL) {
        printf("[ERROR]: Failed to get model property for CSkeletalAnimatedObject\n");
        return;
    }

    String skeleton_bsk_name = {0};
    Path_replace_extension(skeleton_filename, "bsk", &skeleton_bsk_name);

    const GL_ID skin_id = export_file(context, archive_manager, lib, havok_lib, &skeleton_bsk_name,
                                      hash_string(&skeleton_bsk_name),
                                      export_path);
    const cgltf_skin *skin = &context->skins.items[skin_id.v];
    const GL_ID root_bone = skin->joints[0] != NULL ? gltf_untag_index(skin->joints[0]) : INVALID_GL_ID;
    if (IS_VALID_GL_ID(root_bone)) {
        GLTFContext_node_set_parent(context, parent_gltf_node, root_bone);
    }

    GLTFContext_push_skin(context, skin_id);
    const GL_ID output_node = export_adf_file(context, archive_manager, lib, havok_lib, model_filename,
                              hash_string(model_filename),
                              export_path);

    add_extras(context, node, output_node);
    set_world_matrix(context, output_node, node);
    if (IS_VALID_GL_ID(parent_gltf_node))
        GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
    else {
        const String *node_name_hash = &node->name;
        printf("Invalid parent setup: %s\n", String_data(node_name_hash));
    }

    process_children(context, archive_manager, lib, havok_lib, node, path_hash, path, export_path, output_node);
    GLTFContext_pop_skin(context);
}

void handle_CBoneAttachment(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                    Havok_TypeLibrary *havok_lib, RuntimeNode *node, const uint32 path_hash, const String *path,
                    const String *export_path, const GL_ID parent_gltf_node) {
    const String *node_name = RuntimeNode_get_prop_str(node, "name");
    const String *node_name_hash = &node->name;

    const GL_ID output_node = GLTFContext_node_add(
            context, node_name != NULL ? String_data(node_name) : String_data(node_name_hash));

    set_world_matrix(context, output_node, node);
    add_extras(context, node, output_node);

    const GL_ID current_skin = GLTFContext_current_skin(context);
    if (context->skins.count > 0 && IS_VALID_GL_ID(current_skin)) {
        const String *parent_bone_name = RuntimeNode_get_prop_by_hash_str(node, 0x87becf63);
        mat4 node_global_matrix = {0};
        const GL_ID parent_bone_id = GLTFContext_skin_find_bone_by_name(context, current_skin,
                                                                        String_data(parent_bone_name));
        if (IS_VALID_GL_ID(parent_bone_id)) {
            calculate_global_node_matrix(context, parent_bone_id, node_global_matrix);
            glm_mat4_inv(node_global_matrix, node_global_matrix);
            GLTFContext_node_set_matrix(context, output_node, (float *) node_global_matrix);
            GLTFContext_node_set_parent(context, output_node, parent_bone_id);
        }else {
            printf("Parent bone not found: %s\n", String_data(parent_bone_name));
            GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
        }
    } else if (IS_VALID_GL_ID(parent_gltf_node))
        GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
    else {
        printf("Invalid parent setup: %s\n", String_data(node_name_hash));
    }

    process_children(context, archive_manager, lib, havok_lib, node, path_hash, path, export_path, output_node);
}

void handle_default(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                    Havok_TypeLibrary *havok_lib, RuntimeNode *node, const uint32 path_hash, const String *path,
                    const String *export_path, const GL_ID parent_gltf_node) {
    const String *node_name = RuntimeNode_get_prop_str(node, "name");
    const String *node_name_hash = &node->name;

    const GL_ID output_node = GLTFContext_node_add(
        context, node_name != NULL ? String_data(node_name) : String_data(node_name_hash));

    add_extras(context, node, output_node);
    set_world_matrix(context, output_node, node);

    if (IS_VALID_GL_ID(parent_gltf_node))
        GLTFContext_node_set_parent(context, output_node, parent_gltf_node);
    else {
        printf("Invalid parent setup: %s\n", String_data(node_name_hash));
    }

    DA_FORI(node->children, i) {
        process_epe_node(context, archive_manager, lib, havok_lib,
                         DA_at(&node->children, i),
                         path_hash,
                         path, export_path, output_node);
    }
}

void process_epe_node(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                      Havok_TypeLibrary *havok_lib, RuntimeNode *node, const uint32 path_hash, const String *path,
                      const String *export_path, const GL_ID parent_gltf_node) {
    assert(context!=NULL && "context must be initialized");
    if (!RuntimeNode_has_prop(node, "_class")) {
        return;
    }
    const String *class_name = RuntimeNode_get_prop_str(node, "_class");
    if (class_name == NULL) {
        printf("[ERROR]: Failed to get _class property\n");
        exit(1);
    }
    if (String_cequals(class_name, "CCharacter")) {
        handle_CCharacter(context, archive_manager, lib, havok_lib, node, path_hash, path, export_path, parent_gltf_node);
    }else if (String_cequals(class_name, "CSecondaryMotionAttachment")) {
        handle_CSecondaryMotionAttachment(context, archive_manager, lib, havok_lib, node, path_hash, path, export_path, parent_gltf_node);
    }else if (String_cequals(class_name, "CRigidObject")) {
        handle_CRigidObject(context, archive_manager, lib, havok_lib, node, path_hash, path, export_path, parent_gltf_node);
    }else if (String_cequals(class_name, "CDamageableCharacterPart")) {
        handle_CDamageableCharacterPart(context, archive_manager, lib, havok_lib, node, path_hash, path, export_path, parent_gltf_node);
    }else if (String_cequals(class_name, "CSkeletalAnimatedObject")) {
        handle_CSkeletalAnimatedObject(context, archive_manager, lib, havok_lib, node, path_hash, path, export_path, parent_gltf_node);
    }else if (String_cequals(class_name, "CBoneAttachment")){
        handle_CBoneAttachment(context, archive_manager, lib, havok_lib, node, path_hash, path, export_path, parent_gltf_node);
    } else {
        handle_default(context, archive_manager, lib, havok_lib, node, path_hash, path, export_path, parent_gltf_node);
    }
}

GL_ID export_epe(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                 Havok_TypeLibrary *havok_lib, RuntimeNode *root_node, uint32 path_hash, const String *path,
                 const String *export_path) {
    assert(context!=NULL && "context must be initialized");
    if (path == NULL) {
        printf("[ERROR]: Path is NULL\n");
        exit(1);
    }
    String epe_export_path = {};
    Path_join(&epe_export_path, export_path);
    Path_join(&epe_export_path, path);
    Path_ensure_parent_dirs(&epe_export_path);
    String_append_cstr(&epe_export_path, ".gltf");
    GLTFContext_set_save_path(context, &epe_export_path);

    String epe_name = {};
    Path_filename(path, &epe_name);

    const GL_ID epe_root_node_id = GLTFContext_node_add(context, "epe_root");

    DA_FORI(root_node->children, i) {
        process_epe_node(context, archive_manager, lib, havok_lib, DA_at(&root_node->children, i),
                         path_hash,
                         path, export_path, epe_root_node_id);
    }
    return epe_root_node_id;
}
