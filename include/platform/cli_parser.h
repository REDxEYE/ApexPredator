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

typedef struct CliResult CliResult;

typedef void (*execute_fn)(void* user_state, const CliResult* result);

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

    const SubCommand *commands;
    size_t command_count;
} CliSpec;

typedef struct CliResult {
    const char *exe_path;
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


static void cli_print_cmd_usage(const CliSpec *spec, const SubCommand *cmd, FILE *outf);

void cli_print_help_cmd(const CliSpec *spec, const char *cmd_name, FILE *outf);

void cli_print_help(const CliSpec *spec, const char *exe_path, FILE *out);

#endif //APEXPREDATOR_CLI_PARSER_H
