//
// Created by red_eye on 9/24/26.
//

#pragma once
#include <cstring>

#include "apex/package/tab.hpp"
#include "utils/murmur3.h"

namespace TabV31 {
#pragma pack(push, 1)
    struct TabHeader {
        char dwMagic[4]; // 0x424154 (TAB\0)
        int16 wMajorVersion; // 3
        int16 wMinorVersion; // 1
        uint32_t alignment; // 0x08  = 0x1000
        uint32_t file_count; // 0x0C number of TabFileEntry
        uint32_t block_count; // 0x10 number of TabBlockEntry
        uint32_t padding; // 0x14 always 0
        uint32_t max_compressed_block_size; // 0x18
        uint32_t uncompressed_block_size; // 0x1C
    };

    enum class CompressionType : uint8_t {
        Raw = 0, // file data written verbatim inside .arc
        Zlib = 1, // zlib stream (rare in RAGE 2, used for a handful of small files)
        Oodle = 4, // Oodle v7 (Kraken by default), the vast majority of assets
        // 2 and 3 reserved / unused in TAB 3.1
    };

    struct TabBlockEntry {
        uint32_t compressed_size; // 0x00 csize of the block
        uint32_t uncompressed_size; // 0x04 usize of the block
    };

    struct TabEntry {
        uint64_t hash; // 0x00 MurmurHash3_x64_128(path, seed=0).h1
        uint32_t offset; // 0x08 byte offset of the data inside the .arc
        uint32_t compressed_size; // 0x0C csize of this file
        uint32_t uncompressed_size; // 0x10 usize of this file
        uint16_t block_index; // 0x14 first block; 0 = sizes are stored directly in the file entry
        CompressionType compression_type; // 0x16  0 = raw, 1 = zlib, 4 = Oodle v7
        uint8_t compression_flags; // 0x17 observed as 0 or 1; preserved, not used to select compression
    };

#pragma pack(pop)
    static_assert(sizeof(TabHeader) == 32);
    static_assert(sizeof(TabBlockEntry) == 8);
    static_assert(sizeof(TabEntry) == 24);

    inline uint64_t asset_hash(std::string_view name) {
        return MurmurHash3_x64_128(name.data(), static_cast<int>(name.size()), 0);
    }

    class TabV31 : public Tab {
    public:
        TabV31(IO::NativeFile &tab_buffer, const std::filesystem::path &path);

        bool has(std::string_view path) override;

        bool has(const u64 &hash) override;

        std::unique_ptr<IO::File> get(std::string_view path) override;

        std::unique_ptr<IO::File> get(const u64 &hash) override;

        const u64 & key() const override;

        bool foreach_file(const std::function<bool(const Archive<u64>::ArchiveEntry &)> &callback) override;

    private:
        IO::NativeFile arc_buffer;
        uint64_t m_arc_size{};
        std::vector<TabBlockEntry> m_blocks{};
        std::unordered_map<uint64, TabEntry> m_entries{};
    };
}
