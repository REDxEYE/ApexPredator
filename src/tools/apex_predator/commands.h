// Created by RED on 12.02.2026.

#ifndef APEXPREDATOR_COMMANDS_H
#define APEXPREDATOR_COMMANDS_H
#include "platform/app_state.h"
#include "platform/cli_parser.h"

static const CommandArgument extract_arguments[] = {
    {
        .name = "game_root",
        .flag = "g",
        .description = "Path to the root directory of the game assets (enerationZero\\archives_win64).",
        .type = COMMAND_ARG_TYPE_STRING,
        .named = false,
        .required = true,
        .has_default = false,
    },
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
        .name = "no_textures",
        .flag = "n",
        .description = "Don't export textures.",
        .type = COMMAND_ARG_TYPE_BOOL,
        .named = true,
        .required = false,
        .has_default = true,
        .bool_value = false,
    },
    {
        .name = "raw",
        .flag = "r",
        .description = "Export raw data without converting to glTF.",
        .type = COMMAND_ARG_TYPE_BOOL,
        .named = true,
        .required = false,
        .has_default = true,
        .bool_value = false,
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
    },
    {
        .name = "db_path",
        .flag = "d",
        .description =
        "Path to the hashes.db file for resolving asset paths from hashes. Required if using hashes instead of paths.",
        .type = COMMAND_ARG_TYPE_STRING,
        .named = true,
        .required = false,
        .has_default = true,
        .string_value = "./hashes.db",
    }
};

static const CommandArgument extract_anim_arguments[] = {
    {
        .name = "game_root",
        .flag = "g",
        .description = "Path to the root directory of the game assets (enerationZero\\archives_win64).",
        .type = COMMAND_ARG_TYPE_STRING,
        .named = false,
        .required = true,
        .has_default = false,
    },
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
        .string_value = "./extracted",
    },
    {
        .name = "db_path",
        .flag = "d",
        .description =
        "Path to the hashes.db file for resolving asset paths from hashes. Required if using hashes instead of paths.",
        .type = COMMAND_ARG_TYPE_STRING,
        .named = true,
        .required = false,
        .has_default = true,
        .string_value = "./hashes.db",
    }
};

static const CommandArgument search_arguments[] = {
    {
        .name = "query",
        .description = "Hash or path or pattern to search for. Uses sqlite3 syntax for wildcards (%)",
        .type = COMMAND_ARG_TYPE_STRING,
        .named = false,
        .required = true,
        .has_default = false,
    },
    {
        .name = "db_path",
        .flag = "d",
        .description =
        "Path to the hashes.db file for resolving asset paths from hashes. Required if using hashes instead of paths.",
        .type = COMMAND_ARG_TYPE_STRING,
        .named = true,
        .required = false,
        .has_default = true,
        .string_value = "./hashes.db",
    }
};


void extract_handler(AppState* app_state, const CliResult* cli_res);
void extract_anims_handler(AppState* app_state, const CliResult* cli_res);
void search_handler(AppState* app_state, const CliResult* cli_res);


static const SubCommand sub_commands[] = {
    {
        .name = "extract",
        .description = "Extract assets.",
        .execute = (execute_fn)extract_handler,
        .argument_count = sizeof(extract_arguments) / sizeof(CommandArgument),
        .arguments = extract_arguments,
    },
    {
        .name = "extract-anims",
        .description = "Extract animations from a Havok animation container.",
        .execute = (execute_fn)extract_anims_handler,
        .argument_count = sizeof(extract_anim_arguments) / sizeof(CommandArgument),
        .arguments = extract_anim_arguments,
    },
    {
        .name = "search",
        .description = "Search for assets by hash or path or pattern.",
        .execute = (execute_fn)search_handler,
        .argument_count = sizeof(search_arguments) / sizeof(CommandArgument),
        .arguments = search_arguments,
    }
};

extern const CliSpec cli_spec;

#endif //APEXPREDATOR_COMMANDS_H