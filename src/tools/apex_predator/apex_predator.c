#include <stdio.h>

#include "platform/app_state.h"
#include "platform/cli_parser.h"


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#else
#include <unistd.h>
#endif

#include "havok/generated/havok_generated.h"
#include "apex/hashes.h"
#include "apex/adf/adf_types.h"
#include "apex/package/tab_archive.h"
#include "platform/archive_manager.h"
#include "utils/path.h"
#include "utils/string.h"
#include "exporter/common_export.h"
#include "platform/logger.h"
#include "utils/memory_tracker.h"

#include "tracy/TracyC.h"

#include "commands.h"

const CliSpec cli_spec = {
    .prog = NULL,
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
    printf("\n");
    TracyCZoneN(ctx, "App", 1);

    String db_path = {0};
    cli_get_string(&cli_res, "db_path", &db_path);
    set_db_path(String_cstr(&db_path));

    AppState app_state = {};

    ArchiveManager_init(&app_state.archive_manager);
    ArchiveManager_set_archive_loader_function(&app_state.archive_manager, mount_archive);

    STI_ADF_TYPES_register_functions();
    HAVOK_TYPES_register_functions();

    cli_get_string(&cli_res, "game_root", &app_state.game_root);
    Path_convert_to_wsl(&app_state.game_root);
    Path_normalize_native(&app_state.game_root);

    cli_get_string(&cli_res, "out_dir", &app_state.export_path);
    Path_convert_to_wsl(&app_state.export_path);
    Path_normalize_native(&app_state.export_path);

    TabArchives_init(&app_state.archive_manager, &app_state.game_root);

    cli_res.cmd->execute(&app_state, &cli_res);

    ArchiveManager_free(&app_state.archive_manager);
    // STI_TypeLibrary_free(&lib);
    DM_free(&HAVOK_TYPES_type_info);
    DM_free(&ADF_TYPES_type_info);
    cli_free(&cli_res);
    close_assets_db();
    AppState_free(&app_state);
    TracyCZoneEnd(ctx);
    return 0;
}
