#include "apex/package/tab_v1.hpp"

#include <stdexcept>
#include <string_view>
#include <utility>

#include "redscore/platform/file/memory_file.h"
#include "utils/lookup3.h"
#include "utils/zlib_wrapper.h"

namespace TabV1 {
    namespace fs = std::filesystem;

    namespace {
        struct Header { uint32_t alignment, groups; };
        static_assert(sizeof(Header) == 8);

        uint32_t name_hash(std::string_view path) {
            const auto separator = path.find_last_of("/\\");
            const auto name = path.substr(separator == std::string_view::npos ? 0 : separator + 1);
            return hashlittle(name.data(), name.size(), 0);
        }
    }

    bool TabV1::plausible_header(const fs::path &path) {
        if (!fs::is_regular_file(path) || !fs::is_regular_file(get_arc_path(path)) ||
            fs::file_size(path) < sizeof(Header) + sizeof(Entry)) return false;
        IO::NativeFile tab(path, std::ios::in | std::ios::binary);
        if (!tab.stream().is_open()) return false;
        const auto header = tab.read_pod<Header>();
        return header.alignment == 2048 && header.groups <= tab.remaining() / 16;
    }

    TabV1::TabV1(IO::NativeFile &tab, const fs::path &path) : arc_path(get_arc_path(path)) {
        static_assert(sizeof(Entry) == 16 && sizeof(BlockStart) == 8);
        IO::NativeFile arc(arc_path, std::ios::in | std::ios::binary);
        if (!tab.stream().is_open() || !arc.stream().is_open())
            throw std::runtime_error("Missing TAB v1 ARC: " + path.string());
        if (tab.get_size() < sizeof(Header) + sizeof(Entry))
            throw std::runtime_error("Truncated TAB v1: " + path.string());
        tab.set_position(0, std::ios::beg);
        const auto header = tab.read_pod<Header>();
        if (header.alignment != 2048 || header.groups > tab.remaining() / 16)
            throw std::runtime_error("Invalid TAB v1 header: " + path.string());

        std::unordered_map<uint32_t, std::vector<BlockStart>> groups;
        for (uint32_t i = 0; i < header.groups; ++i) {
            const auto hash = tab.read_u32();
            const auto count = tab.read_u32();
            if (!count || count > tab.remaining() / sizeof(BlockStart))
                throw std::runtime_error("Invalid TAB v1 block group: " + path.string());
            auto [it, inserted] = groups.try_emplace(hash);
            if (!inserted) throw std::runtime_error("Duplicate TAB v1 block group");
            auto &starts = it->second;
            starts.reserve(count);
            for (uint32_t j = 0; j < count; ++j) {
                const auto start = tab.read_pod<BlockStart>();
                if (j == 0 && (start.packed || start.unpacked) ||
                    j && (start.packed <= starts.back().packed || start.unpacked <= starts.back().unpacked))
                    throw std::runtime_error("Invalid TAB v1 block offsets: " + path.string());
                starts.push_back(start);
            }
        }
        if (tab.remaining() % sizeof(Entry)) throw std::runtime_error("Invalid TAB v1 entry table: " + path.string());
        entries.reserve(tab.remaining() / sizeof(Entry));
        const auto arc_size = arc.get_size();
        while (tab.remaining()) {
            const auto entry = tab.read_pod<Entry>();
            if (entry.offset % header.alignment || uint64_t(entry.offset) + entry.packed > arc_size)
                throw std::runtime_error("TAB v1 entry outside ARC: " + path.string());
            auto group = groups.find(entry.hash);
            if (group != groups.end() &&
                (group->second.back().packed >= entry.packed || group->second.back().unpacked >= entry.unpacked))
                throw std::runtime_error("TAB v1 block exceeds asset: " + path.string());
            auto [it, inserted] = entries.try_emplace(entry.hash, Asset{entry, {}});
            if (!inserted) throw std::runtime_error("Duplicate TAB v1 asset hash: " + path.string());
            if (group != groups.end()) {
                it->second.starts = std::move(group->second);
                groups.erase(group);
            }
        }
        if (!groups.empty()) throw std::runtime_error("Orphan TAB v1 block group: " + path.string());
        m_name = fs::relative(path, path.parent_path().parent_path()).generic_string();
        m_hash = hashlittle(m_name.data(), m_name.size(), 0);
    }

    bool TabV1::has(std::string_view path) { return has(name_hash(path)); }
    bool TabV1::has(const u64 &hash) { return hash <= UINT32_MAX && entries.contains(static_cast<uint32_t>(hash)); }
    std::unique_ptr<IO::File> TabV1::get(std::string_view path) { return get(name_hash(path)); }

    std::unique_ptr<IO::File> TabV1::get(const u64 &hash) {
        if (hash > UINT32_MAX) return nullptr;
        const auto found = entries.find(static_cast<uint32_t>(hash));
        if (found == entries.end()) return nullptr;
        const auto &asset = found->second;
        IO::NativeFile arc(arc_path, std::ios::in | std::ios::binary);
        if (!arc.stream().is_open()) throw std::runtime_error("Cannot open TAB v1 ARC: " + arc_path.string());
        arc.set_position(asset.entry.offset, std::ios::beg);
        std::vector<uint8_t> output(asset.entry.unpacked);
        std::vector<uint8_t> compressed;
        auto segment = [&](uint32_t unpacked_at, uint32_t packed_size, uint32_t unpacked_size) {
            auto *destination = output.data() + unpacked_at;
            if (packed_size == unpacked_size) {
                arc.read_exact(std::span(destination, unpacked_size));
                return;
            }
            if (!packed_size || !unpacked_size) throw std::runtime_error("Empty TAB v1 compressed block");
            compressed.resize(packed_size);
            arc.read_exact(std::span(compressed.data(), packed_size));
            size_t consumed = 0;
            if (inflate_exact_raw(compressed.data(), packed_size, destination, unpacked_size, nullptr, &consumed) != Z_OK ||
                consumed != packed_size)
                throw std::runtime_error("Invalid TAB v1 raw-DEFLATE block");
        };
        uint32_t packed_at = 0, unpacked_at = 0;
        for (size_t i = 1; i < asset.starts.size(); ++i) {
            const auto &start = asset.starts[i];
            segment(unpacked_at, start.packed - packed_at, start.unpacked - unpacked_at);
            packed_at = start.packed;
            unpacked_at = start.unpacked;
        }
        segment(unpacked_at, asset.entry.packed - packed_at, asset.entry.unpacked - unpacked_at);
        return std::make_unique<IO::MemoryFile>(std::move(output));
    }

    bool TabV1::foreach_file(const std::function<bool(const Archive<u64>::ArchiveEntry &)> &callback) {
        const auto total = entries.size();
        uint64_t index = 0;
        for (const auto &[hash, asset] : entries)
            if (!callback({hash, 0, asset.entry.unpacked, total, index++})) return false;
        return true;
    }
}
