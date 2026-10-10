#pragma once

#include "apex/package/tab.hpp"

namespace TabJC2 {
    struct TabHeader {
        uint32 alignment;
    };

    struct TabEntry {
        uint32 hash;
        uint32 offset;
        uint32 size;
    };

    static_assert(sizeof(TabHeader) == 4);
    static_assert(sizeof(TabEntry) == 12);

    class TabJC2 final : public Tab {
    public:
        TabJC2(IO::NativeFile &tab, const std::filesystem::path &path);
        bool has(std::string_view path) override;
        bool has(const u64 &hash) override;
        std::unique_ptr<IO::File> get(std::string_view path) override;
        std::unique_ptr<IO::File> get(const u64 &hash) override;
        const u64 &key() const override;
        bool foreach_file(const std::function<bool(const Archive<u64>::ArchiveEntry &)> &callback) override;

    private:
        struct Entry {
            uint32 offset;
            uint32 size;
        };
        IO::NativeFile arc_buffer;
        std::unordered_map<uint32, Entry> entries;
    };
}
