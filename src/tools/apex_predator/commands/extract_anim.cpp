#include "../commands.h"

void ExtractAnimationCommand::handle() {
    Modules::Session session(select_module(m_game_root), m_game_root, m_db_path, m_export_path, true);
    for (const auto &asset : m_animations) session.animation(m_skeleton_path, asset, m_apply_root_motion);
}
