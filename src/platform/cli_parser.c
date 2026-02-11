// Created by RED on 31.01.2026.

#include "platform/cli_parser.h"

#include <errno.h>

#include "utils/path.h"


const char *command_argument_type_names[] = {
    "string",
    "int",
    "float",
    "bool",
    "array of string",
    "array of ints",
    "array of floats"
};

static const char *cli_type_name(CommandArgumentType t) {
    return command_argument_type_names[t];
}

static bool cli_is_opt(const char *s) {
    return s && s[0] == '-' && s[1] != '\0';
}

static bool cli_is_array_type(CommandArgumentType t) {
    return t == COMMAND_ARG_TYPE_ARRAY_STRING ||
           t == COMMAND_ARG_TYPE_ARRAY_INT ||
           t == COMMAND_ARG_TYPE_ARRAY_FLOAT;
}

static bool cli_parse_int(const char *s, int *out) {
    if (!s || !*s) return false;
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return false;
    if (v < (long) INT32_MIN || v > (long) INT32_MAX) return false;
    *out = (int) v;
    return true;
}

static bool cli_parse_float(const char *s, float *out) {
    if (!s || !*s) return false;
    errno = 0;
    char *end = NULL;
    float v = strtof(s, &end);
    if (errno != 0 || end == s || *end != '\0') return false;
    *out = v;
    return true;
}

static bool cli_parse_bool(const char *s, int *out) {
    if (!s || !*s) return false;
    if (!strcmp(s, "1") || !strcmp(s, "true") || !strcmp(s, "TRUE") || !strcmp(s, "yes") || !strcmp(s, "YES")) {
        *out = 1;
        return true;
    }
    if (!strcmp(s, "0") || !strcmp(s, "false") || !strcmp(s, "FALSE") || !strcmp(s, "no") || !strcmp(s, "NO")) {
        *out = 0;
        return true;
    }
    return false;
}

static const SubCommand *cli_find_cmd(const CliSpec *spec, const char *name) {
    for (size_t i = 0; i < spec->command_count; ++i) {
        if (!strcmp(spec->commands[i].name, name))
            return &spec->commands[i];
    }
    return NULL;
}

static CommandArgument *cli_find_named_long(CliResult *r, const char *name) {
    for (uint32_t i = 0; i < r->arg_count; ++i) {
        CommandArgument *a = &r->args[i];
        if (a->named && a->name && !strcmp(a->name, name))
            return a;
    }
    return NULL;
}

static CommandArgument *cli_find_named_short(CliResult *r, char f) {
    for (uint32_t i = 0; i < r->arg_count; ++i) {
        CommandArgument *a = &r->args[i];
        if (!a->named) continue;
        if (a->flag && a->flag[0] == f && a->flag[1] == '\0')
            return a;
    }
    return NULL;
}

static void cli_clear_array(CommandArgument *a) {
    if (!a || !cli_is_array_type(a->type)) return;

    if (a->type == COMMAND_ARG_TYPE_ARRAY_STRING) {
        free((void *) a->array_string_value.values);
        a->array_string_value.values = NULL;
        a->array_string_value.count = 0;
    }
    else if (a->type == COMMAND_ARG_TYPE_ARRAY_INT) {
        free(a->array_int_value.values);
        a->array_int_value.values = NULL;
        a->array_int_value.count = 0;
    }
    else if (a->type == COMMAND_ARG_TYPE_ARRAY_FLOAT) {
        free(a->array_float_value.values);
        a->array_float_value.values = NULL;
        a->array_float_value.count = 0;
    }
}

void cli_free(CliResult *r) {
    if (!r) return;
    if (r->args) {
        for (uint32_t i = 0; i < r->arg_count; ++i)
            cli_clear_array(&r->args[i]);
        free(r->args);
    }
    memset(r, 0, sizeof(*r));
}

static CliStatus cli_set_scalar(CommandArgument *a, const char *value) {
    switch (a->type) {
        case COMMAND_ARG_TYPE_STRING:
            if (!value) return CLI_EBADVALUE;
            a->string_value = value;
            return CLI_OK;
        case COMMAND_ARG_TYPE_INT: {
            int v;
            if (!value || !cli_parse_int(value, &v)) return CLI_EBADVALUE;
            a->int_value = v;
            return CLI_OK;
        }
        case COMMAND_ARG_TYPE_FLOAT: {
            float v;
            if (!value || !cli_parse_float(value, &v)) return CLI_EBADVALUE;
            a->float_value = v;
            return CLI_OK;
        }
        case COMMAND_ARG_TYPE_BOOL: {
            if (!value) {
                a->bool_value = 1;
                return CLI_OK;
            }
            int v;
            if (!cli_parse_bool(value, &v)) return CLI_EBADVALUE;
            a->bool_value = v;
            return CLI_OK;
        }
        default:
            return CLI_EBADVALUE;
    }
}

static CliStatus cli_append_array_one(CommandArgument *a, const char *value) {
    if (!a || !value) return CLI_EBADVALUE;

    if (a->type == COMMAND_ARG_TYPE_ARRAY_STRING) {
        size_t n = a->array_string_value.count;
        const char **tmp = (const char **) realloc((void *) a->array_string_value.values,
                                                   (n + 1) * sizeof(const char *));
        if (!tmp) return CLI_ENOMEM;
        tmp[n] = value;
        a->array_string_value.values = tmp;
        a->array_string_value.count = n + 1;
        return CLI_OK;
    }

    if (a->type == COMMAND_ARG_TYPE_ARRAY_INT) {
        int v;
        if (!cli_parse_int(value, &v)) return CLI_EBADVALUE;
        size_t n = a->array_int_value.count;
        int *tmp = (int *) realloc(a->array_int_value.values, (n + 1) * sizeof(int));
        if (!tmp) return CLI_ENOMEM;
        tmp[n] = v;
        a->array_int_value.values = tmp;
        a->array_int_value.count = n + 1;
        return CLI_OK;
    }

    if (a->type == COMMAND_ARG_TYPE_ARRAY_FLOAT) {
        float v;
        if (!cli_parse_float(value, &v)) return CLI_EBADVALUE;
        size_t n = a->array_float_value.count;
        float *tmp = (float *) realloc(a->array_float_value.values, (n + 1) * sizeof(float));
        if (!tmp) return CLI_ENOMEM;
        tmp[n] = v;
        a->array_float_value.values = tmp;
        a->array_float_value.count = n + 1;
        return CLI_OK;
    }

    return CLI_EBADVALUE;
}

static CliStatus cli_init_from_spec(CliResult *out, const SubCommand *cmd) {
    out->cmd = cmd;
    out->arg_count = cmd->argument_count;

    out->args = (CommandArgument *) calloc(out->arg_count, sizeof(CommandArgument));
    if (!out->args) return CLI_ENOMEM;

    for (uint32_t i = 0; i < out->arg_count; ++i) {
        out->args[i] = cmd->arguments[i];
        if (cli_is_array_type(out->args[i].type)) {
            if (out->args[i].type == COMMAND_ARG_TYPE_ARRAY_STRING) {
                out->args[i].array_string_value.values = NULL;
                out->args[i].array_string_value.count = 0;
            }
            else if (out->args[i].type == COMMAND_ARG_TYPE_ARRAY_INT) {
                out->args[i].array_int_value.values = NULL;
                out->args[i].array_int_value.count = 0;
            }
            else if (out->args[i].type == COMMAND_ARG_TYPE_ARRAY_FLOAT) {
                out->args[i].array_float_value.values = NULL;
                out->args[i].array_float_value.count = 0;
            }
        }
    }
    return CLI_OK;
}

static CliStatus cli_validate_positional_shape(const SubCommand *cmd) {
    bool seen_pos_array = false;
    for (uint32_t i = 0; i < cmd->argument_count; ++i) {
        const CommandArgument *a = &cmd->arguments[i];
        if (a->named) continue;

        if (cli_is_array_type(a->type)) {
            if (seen_pos_array) return CLI_EUSAGE;
            seen_pos_array = true;
            for (uint32_t j = i + 1; j < cmd->argument_count; ++j) {
                const CommandArgument *b = &cmd->arguments[j];
                if (!b->named) return CLI_EUSAGE;
            }
            break;
        }
    }
    return CLI_OK;
}

static CliStatus cli_consume_named(CliResult *r, int *io_i, int argc, const char *argv[], const char **out_err_tok) {
    const char *tok = argv[*io_i];

    if (!strcmp(tok, "--")) return CLI_EBADVALUE;

    if (tok[0] == '-' && tok[1] == '-') {
        const char *name = tok + 2;
        const char *eq = strchr(name, '=');
        const char *val = NULL;

        char buf[256];
        if (eq) {
            size_t n = (size_t) (eq - name);
            if (n == 0 || n >= sizeof(buf)) {
                if (out_err_tok) *out_err_tok = tok;
                return CLI_EUNKNOWN_OPT;
            }
            memcpy(buf, name, n);
            buf[n] = '\0';
            name = buf;
            val = eq + 1;
        }

        CommandArgument *a = cli_find_named_long(r, name);
        if (!a) {
            if (out_err_tok) *out_err_tok = tok;
            return CLI_EUNKNOWN_OPT;
        }

        if (cli_is_array_type(a->type)) {
            if (!val) {
                if (*io_i + 1 >= argc || cli_is_opt(argv[*io_i + 1])) {
                    if (out_err_tok) *out_err_tok = tok;
                    return CLI_EBADVALUE;
                }
                val = argv[++(*io_i)];
            }
            return cli_append_array_one(a, val);
        }

        if (a->type == COMMAND_ARG_TYPE_BOOL) {
            if (!val) {
                if (*io_i + 1 < argc && !cli_is_opt(argv[*io_i + 1])) {
                    int tmp;
                    if (cli_parse_bool(argv[*io_i + 1], &tmp))
                        val = argv[++(*io_i)];
                }
            }
            return cli_set_scalar(a, val);
        }

        if (!val) {
            if (*io_i + 1 >= argc || cli_is_opt(argv[*io_i + 1])) {
                if (out_err_tok) *out_err_tok = tok;
                return CLI_EBADVALUE;
            }
            val = argv[++(*io_i)];
        }
        return cli_set_scalar(a, val);
    }

    if (tok[0] == '-' && tok[1] != '\0' && tok[2] == '\0') {
        CommandArgument *a = cli_find_named_short(r, tok[1]);
        if (!a) {
            if (out_err_tok) *out_err_tok = tok;
            return CLI_EUNKNOWN_OPT;
        }

        if (cli_is_array_type(a->type)) {
            if (*io_i + 1 >= argc || cli_is_opt(argv[*io_i + 1])) {
                if (out_err_tok) *out_err_tok = tok;
                return CLI_EBADVALUE;
            }
            return cli_append_array_one(a, argv[++(*io_i)]);
        }

        if (a->type == COMMAND_ARG_TYPE_BOOL) {
            const char *val = NULL;
            if (*io_i + 1 < argc && !cli_is_opt(argv[*io_i + 1])) {
                int tmp;
                if (cli_parse_bool(argv[*io_i + 1], &tmp))
                    val = argv[++(*io_i)];
            }
            return cli_set_scalar(a, val);
        }

        if (*io_i + 1 >= argc || cli_is_opt(argv[*io_i + 1])) {
            if (out_err_tok) *out_err_tok = tok;
            return CLI_EBADVALUE;
        }
        return cli_set_scalar(a, argv[++(*io_i)]);
    }

    if (out_err_tok) *out_err_tok = tok;
    return CLI_EUNKNOWN_OPT;
}

static CliStatus cli_assign_positionals(CliResult *r, const char **pos, size_t pos_count, const char **out_err_tok) {
    size_t p = 0;

    for (uint32_t i = 0; i < r->arg_count; ++i) {
        CommandArgument *a = &r->args[i];
        if (a->named) continue;

        if (!cli_is_array_type(a->type)) {
            if (p >= pos_count) {
                if (out_err_tok) *out_err_tok = a->name;
                return CLI_EMISSING;
            }
            CliStatus st = cli_set_scalar(a, pos[p++]);
            if (st != CLI_OK) {
                if (out_err_tok) *out_err_tok = a->name;
                return st;
            }
            continue;
        }

        for (; p < pos_count; ++p) {
            CliStatus st = cli_append_array_one(a, pos[p]);
            if (st != CLI_OK) {
                if (out_err_tok) *out_err_tok = a->name;
                return st;
            }
        }
    }

    if (p != pos_count) {
        if (out_err_tok) *out_err_tok = pos[p];
        return CLI_EBADVALUE;
    }

    return CLI_OK;
}

static CliStatus cli_validate_required(const CliResult *r, const char **out_err_tok) {
    for (uint32_t i = 0; i < r->arg_count; ++i) {
        const CommandArgument *a = &r->args[i];
        if (!a->required) continue;

        if (a->type == COMMAND_ARG_TYPE_STRING) {
            if (!a->string_value) {
                if (out_err_tok) *out_err_tok = a->name;
                return CLI_EMISSING;
            }
        }
        else if (a->type == COMMAND_ARG_TYPE_ARRAY_STRING) {
            if (a->array_string_value.count == 0) {
                if (out_err_tok) *out_err_tok = a->name;
                return CLI_EMISSING;
            }
        }
        else if (a->type == COMMAND_ARG_TYPE_ARRAY_INT) {
            if (a->array_int_value.count == 0) {
                if (out_err_tok) *out_err_tok = a->name;
                return CLI_EMISSING;
            }
        }
        else if (a->type == COMMAND_ARG_TYPE_ARRAY_FLOAT) {
            if (a->array_float_value.count == 0) {
                if (out_err_tok) *out_err_tok = a->name;
                return CLI_EMISSING;
            }
        }
        else if (a->type == COMMAND_ARG_TYPE_BOOL) {
            /* bool required is always satisfied if default exists or explicitly set; we treat unset as false */
        }
        else {
            /* int/float required: cannot reliably detect “unset” without extra state; treat as satisfied */
        }
    }
    return CLI_OK;
}

CliStatus cli_parse(const CliSpec *spec, CliResult *out, int argc, const char *argv[], const char **out_err_tok) {
    if (out_err_tok) *out_err_tok = NULL;
    memset(out, 0, sizeof(*out));

    if (!spec || !spec->commands || spec->command_count == 0) return CLI_EUSAGE;
    if (argc < 3) {
        if (out_err_tok) *out_err_tok = "expected <command>";
        return CLI_EUSAGE;
    }

    out->exe_path = argv[0];

    const SubCommand *cmd = cli_find_cmd(spec, argv[1]);
    if (!cmd) {
        if (out_err_tok) *out_err_tok = argv[1];
        return CLI_EUNKNOWN_CMD;
    }

    CliStatus st = cli_validate_positional_shape(cmd);
    if (st != CLI_OK) {
        if (out_err_tok) *out_err_tok = cmd->name;
        return st;
    }

    st = cli_init_from_spec(out, cmd);
    if (st != CLI_OK) {
        if (out_err_tok) *out_err_tok = "oom";
        return st;
    }

    const char **pos = NULL;
    size_t pos_count = 0, pos_cap = 0;

    for (int i = 2; i < argc; ++i) {
        const char *tok = argv[i];

        if (!strcmp(tok, "--")) {
            for (int j = i + 1; j < argc; ++j) {
                if (pos_count == pos_cap) {
                    size_t nc = pos_cap ? pos_cap * 2 : 8;
                    const char **tmp = (const char **) realloc(pos, nc * sizeof(const char *));
                    if (!tmp) {
                        free(pos);
                        cli_free(out);
                        if (out_err_tok) *out_err_tok = "oom";
                        return CLI_ENOMEM;
                    }
                    pos = tmp;
                    pos_cap = nc;
                }
                pos[pos_count++] = argv[j];
            }
            break;
        }

        if (cli_is_opt(tok)) {
            st = cli_consume_named(out, &i, argc, argv, out_err_tok);
            if (st != CLI_OK) {
                free(pos);
                cli_free(out);
                return st;
            }
            continue;
        }

        if (pos_count == pos_cap) {
            size_t nc = pos_cap ? pos_cap * 2 : 8;
            const char **tmp = (const char **) realloc(pos, nc * sizeof(const char *));
            if (!tmp) {
                free(pos);
                cli_free(out);
                if (out_err_tok) *out_err_tok = "oom";
                return CLI_ENOMEM;
            }
            pos = tmp;
            pos_cap = nc;
        }
        pos[pos_count++] = tok;
    }

    st = cli_assign_positionals(out, pos, pos_count, out_err_tok);
    free(pos);
    if (st != CLI_OK) {
        cli_free(out);
        return st;
    }

    st = cli_validate_required(out, out_err_tok);
    if (st != CLI_OK) {
        cli_free(out);
        return st;
    }

    return CLI_OK;
}

bool cli_get_cstring(const CliResult *r, const char *name, const char **out) {
    for (uint32_t i = 0; i < r->arg_count; ++i) {
        const CommandArgument *a = &r->args[i];
        if (!strcmp(a->name, name) && a->type == COMMAND_ARG_TYPE_STRING) {
            if (out) *out = a->string_value;
            return true;
        }
    }
    return false;
}

bool cli_get_string(const CliResult *r, const char *name, String* out) {
    for (uint32_t i = 0; i < r->arg_count; ++i) {
        const CommandArgument *a = &r->args[i];
        if (!strcmp(a->name, name) && a->type == COMMAND_ARG_TYPE_STRING) {
            if (out) String_from_cstr(out, a->string_value ? a->string_value : "");
            return true;
        }
    }
    return false;
}

bool cli_get_int(const CliResult *r, const char *name, int *out) {
    for (uint32_t i = 0; i < r->arg_count; ++i) {
        const CommandArgument *a = &r->args[i];
        if (!strcmp(a->name, name) && a->type == COMMAND_ARG_TYPE_INT) {
            if (out) *out = a->int_value;
            return true;
        }
    }
    return false;
}

bool cli_get_float(const CliResult *r, const char *name, float *out) {
    for (uint32_t i = 0; i < r->arg_count; ++i) {
        const CommandArgument *a = &r->args[i];
        if (!strcmp(a->name, name) && a->type == COMMAND_ARG_TYPE_FLOAT) {
            if (out) *out = a->float_value;
            return true;
        }
    }
    return false;
}

bool cli_get_bool(const CliResult *r, const char *name, bool *out) {
    for (uint32_t i = 0; i < r->arg_count; ++i) {
        const CommandArgument *a = &r->args[i];
        if (!strcmp(a->name, name) && a->type == COMMAND_ARG_TYPE_BOOL) {
            if (out) *out = a->bool_value != 0;
            return true;
        }
    }
    return false;
}

bool cli_get_array_string(const CliResult *r, const char *name, const char ***out, size_t *count) {
    for (uint32_t i = 0; i < r->arg_count; ++i) {
        const CommandArgument *a = &r->args[i];
        if (!strcmp(a->name, name) &&
            a->type == COMMAND_ARG_TYPE_ARRAY_STRING) {
            if (out) *out = a->array_string_value.values;
            if (count) *count = a->array_string_value.count;
            return true;
        }
    }
    return false;
}

bool cli_get_array_int(const CliResult *r, const char *name, const int **out, size_t *count) {
    for (uint32_t i = 0; i < r->arg_count; ++i) {
        const CommandArgument *a = &r->args[i];
        if (!strcmp(a->name, name) &&
            a->type == COMMAND_ARG_TYPE_ARRAY_INT) {
            if (out) *out = a->array_int_value.values;
            if (count) *count = a->array_int_value.count;
            return true;
        }
    }
    return false;
}

bool cli_get_array_float(const CliResult *r, const char *name, const float **out, size_t *count) {
    for (uint32_t i = 0; i < r->arg_count; ++i) {
        const CommandArgument *a = &r->args[i];
        if (!strcmp(a->name, name) &&
            a->type == COMMAND_ARG_TYPE_ARRAY_FLOAT) {
            if (out) *out = a->array_float_value.values;
            if (count) *count = a->array_float_value.count;
            return true;
        }
    }
    return false;
}

static void cli_fprint_default(FILE *out, const CommandArgument *a) {
    if (!a->has_default) return;

    switch (a->type) {
        case COMMAND_ARG_TYPE_STRING:
            if (a->string_value) fprintf(out, " (default: %s)", a->string_value);
            break;
        case COMMAND_ARG_TYPE_INT:
            fprintf(out, " (default: %d)", a->int_value);
            break;
        case COMMAND_ARG_TYPE_FLOAT:
            fprintf(out, " (default: %g)", a->float_value);
            break;
        case COMMAND_ARG_TYPE_BOOL:
            fprintf(out, " (default: %s)", a->bool_value ? "true" : "false");
            break;
        default:
            break;
    }
}

static void cli_print_one_arg(FILE *out, const CommandArgument *a) {
    if (a->named) {
        /* named args in your sample are shown as [--name] */
        if (a->flag != NULL) {
            fprintf(out, "      [--%s or -%s]: %s, named.", a->name, a->flag, cli_type_name(a->type));
        }else {
            fprintf(out, "      [--%s]: %s, named.", a->name, cli_type_name(a->type));
        }

        cli_fprint_default(out, a);
        fprintf(out, "\n");
    }
    else {
        fprintf(out, "      %s: %s, positional.\n", a->name ? a->name : "", cli_type_name(a->type));
    }

    if (a->description && a->description[0]) {
        fprintf(out, "      %s\n", a->description);
    }
    fprintf(out, "\n");
}

void cli_print_help(const CliSpec *spec, const char *exe_path, FILE *out) {
    String exe_ = {0};
    Path_filename_sv(StringView_from_cstr(exe_path), &exe_);

    const char *prog = spec->prog ? spec->prog : String_cstr(&exe_);

    fprintf(out, "%s <command> [arguments]\n\n", prog);

    fprintf(out, "Available commands:\n");
    for (size_t ci = 0; ci < spec->command_count; ++ci) {
        const SubCommand *cmd = &spec->commands[ci];

        fprintf(out, "  %s: %s\n", cmd->name, cmd->description ? cmd->description : "");
        fprintf(out, "    Arguments:\n");

        for (uint32_t ai = 0; ai < cmd->argument_count; ++ai) {
            cli_print_one_arg(out, &cmd->arguments[ai]);
        }
    }
    String_free(&exe_);
}
