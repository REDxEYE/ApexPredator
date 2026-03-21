// Created by RED on 16.02.2026.
#include "apex/hashes.h"
#include "../commands.h"
#include "platform/app_state.h"
#include "../../../../include/apex/asset_db.h"

void SearchCommand::handle() {
    AssetDB db(m_db_path);
    AssetDB::set_instance(&db);
    std::vector<std::string> result;
    search_file_table(m_search_query,result);
    if (result.empty()) {
        printf("No results found for query \"%s\".\n", m_search_query.c_str());
    } else {
        printf("Search results(%zu found) for query \"%s\":\n", result.size(), m_search_query.c_str());
        for (const auto &entry : result) {
            printf("  %s\n", entry.c_str());
        }
    }
    AssetDB::set_instance(nullptr);
}

