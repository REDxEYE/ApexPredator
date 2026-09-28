#include "modules/module_loader.h"
#include <iostream>

int main(int argc, char **argv) {
    try {
        if (argc != 3) throw std::runtime_error("Expected module library and fixture root");
        const std::filesystem::path root(argv[2]);
        auto library = std::make_shared<Modules::Library>(argv[1]);
        std::weak_ptr<Modules::Library> lifetime = library;
        auto first = std::make_unique<Modules::Session>(library, root, root / "session1.db", root / "session1", false);
        auto second = std::make_unique<Modules::Session>(library, root, root / "session2.db", root / "session2", false);
        library.reset();
        if (lifetime.expired()) throw std::runtime_error("Library unloaded while sessions are alive");
        first->extract("0x12340001", true);
        first.reset();
        if (lifetime.expired()) throw std::runtime_error("Library unloaded before last session");
        second->extract("0x12340001", true);
        second.reset();
        if (!lifetime.expired()) throw std::runtime_error("Library retained after last session");
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
