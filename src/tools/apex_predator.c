#include <stdio.h>

#include "platform/cli_parser.h"


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
        .name = "out-dir",
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

    const char **file_paths = NULL;
    size_t file_path_count = 0;
    cli_get_array_string(&cli_res, "paths", &file_paths, &file_path_count);
    for (int file_id = 0; file_id < file_path_count; ++file_id) {
        GLTFContext_init(&context, "root");
        String file_path = {};
        String_from_cstr(&file_path, file_paths[file_id]);

        Path_normalize_posix(&file_path);
        export_file(&context, &manager, &lib, &file_path, hash_string(&file_path), &export_path);


        GLTFContext_write_and_free(&context);
        String_free(&file_path);
    }


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
