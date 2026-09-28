#include "../commands.h"

void SearchCommand::handle() {
    auto library = select_module();
    const auto results = library->api().search(Modules::normalize_path(m_db_path), m_search_query);
    std::cout << "Search results (" << results.size() << " found) for query \"" << m_search_query << "\":\n";
    for (const auto &entry : results) std::cout << "  " << entry << '\n';
}
