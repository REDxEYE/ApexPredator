#include "../commands.h"

void ExtractCommand::handle() {
    Modules::Session session(select_module(m_game_root), m_game_root, m_db_path, m_export_path, m_skip_textures);
    for (const auto &asset : m_assets) session.extract(asset, m_extract_raw);
}
