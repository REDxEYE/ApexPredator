//
// Created by red_eye on 9/24/26.
//

#pragma once
#include "apex/package/tab.hpp"

namespace TabV21 {
#pragma pack(push, 1)
    struct TabHeader {
        char dwMagic[4]; // 0x424154 (TAB\0)
        int16 wMajorVersion; // 2
        int16 wMinorVersion; // 1
        int32 dwAligment; // 2048
    };

    struct TabEntry {
        uint32 hash;
        uint32 offset;
        uint32 size;
    };

#pragma pack(pop)

    class TabV21 : public Tab {
    public:
        TabV21(IO::NativeFile &tab_buffer, const std::filesystem::path &path) : arc_buffer(
            get_arc_path(path), std::ios::in | std::ios::binary) {

            const auto base = path.parent_path().parent_path();
            const auto relative_path = std::filesystem::relative(path, base);
            m_name = relative_path.string();
            m_hash = asset_path_hash(m_name);

            if (!tab_buffer.stream().is_open()) {
                throw std::runtime_error("Failed to open tab archive file: " + path.string());
            }
            const auto header = tab_buffer.read_pod<TabHeader>();

            if (memcmp(header.dwMagic, "TAB\0", 4) != 0) {
                throw std::runtime_error("Invalid TAB archive magic");
            }

            if (header.wMajorVersion != 2 || header.wMinorVersion != 1) {
                throw std::runtime_error("Unsupported TAB archive version");
            }

            const uint32 entry_count = tab_buffer.remaining() / sizeof(TabEntry);
            m_entries.reserve(entry_count);
            for (uint32 i = 0; i < entry_count; ++i) {
                const auto entry = tab_buffer.read_pod<TabEntry>();
                m_entries[entry.hash] = entry;
            }
        }

        bool has(std::string_view path) override;

        bool has(const u64 &hash) override;

        std::unique_ptr<IO::File> get(std::string_view path) override;

        std::unique_ptr<IO::File> get(const u64 &hash) override;

        const u64 & key() const override;

        bool foreach_file(const std::function<bool(const Archive<u64>::ArchiveEntry &)> &callback) override;

    private:
        IO::NativeFile arc_buffer;
        std::unordered_map<uint64, TabEntry> m_entries{};
    };
}
