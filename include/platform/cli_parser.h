// Created by RED on 31.01.2026.

#ifndef APEXPREDATOR_CLI_PARSER_H
#define APEXPREDATOR_CLI_PARSER_H
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "int_def.h"
#include "utils/memory_profiling.h"
#include "utils/string.h"

typedef void (*execute_fn)(int argc, const char *argv[]);

typedef enum CommandArgumentType {
    COMMAND_ARG_TYPE_STRING,
    COMMAND_ARG_TYPE_INT,
    COMMAND_ARG_TYPE_FLOAT,
    COMMAND_ARG_TYPE_BOOL,
    COMMAND_ARG_TYPE_ARRAY_STRING,
    COMMAND_ARG_TYPE_ARRAY_INT,
    COMMAND_ARG_TYPE_ARRAY_FLOAT
} CommandArgumentType;


typedef struct CommandArgument {
    const char *name;
    const char *flag;
    const char *description;
    CommandArgumentType type;
    bool named;
    bool required;
    bool has_default;

    union {
        const char *string_value;
        int int_value;
        float float_value;
        int bool_value;

        struct {
            const char **values;
            size_t count;
        } array_string_value;

        struct {
            int *values;
            size_t count;
        } array_int_value;

        struct {
            float *values;
            size_t count;
        } array_float_value;
    };
} CommandArgument;

typedef struct SubCommand {
    const char *name;
    const char *description;
    execute_fn execute;
    uint32 argument_count;
    const CommandArgument *arguments;
} SubCommand;

typedef enum CliStatus {
    CLI_OK = 0,
    CLI_EUSAGE,
    CLI_EUNKNOWN_CMD,
    CLI_EUNKNOWN_OPT,
    CLI_EMISSING,
    CLI_EBADVALUE,
    CLI_ENOMEM
} CliStatus;

typedef struct CliSpec {
    const char *prog;              /* e.g. "ApexPredator.exe" (optional; fallback argv[0]) */
    const char *root_name;         /* e.g. "game path" (help text only) */
    const char *root_desc;         /* description in help (optional) */

    const SubCommand *commands;
    size_t command_count;
} CliSpec;

typedef struct CliResult {
    const char *exe_path;
    const char *game_root;
    const SubCommand *cmd;

    uint32_t arg_count;
    CommandArgument *args;
} CliResult;


void cli_free(CliResult *r);

CliStatus cli_parse(const CliSpec *spec, CliResult *out, int argc, const char *argv[], const char **out_err_tok);

bool cli_get_cstring(const CliResult *r, const char *name, const char **out);

bool cli_get_string(const CliResult *r, const char *name, String* out);

bool cli_get_int(const CliResult *r, const char *name, int *out);

bool cli_get_float(const CliResult *r, const char *name, float *out);

bool cli_get_bool(const CliResult *r, const char *name, bool *out);

bool cli_get_array_string(const CliResult *r, const char *name, const char ***out, size_t *count);

bool cli_get_array_int(const CliResult *r, const char *name, const int **out, size_t *count);

bool cli_get_array_float(const CliResult *r, const char *name, const float **out, size_t *count);

// void cli_print_help(int argc, const char *argv[]) {
//     const bool is_posix_path = strchr(argv[0], '/') != NULL;
//     const uint32 last_slash = is_posix_path
//                                   ? (uint32) (strrchr(argv[0], '/') - argv[0] + 1)
//                                   : (uint32) (strrchr(argv[0], '\\') - argv[0] + 1);
//     const char *exe_name = argv[0] + last_slash;
//
//     printf("%s <game path> <command> [options]\n\n", exe_name);
//     printf("Available commands:\n");
//     for (size_t i = 0; i < sizeof(sub_commands) / sizeof(SubCommand); ++i) {
//         const SubCommand *sub_command = &sub_commands[i];
//         printf("  %s: %s\n", sub_command->name, sub_command->description);
//         printf("    Arguments:\n");
//         for (int arg_id = 0; arg_id < sub_command->argument_count; ++arg_id) {
//             const CommandArgument *arg = &sub_command->arguments[arg_id];
//             printf("      ");
//             if (!arg->required) {
//                 printf("[");
//             }
//             if (arg->named) {
//                 printf("--%s", arg->name);
//             }
//             else {
//                 printf("%s", arg->name);
//             }
//             if (!arg->required) {
//                 printf("]");
//             }
//             printf(": %s.", command_argument_type_names[arg->type]);
//             if (arg->has_default) {
//                 printf(" (default: ");
//                 switch (arg->type) {
//                     case COMMAND_ARG_TYPE_STRING:
//                         printf("%s", arg->string_value);
//                         break;
//                     case COMMAND_ARG_TYPE_INT:
//                         printf("%d", arg->int_value);
//                         break;
//                     case COMMAND_ARG_TYPE_FLOAT:
//                         printf("%f", arg->float_value);
//                         break;
//                     case COMMAND_ARG_TYPE_BOOL:
//                         printf("%s", arg->bool_value ? "true" : "false");
//                         break;
//                     case COMMAND_ARG_TYPE_ARRAY_STRING:
//                         for (size_t j = 0; j < arg->array_string_value.count; ++j) {
//                             if (j > 0) printf(", ");
//                             printf("%s", arg->array_string_value.values[j]);
//                         }
//                         break;
//                     case COMMAND_ARG_TYPE_ARRAY_INT:
//                         for (size_t j = 0; j < arg->array_int_value.count; ++j) {
//                             if (j > 0) printf(", ");
//                             printf("%d", arg->array_int_value.values[j]);
//                         }
//                         break;
//                     case COMMAND_ARG_TYPE_ARRAY_FLOAT:
//                         for (size_t j = 0; j < arg->array_float_value.count; ++j) {
//                             if (j > 0) printf(", ");
//                             printf("%f", arg->array_float_value.values[j]);
//                         }
//                         break;
//                 }
//                 printf(")");
//             }
//             printf("\n");
//             printf("      %s\n", arg->description);
//
//             printf("\n");
//         }
//     }
//     printf("\n");
// }


static void cli_print_cmd_usage(const CliSpec *spec, const SubCommand *cmd, FILE *outf);

void cli_print_help_cmd(const CliSpec *spec, const char *cmd_name, FILE *outf);

void cli_print_help(const CliSpec *spec, const char *exe_path, FILE *out);

#endif //APEXPREDATOR_CLI_PARSER_H
