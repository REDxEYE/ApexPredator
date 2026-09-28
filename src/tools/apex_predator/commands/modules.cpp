#include "../commands.h"

void ModulesCommand::handle() {
    Modules::Registry registry(m_module_directories);
    for (const auto &diagnostic : registry.diagnostics()) std::cerr << "Module warning: " << diagnostic << '\n';
    auto libraries = registry.libraries();
    if (!m_module.empty()) libraries = {registry.select(m_module)};
    if (libraries.empty()) throw std::runtime_error("No compatible game modules found");
    for (const auto &library : libraries) {
        std::cout << library->api().id() << " - " << library->api().name() << " (" << Modules::utf8(library->path()) << ")";
        if (!m_root.empty()) {
            std::string reason; auto score = library->probe(m_root, reason);
            std::cout << "\n  " << (score > 0 ? "supported" : score == 0 ? "unsupported" : "probe error") << " [" << score << "]: " << reason;
        }
        std::cout << '\n';
    }
}
