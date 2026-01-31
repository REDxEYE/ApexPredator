#include <stdio.h>

#include "exporter/havok_export.h"
#include "platform/cli_parser.h"
#include "utils/common.h"


#ifdef _WIN32
#include "Windows.h"
#else
#include <unistd.h>
#endif

#include "havok/generated/havok_generated.h"
#include "apex/hashes.h"
#include "apex/adf/adf_types.h"
#include "apex/package/tab_archive.h"
#include "havok/havok_codegen.h"
#include "platform/archive_manager.h"
#include "utils/hash_helper.h"
#include "utils/path.h"
#include "utils/string.h"
#include "utils/gltf/cgltf_helper.h"

#include "exporter/amf_export.h"
#include "exporter/common_export.h"
#include "platform/logger.h"
#include "utils/memory_tracker.h"

#include "tracy/TracyC.h"

const CommandArgument extract_arguments[] = {
    {
        .name = "paths",
        .flag = NULL,
        .description = "Paths to the assets to extract.",
        .type = COMMAND_ARG_TYPE_ARRAY_STRING,
        .named = false,
        .required = true,
        .has_default = false,
    },
    {
        .name = "out_dir",
        .flag = "o",
        .description = "Output directory for extracted assets.",
        .type = COMMAND_ARG_TYPE_STRING,
        .named = true,
        .required = false,
        .has_default = true,
        .string_value = "./extracted",
    }
};

const CommandArgument extract_anim_arguments[] = {
    {
        .name = "skeleton-path",
        .description = "Path(or hash) to the Havok animation container file containing the skeleton.",
        .type = COMMAND_ARG_TYPE_STRING,
        .named = false,
        .required = true,
        .has_default = false,
    },
    {
        .name = "animations",
        .description = "Paths(or hashes) to the Havok animation container files to extract animations from.",
        .type = COMMAND_ARG_TYPE_ARRAY_STRING,
        .named = false,
        .required = true,
        .has_default = false,
    },
    {
        .name = "out_dir",
        .flag = "o",
        .description = "Output directory for extracted animations.",
        .type = COMMAND_ARG_TYPE_STRING,
        .named = true,
        .required = false,
        .has_default = true,
        .string_value = "./extracted_anims",
    }
};

const SubCommand sub_commands[] = {
    {
        .name = "extract",
        .description = "Extract assets.",
        .execute = NULL,
        .argument_count = sizeof(extract_arguments) / sizeof(CommandArgument),
        .arguments = extract_arguments,
    },
    {
        .name = "extract-anims",
        .description = "Extract animations from a Havok animation container.",
        .execute = NULL,
        .argument_count = sizeof(extract_anim_arguments) / sizeof(CommandArgument),
        .arguments = extract_anim_arguments,
    }
};

const CliSpec cli_spec = {
    .prog = NULL,
    .root_name = "game_root",
    .root_desc = "Path to the root directory of the game installation.",
    .commands = sub_commands,
    .command_count = sizeof(sub_commands) / sizeof(SubCommand),
};

int main(int argc, const char *argv[]) {
    mp_init();
    CliResult cli_res;
    const char *cli_error;
    const CliStatus cli_status = cli_parse(&cli_spec, &cli_res, argc, argv, &cli_error);
    if (cli_status != CLI_OK) {
        cli_print_help(&cli_spec, argv[0], stdout);
        if (cli_error) {
            GLog_Error("Error parsing command line: %s", cli_error);
        }
        else {
            GLog_Error("Error parsing command line.");
        }
        mp_shutdown();
        cli_free(&cli_res);
        return -1;
    }
    //     while (!TracyCIsConnected) {
    // #ifdef _WIN32
    //         Sleep(100); /* Windows */
    // #else
    //         usleep(10000);
    // #endif
    //         printf("\rWaiting for tracy;");
    //     }
    //     printf("\n");

    TracyCZoneN(ctx, "App", 1);
    ArchiveManager manager = {0};
    ArchiveManager_init(&manager);
    ArchiveManager_set_archive_loader_function(&manager, mount_archive);

    STI_TypeLibrary lib = {0};
    Havok_TypeLibrary havok_lib = {0};
    Havok_TypeLibrary_init(&havok_lib);
    STI_TypeLibrary_init(&lib);
    STI_ADF_TYPES_register_functions(&lib);
    HAVOK_TYPES_register_functions();

    String game_root = {0};
    String tmp = {0};
    String_from_cstr(&tmp, cli_res.game_root);
    Path_convert_to_wsl(&tmp, &game_root);
    String_free(&tmp);

    String export_path = {};
    const char *export_path_cstr = NULL;
    cli_get_string(&cli_res, "out_dir", &export_path_cstr);
    String_from_cstr(&export_path, export_path_cstr);

    TabArchives_init(&manager, &game_root);

    GLTFContext context = {0};

    // String_from_cstr(&file_path, "editor/entities/characters/machines/dreadnought/drea_classb_load01.epe");

    if (strcmp(cli_res.cmd->name, "extract") == 0) {
        const char **file_paths = NULL;
        size_t file_path_count = 0;
        cli_get_array_string(&cli_res, "paths", &file_paths, &file_path_count);
        for (int file_id = 0; file_id < file_path_count; ++file_id) {
            GLTFContext_init(&context, "root");
            String file_path = {0};
            String_from_cstr(&file_path, file_paths[file_id]);

            Path_normalize_posix(&file_path);
            export_file(&context, &manager, &lib, &file_path, hash_string(&file_path), &export_path);


            GLTFContext_write_and_free(&context);
            String_free(&file_path);
        }
    }
    else if (strcmp(cli_res.cmd->name, "extract-anims") == 0) {
        const char *skeleton_path_cstr = NULL;

        cli_get_string(&cli_res, "skeleton-path", &skeleton_path_cstr);
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
        if (!ArchiveManager_get_file_by_hash(&manager, skeleton_path_hash, &mb)) {
            GLog_Error("Skeleton file not found: %s", skeleton_path_cstr);
            goto END_CLEANUP;
        }

        TagFile skeleton_tag_file = {0};

        if (memcmp(mb.data + 4, "TAG0", 4) == 0) {
            TagFile_from_buffer(&skeleton_tag_file, (Buffer *) &mb);
            Buffer_close((Buffer *) &mb);
        }
        else {
            GLog_Error("Skeleton file is not a valid Havok TAG0 file.");
            Buffer_close((Buffer *) &mb);
            goto END_CLEANUP;
        }

        TypedPtr *skeleton_item = TagFile_get_item(&skeleton_tag_file, 0);
        if (skeleton_item->type_info_->hash != hkRootLevelContainer_HASH) {
            GLog_Error("Skeleton file does not contain a hkRootLevelContainer: %s", skeleton_path_cstr);
        INVALID_SKELETON_CLEANUP:
            TagFile_free_item(skeleton_item);
            TagFile_free(&skeleton_tag_file);
            goto END_CLEANUP;
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
        cli_get_array_string(&cli_res, "animations", &file_paths, &file_path_count);

        for (int file_id = 0; file_id < file_path_count; ++file_id) {
            const char *animation_path_cstr = file_paths[file_id];
            uint32 animation_path_hash = 0;
            String *animation_path_tmp = NULL;
            if (is_hex(animation_path_cstr)) {
                animation_path_hash = parse_hex_u32(animation_path_cstr);
                animation_path_tmp = find_name32(animation_path_hash);
            }
            else if (is_digits(animation_path_cstr)) {
                animation_path_hash = parse_digits_u32(animation_path_cstr);
                animation_path_tmp = find_name32(animation_path_hash);
            }
            else {
                animation_path_tmp = String_new_from_cstr(animation_path_cstr);
                Path_normalize_posix(animation_path_tmp);
                animation_path_hash = hash_string(animation_path_tmp);
            }

            if (!ArchiveManager_get_file_by_hash(&manager, animation_path_hash, &mb)) {
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
                if (animation_path_tmp!=NULL) {
                    String_free(animation_path_tmp);
                }
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
            if (animation_path_tmp==NULL) {
                animation_path_tmp = String_new(16);
                String_format(animation_path_tmp, "anim_%08X", animation_path_hash);
            }

            String anim_file_name = {0};
            Path_filename(animation_path_tmp, &anim_file_name);

            GLTFContext_init(&context, String_cstr(&anim_file_name));
            export_animation(&context, binding, skeleton, animation_path_tmp);

            String export_file_path = {0};
            String_copy_from(&export_file_path, &export_path);
            Path_join(&export_file_path, animation_path_tmp);
            Path_ensure_parent_dirs(&export_file_path);
            Path_replace_extension(&export_file_path, "gltf", &anim_file_name);
            GLTFContext_set_save_path(&context, &anim_file_name);
            String_free(&anim_file_name);
            String_free(&export_file_path);
            GLTFContext_write_and_free(&context);

            String_free(animation_path_tmp);
            TagFile_free_item(anim_item);
            TagFile_free(&anim_tag_file);
        }
        TagFile_free_item(skeleton_item);
        TagFile_free(&skeleton_tag_file);
    }
END_CLEANUP:
    ArchiveManager_free(&manager);
    STI_TypeLibrary_free(&lib);
    Havok_TypeLibrary_free(&havok_lib);
    String_free(&tmp);
    String_free(&game_root);
    String_free(&export_path);
    DM_free(&HAVOK_TYPES_type_info);
    cli_free(&cli_res);
    close_assets_db();
    TracyCZoneEnd(ctx);
    return 0;
}
