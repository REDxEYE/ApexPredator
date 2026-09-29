#include "tab_v31.hpp"

#include <climits>
#include <limits>
#include "ooz.h"
#include "utils/zlib_wrapper.h"

namespace TabV31 {
    TabV31::TabV31(IO::NativeFile &tab_buffer, const std::filesystem::path &path)
        : arc_buffer(get_arc_path(path), std::ios::in | std::ios::binary) {
        if (!tab_buffer.stream().is_open() || !arc_buffer.stream().is_open())
            throw std::runtime_error("Failed to open TAB/ARC pair: " + path.string());
        m_name = std::filesystem::relative(path, path.parent_path().parent_path()).generic_string();
        m_hash = asset_hash(m_name);
        m_arc_size = arc_buffer.get_size();
        const auto header = tab_buffer.read_pod<TabHeader>();
        if (std::memcmp(header.dwMagic, "TAB\0", 4) || header.wMajorVersion != 3 || header.wMinorVersion != 1)
            throw std::runtime_error("Invalid TAB 3.1 header");
        if (!header.alignment || (header.alignment & (header.alignment - 1)))
            throw std::runtime_error("Invalid TAB 3.1 alignment");
        const uint64_t tables_size = uint64_t(header.block_count) * sizeof(TabBlockEntry) +
                                     uint64_t(header.file_count) * sizeof(TabEntry);
        if (tables_size != tab_buffer.remaining())
            throw std::runtime_error("Invalid TAB 3.1 table sizes");
        m_blocks = tab_buffer.read_exact<TabBlockEntry>(header.block_count);
        m_entries.reserve(header.file_count);
        for (uint32_t i = 0; i < header.file_count; ++i) {
            const auto entry = tab_buffer.read_pod<TabEntry>();
            if (uint64_t(entry.offset) + entry.compressed_size > m_arc_size)
                throw std::runtime_error("TAB 3.1 entry exceeds ARC bounds");
            if (entry.compression_type != CompressionType::Raw && entry.compression_type != CompressionType::Zlib &&
                entry.compression_type != CompressionType::Oodle)
                throw std::runtime_error("Unsupported TAB 3.1 compression type");
            if (entry.block_index != 0) {
                uint64_t compressed = 0, uncompressed = 0;
                size_t index = entry.block_index;
                while (compressed < entry.compressed_size) {
                    if (index >= m_blocks.size()) throw std::runtime_error("TAB 3.1 block index out of range");
                    const auto &block = m_blocks[index++];
                    if (!block.compressed_size || !block.uncompressed_size ||
                        block.compressed_size == UINT32_MAX || block.uncompressed_size == UINT32_MAX)
                        throw std::runtime_error("Invalid TAB 3.1 data block (empty or sentinel)");
                    compressed += block.compressed_size;
                    uncompressed += block.uncompressed_size;
                }
                if (compressed != entry.compressed_size || uncompressed != entry.uncompressed_size)
                    throw std::runtime_error("TAB 3.1 block sizes do not match file sizes");
            }
            if (!m_entries.emplace(entry.hash, entry).second)
                throw std::runtime_error("Duplicate TAB 3.1 asset hash");
        }
    }

    bool TabV31::has(std::string_view path) { return has(asset_hash(path)); }
    bool TabV31::has(const u64 &hash) { return m_entries.contains(hash); }
    std::unique_ptr<IO::File> TabV31::get(std::string_view path) { return get(asset_hash(path)); }

    std::unique_ptr<IO::File> TabV31::get(const u64 &hash) {
        const auto it = m_entries.find(hash);
        if (it == m_entries.end()) return nullptr;
        const auto &entry = it->second;
        arc_buffer.set_position(entry.offset, std::ios::beg);
        // The decoder's SIMD copies require writable padding beyond each output.
        std::vector<uint8> output(size_t(entry.uncompressed_size) + OOZ_SAFE_SPACE);
        size_t output_offset = 0;
        auto decode = [&](uint32_t compressed_size, uint32_t uncompressed_size) {
            if (uncompressed_size > output.size() - OOZ_SAFE_SPACE - output_offset)
                throw std::runtime_error("TAB 3.1 decoded block exceeds file size");
            auto *destination = output.data() + output_offset;
            if (compressed_size == uncompressed_size) {
                arc_buffer.read_exact(std::span<uint8>(destination, uncompressed_size));
            } else {
                std::vector<uint8> compressed(size_t(compressed_size) + OOZ_SAFE_SPACE);
                arc_buffer.read_exact(std::span<uint8>(compressed.data(), compressed_size));
                if (entry.compression_type == CompressionType::Zlib) {
                    size_t consumed = 0;
                    if (inflate_exact_into(compressed.data(), compressed_size, destination, uncompressed_size,
                                           MAX_WBITS, nullptr, &consumed) != Z_OK || consumed != compressed_size)
                        throw std::runtime_error("TAB 3.1 zlib decompression failed");
                } else if (entry.compression_type == CompressionType::Oodle) {
                    if (uncompressed_size > INT_MAX ||
                        Kraken_Decompress(compressed.data(), compressed_size, destination, uncompressed_size) !=
                        static_cast<int>(uncompressed_size))
                        throw std::runtime_error("TAB 3.1 Oodle decompression failed");
                } else {
                    throw std::runtime_error("TAB 3.1 raw block has inconsistent sizes");
                }
            }
            output_offset += uncompressed_size;
        };
        if (entry.block_index == 0) {
            decode(entry.compressed_size, entry.uncompressed_size);
        } else {
            // ARC entry offsets are aligned; blocks inside a file are contiguous,
            // independently compressed streams. Index 1 can be a real block.
            uint64_t remaining = entry.compressed_size;
            size_t index = entry.block_index;
            while (remaining) {
                const auto &block = m_blocks.at(index++);
                decode(block.compressed_size, block.uncompressed_size);
                remaining -= block.compressed_size;
            }
        }
        output.resize(entry.uncompressed_size);
        return std::make_unique<IO::MemoryFile>(std::move(output));
    }

    const u64 &TabV31::key() const { return m_hash; }

    bool TabV31::foreach_file(const std::function<bool(const Archive<u64>::ArchiveEntry &)> &callback) {
        for (const auto &[hash, entry] : m_entries)
            if (!callback({hash, 0, entry.uncompressed_size})) return false;
        return true;
    }
}
