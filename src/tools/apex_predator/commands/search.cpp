// Created by RED on 16.02.2026.
#include "apex/hashes.h"
#include "platform/app_state.h"
#include "platform/cli_parser.h"

void search_handler(const AppState *app_state, const CliResult *cli_res) {
    (void)app_state;

    const char *query_cstr = NULL;
    cli_get_cstring(cli_res, "query", &query_cstr);
    char **results = NULL;
    uint32 count = 0;
    search_file_table(query_cstr, &results, &count);
    if (count > 0 && results != NULL) {
        printf("Search results(%u found) for query \"%s\":\n", count, query_cstr);
        for (int i = 0; i < count; ++i) {
            printf("  %s\n", results[i]);
            mp_free(results[i]);
        }
        mp_free(results);
    }
}
