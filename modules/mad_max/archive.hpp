#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "apex/package/tab_v1.hpp"
#include "redscore/platform/file/file.h"

namespace MadMax {
    // Empty reason means at least one supported TAB/ARC pair is present.
    std::string probe_archive_directory(const std::filesystem::path &folder);

    class Archives {
        std::vector<std::unique_ptr<TabV1::TabV1>> archives;
        void mount(const std::filesystem::path &folder);

    public:
        explicit Archives(const std::filesystem::path &data);
        std::unique_ptr<IO::File> get(uint32_t hash);
    };
}
