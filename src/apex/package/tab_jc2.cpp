#include "apex/package/tab_jc2.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

#include "redscore/platform/file/memory_file.h"
#include "utils/lookup3.h"
#include "zlib-ng.h"

namespace TabJC2 {
    TabJC2::TabJC2(IO::NativeFile &tab, const std::filesystem::path &path)
        : arc_buffer(get_arc_path(path), std::ios::in | std::ios::binary) {
        if (!arc_buffer.stream().is_open()) throw std::runtime_error("Missing ARC for " + path.string());
        if (tab.get_size() < sizeof(TabHeader) + sizeof(TabEntry) ||
            (tab.get_size() - sizeof(TabHeader)) % sizeof(TabEntry))
            throw std::runtime_error("Invalid Just Cause 2 TAB size: " + path.string());
        if (tab.read_pod<TabHeader>().alignment != 2048)
            throw std::runtime_error("Unsupported Just Cause 2 TAB alignment: " + path.string());
        m_name = std::filesystem::relative(path, path.parent_path().parent_path()).generic_string();
        m_hash = hashlittle(m_name.data(), m_name.size(), 0);
        const auto arc_size = arc_buffer.get_size();
        entries.reserve((tab.get_size() - sizeof(TabHeader)) / sizeof(TabEntry));
        while (tab.remaining()) {
            const auto [hash, offset, size] = tab.read_pod<TabEntry>();
            if (offset % 2048 || !size || uint64_t(offset) + size > arc_size)
                throw std::runtime_error("Invalid TAB entry in " + path.string());
            entries[hash] = Entry{offset, size};
        }
    }

    bool TabJC2::has(std::string_view path) { return has(hashlittle(path.data(), path.size(), 0)); }
    bool TabJC2::has(const u64 &hash) { return hash <= UINT32_MAX && entries.contains(static_cast<uint32>(hash)); }

    std::unique_ptr<IO::File> TabJC2::get(std::string_view path) {
        return get(hashlittle(path.data(), path.size(), 0));
    }

    std::unique_ptr<IO::File> TabJC2::get(const u64 &hash) {
        if (hash > UINT32_MAX) return nullptr;
        const auto it = entries.find(static_cast<uint32>(hash));
        if (it == entries.end()) return nullptr;
        const auto &entry = it->second;
        arc_buffer.set_position(entry.offset, std::ios::beg);
        std::array<unsigned char, 2> prefix{};
        arc_buffer.read_exact(std::span(prefix.data(), prefix.size()));
        const unsigned cmf = prefix[0], flg = prefix[1];
        const bool compressed = (cmf & 15) == 8 && (cmf >> 4) <= 7 && ((cmf << 8 | flg) % 31 == 0);
        arc_buffer.set_position(entry.offset, std::ios::beg);
        if (!compressed)
            return std::make_unique<IO::MemoryFile>(arc_buffer.read_exact<uint8>(entry.size));

        std::vector<uint8> output;
        std::array<uint8, 65536> input{}, decoded{};
        zng_stream stream{};
        if (zng_inflateInit(&stream) != Z_OK) throw std::runtime_error("Cannot initialize zlib decoder");
        uint32 remaining = entry.size;
        try {
            int result = Z_OK;
            while (result != Z_STREAM_END) {
                if (!stream.avail_in && remaining) {
                    const auto n = std::min<size_t>(remaining, input.size());
                    arc_buffer.read_exact(std::span(input.data(), n));
                    remaining -= static_cast<uint32>(n);
                    stream.next_in = input.data();
                    stream.avail_in = static_cast<unsigned>(n);
                }
                stream.next_out = decoded.data();
                stream.avail_out = decoded.size();
                result = zng_inflate(&stream, Z_NO_FLUSH);
                if (result != Z_OK && result != Z_STREAM_END)
                    throw std::runtime_error("Invalid compressed ARC entry");
                output.insert(output.end(), decoded.begin(), decoded.begin() + (decoded.size() - stream.avail_out));
                if (result != Z_STREAM_END && !remaining && !stream.avail_in)
                    throw std::runtime_error("Truncated compressed ARC entry");
            }
            if (remaining || stream.avail_in) throw std::runtime_error("Trailing data in compressed ARC entry");
        } catch (...) {
            zng_inflateEnd(&stream);
            throw;
        }
        zng_inflateEnd(&stream);
        return std::make_unique<IO::MemoryFile>(std::move(output));
    }

    const u64 &TabJC2::key() const { return m_hash; }

    bool TabJC2::foreach_file(const std::function<bool(const Archive<u64>::ArchiveEntry &)> &callback) {
        const auto total = entries.size();
        uint64 i = 0;
        for (const auto &[hash, entry]: entries)
            if (!callback({hash, 0, entry.size, total, i++})) return false;
        return true;
    }
}
