#pragma once

#include "apex/package/tab.hpp"

#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace TabV1 {
    // Legacy compressed TAB: 0x800, block-group count, groups, then 16-byte entries.
    class TabV1 final : public Tab {
        struct Entry { uint32_t hash, offset, packed, unpacked; };
        struct BlockStart { uint32_t unpacked, packed; };
        struct Asset { Entry entry; std::vector<BlockStart> starts; };

        std::filesystem::path arc_path;
        std::unordered_map<uint32_t, Asset> entries;

    public:
        TabV1(IO::NativeFile &tab, const std::filesystem::path &path);
        static bool plausible_header(const std::filesystem::path &path);
        bool has(std::string_view path) override;
        bool has(const u64 &hash) override;
        std::unique_ptr<IO::File> get(std::string_view path) override;
        std::unique_ptr<IO::File> get(const u64 &hash) override;
        const u64 &key() const override { return m_hash; }
        bool foreach_file(const std::function<bool(const Archive<u64>::ArchiveEntry &)> &callback) override;
    };
}
