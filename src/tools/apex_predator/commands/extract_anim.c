// Created by RED on 16.02.2026.

#include "exporter/havok_export.h"
#include "havok/havok_helpers.h"
#include "havok/generated/havok_generated.h"
#include "havok/tag_file/havok_tag_file.h"
#include "platform/app_state.h"
#include "platform/cli_parser.h"
#include "platform/logger.h"
#include "utils/common.h"
#include "utils/hash_helper.h"
#include "utils/path.h"

void extract_anims_handler(AppState *app_state, const CliResult *cli_res) {
    const char *skeleton_path_cstr = NULL;

    cli_get_cstring(cli_res, "skeleton-path", &skeleton_path_cstr);
    uint32 skeleton_path_hash = 0;
    if (is_hex(skeleton_path_cstr)) {
        skeleton_path_hash = parse_hex_u32(skeleton_path_cstr);
    }
    else if (is_digits(skeleton_path_cstr)) {
        skeleton_path_hash = parse_digits_u32(skeleton_path_cstr);
    }
    else {
        String skeleton_path_tmp = {0};
        String_from_cstr(&skeleton_path_tmp, skeleton_path_cstr);
        Path_normalize_posix(&skeleton_path_tmp);
        skeleton_path_hash = hash_string(&skeleton_path_tmp);
        String_free(&skeleton_path_tmp);
    }

    MemoryBuffer mb = {0};
    if (!ArchiveManager_get_file_by_hash(&app_state->archive_manager, skeleton_path_hash, &mb)) {
        GLog_Error("Skeleton file not found: %s", skeleton_path_cstr);
        return;
    }

    TagFile skeleton_tag_file = {0};

    if (memcmp(mb.data + 4, "TAG0", 4) == 0) {
        TagFile_from_buffer(&skeleton_tag_file, (Buffer *) &mb);
        Buffer_close((Buffer *) &mb);
    }
    else {
        GLog_Error("Skeleton file is not a valid Havok TAG0 file.");
        Buffer_close((Buffer *) &mb);
        return;
    }

    TypedPtr *skeleton_item = TagFile_get_item(&skeleton_tag_file, 0);
    if (skeleton_item->type_info_->hash != hkRootLevelContainer_HASH) {
        GLog_Error("Skeleton file does not contain a hkRootLevelContainer: %s", skeleton_path_cstr);
    INVALID_SKELETON_CLEANUP:
        TagFile_free_item(skeleton_item);
        TagFile_free(&skeleton_tag_file);
        return;
    }

    const hkRootLevelContainer *root_container = (hkRootLevelContainer *) skeleton_item;
    hkReferencedObject *variant = root_container->namedVariants.m_data->variant.ptr;
    if (variant->type_info_->hash != hkaAnimationContainer_HASH) {
        GLog_Error("Skeleton root_container file does not contain a hkaAnimationContainer: %s", skeleton_path_cstr);
        goto INVALID_SKELETON_CLEANUP;
    }
    const hkaAnimationContainer *animation_container = (const hkaAnimationContainer *) variant;
    const hkaSkeleton *skeleton = NULL;
    if (animation_container->skeletons.m_size > 0) {
        skeleton = animation_container->skeletons.m_data[0].ptr;
    }

    const char **file_paths = NULL;
    size_t file_path_count = 0;
    cli_get_array_string(cli_res, "animations", &file_paths, &file_path_count);

    for (int file_id = 0; file_id < file_path_count; ++file_id) {
        const char *animation_path_cstr = file_paths[file_id];
        uint32 animation_path_hash = 0;
        if (is_hex(animation_path_cstr)) {
            animation_path_hash = parse_hex_u32(animation_path_cstr);
        }
        else if (is_digits(animation_path_cstr)) {
            animation_path_hash = parse_digits_u32(animation_path_cstr);
        }
        else {
            String animation_path_tmp = {0};
            String_from_cstr(&animation_path_tmp, animation_path_cstr);
            Path_normalize_posix(&animation_path_tmp);
            animation_path_hash = hash_string(&animation_path_tmp);
        }

        if (!ArchiveManager_get_file_by_hash(&app_state->archive_manager, animation_path_hash, &mb)) {
            GLog_Error("Animation file not found: %s", animation_path_cstr);
            continue;
        }
        TagFile anim_tag_file = {0};

        if (memcmp(mb.data + 4, "TAG0", 4) == 0) {
            TagFile_from_buffer(&anim_tag_file, (Buffer *) &mb);
            Buffer_close((Buffer *) &mb);
        }
        else {
            GLog_Error("Skeleton file is not a valid Havok TAG0 file.");
            Buffer_close((Buffer *) &mb);
            TagFile_free(&skeleton_tag_file);
            continue;
        }

        // export_animation
        TypedPtr *anim_item = TagFile_get_item(&anim_tag_file, 0);
        if (anim_item->type_info_->hash != hkRootLevelContainer_HASH) {
            GLog_Error("Animation file does not contain a hkRootLevelContainer: %s", animation_path_cstr);
        INVALID_ANIM_CLEANUP:
            TagFile_free_item(anim_item);
            TagFile_free(&anim_tag_file);
            continue;
        }
        const hkRootLevelContainer *anim_root_container = (hkRootLevelContainer *) anim_item;
        hkReferencedObject *anim_variant = anim_root_container->namedVariants.m_data->variant.ptr;
        if (anim_variant->type_info_->hash != hkaAnimationContainer_HASH) {
            GLog_Error("Animation root_container file does not contain a hkaAnimationContainer: %s",
                       animation_path_cstr);
            goto INVALID_ANIM_CLEANUP;
        }
        const hkaAnimationContainer *anim_animation_container = (const hkaAnimationContainer *) anim_variant;
        assert(anim_animation_container->bindings.m_size==1 &&
            "Only single animation binding per container is supported currently.");

        const hkaAnimationBinding *binding = anim_animation_container->bindings.m_data[0].ptr;
        String anim_file_name = {0};
        StringView animation_path_tmp = find_name32_sv(animation_path_hash);
        if (sv_is_null(animation_path_tmp)) {
            String_format(&anim_file_name, "anim_%08X", animation_path_hash);
        }
        else {
            Path_filename_sv(animation_path_tmp, &anim_file_name);
        }

        GLTFContext_init(&app_state->gltf_context, String_cstr(&anim_file_name));
        String_free(&anim_file_name);

        export_animation(app_state, binding, skeleton, animation_path_tmp);

        String export_file_path = {0};
        String_copy_from(&export_file_path, &app_state->export_path);
        Path_join_sv(&export_file_path, animation_path_tmp);
        Path_ensure_parent_dirs(&export_file_path);
        Path_replace_extension_inplace(&export_file_path, "gltf");
        GLTFContext_set_save_path(&app_state->gltf_context, &export_file_path);
        String_free(&export_file_path);

        GLTFContext_write_and_free(&app_state->gltf_context);

        TagFile_free_item(anim_item);
        TagFile_free(&anim_tag_file);
    }
    TagFile_free_item(skeleton_item);
    TagFile_free(&skeleton_tag_file);
}
