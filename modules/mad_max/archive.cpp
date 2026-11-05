#include "archive.hpp"

#include <algorithm>
#include <charconv>
#include <stdexcept>
#include <utility>

namespace MadMax {
    namespace fs = std::filesystem;

    namespace {
        uint32_t archive_number(const fs::path &path) {
            const auto stem = path.stem().string();
            uint32_t number{};
            if (!stem.starts_with("game")) throw std::runtime_error("Unexpected Mad Max archive name: " + path.string());
            const auto parsed = std::from_chars(stem.data() + 4, stem.data() + stem.size(), number);
            if (parsed.ec != std::errc{} || parsed.ptr != stem.data() + stem.size())
                throw std::runtime_error("Unexpected Mad Max archive number: " + path.string());
            return number;
        }
    }

    std::string probe_archive_directory(const fs::path &folder) {
        if (!fs::is_directory(folder)) return "Missing archives_win64 directory";
        bool found = false;
        for (const auto &file : fs::directory_iterator(folder)) {
            if (!file.is_regular_file() || file.path().extension() != ".tab") continue;
            if (!TabV1::TabV1::plausible_header(file.path())) return "Unsupported or incomplete TAB v1 archive";
            found = true;
        }
        return found ? "" : "No TAB/ARC archives found";
    }

    void Archives::mount(const fs::path &folder) {
        if (!fs::is_directory(folder)) return;
        std::vector<std::pair<uint32_t, fs::path>> tabs;
        for (const auto &file : fs::directory_iterator(folder))
            if (file.is_regular_file() && file.path().extension() == ".tab")
                tabs.emplace_back(archive_number(file.path()), file.path());
        // The game searches higher-numbered gameN archives first within a mount.
        std::sort(tabs.begin(), tabs.end(), [](const auto &a, const auto &b) { return a.first > b.first; });
        for (const auto &[number, path] : tabs) {
            IO::NativeFile tab(path, std::ios::in | std::ios::binary);
            archives.push_back(std::make_unique<TabV1::TabV1>(tab, path));
        }
    }

    Archives::Archives(const fs::path &data) {
        // Native VFS mount priorities: Feral 320, patch 1000, base 3000; lower wins.
        mount(data / "feral_overrides");
        mount(data / "patch_win64");
        const auto dlc = data / "dlc_win64";
        if (fs::is_directory(dlc)) {
            std::vector<fs::path> folders;
            for (const auto &entry : fs::directory_iterator(dlc))
                if (entry.is_directory()) folders.push_back(entry.path());
            std::sort(folders.begin(), folders.end());
            // Installed DLC hashes do not collide; define deterministic fallback for copied archives.
            for (const auto &folder : folders) mount(folder);
        }
        mount(data / "archives_win64");
    }

    std::unique_ptr<IO::File> Archives::get(uint32_t hash) {
        for (const auto &archive : archives)
            if (auto asset = archive->get(uint64_t(hash))) return asset;
        throw std::runtime_error("Asset not found: " + std::to_string(hash));
    }
}
