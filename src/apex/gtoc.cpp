#include "apex/gtoc.h"

#include <cstring>
#include <limits>
#include <stdexcept>
#include <unordered_map>

#include "apex/asset_db.h"

namespace {
    void require_range(const size_t offset, const size_t count, const size_t size) {
        if (offset > size || count > size - offset) {
            throw std::runtime_error("Truncated GTOC data");
        }
    }

    uint32 read_u32(const std::vector<uint8> &data, const size_t offset) {
        require_range(offset, 4, data.size());
        return static_cast<uint32>(data[offset])
               | (static_cast<uint32>(data[offset + 1]) << 8)
               | (static_cast<uint32>(data[offset + 2]) << 16)
               | (static_cast<uint32>(data[offset + 3]) << 24);
    }
}

GTOCFile::GTOCFile(IO::File &buffer) {
    const auto position = buffer.get_position();
    const auto buffer_size = buffer.get_size();
    if (position < 0 || static_cast<uint64>(position) > buffer_size) {
        throw std::runtime_error("Invalid GTOC input position");
    }
    const size_t size = buffer_size - static_cast<size_t>(position);
    if (size < 8 || size > static_cast<uint64>(std::numeric_limits<std::streamsize>::max())
        || size > m_data.max_size()) {
        throw std::runtime_error("Invalid GTOC input size");
    }
    m_data.resize(size);
    buffer.read_exact(m_data);
    if (std::memcmp(m_data.data(), GTOC_MAGIC, 4) != 0) {
        throw std::runtime_error("Invalid GTOC magic");
    }

    const uint32 archive_count = read_u32(m_data, 4);
    if (archive_count > (size - 8) / 12) {
        throw std::runtime_error("Invalid GTOC archive count");
    }

    // Locate the metadata pool before following any references. This also
    // validates every variable-length archive record before allocating tables.
    size_t records_end = 8;
    for (uint32 i = 0; i < archive_count; ++i) {
        require_range(records_end, 12, size);
        const uint32 member_count = read_u32(m_data, records_end + 8);
        records_end += 12;
        if (member_count > (size - records_end) / 8) {
            throw std::runtime_error("Invalid GTOC member count");
        }
        records_end += static_cast<size_t>(member_count) * 8;
    }

    std::unordered_map<size_t, uint32> metadata_indices;
    m_archives.reserve(archive_count);
    size_t cursor = 8;
    for (uint32 i = 0; i < archive_count; ++i) {
        const uint32 hash = read_u32(m_data, cursor);
        const uint32 tag = read_u32(m_data, cursor + 4);
        const uint32 member_count = read_u32(m_data, cursor + 8);
        cursor += 12;
        auto &archive = m_archives.emplace_back(GTOCArchiveEntry{hash, tag, {}});
        archive.members.reserve(member_count);

        for (uint32 j = 0; j < member_count; ++j) {
            const uint32 relative = read_u32(m_data, cursor);
            const uint32 offset = read_u32(m_data, cursor + 4);
            // The unsigned displacement is relative to its own field, not
            // the archive header or the following archive-offset field.
            require_range(cursor, relative, size);
            const size_t metadata = cursor + relative;
            if (metadata < records_end || metadata % 4 != 0) {
                throw std::runtime_error("Invalid GTOC metadata reference");
            }

            uint32 file_index;
            if (const auto it = metadata_indices.find(metadata); it != metadata_indices.end()) {
                file_index = it->second;
            } else {
                require_range(metadata, 13, size);
                const char *name = reinterpret_cast<const char *>(m_data.data() + metadata + 12);
                const auto *end = static_cast<const char *>(std::memchr(name, '\0', size - metadata - 12));
                if (end == nullptr) {
                    throw std::runtime_error("Unterminated GTOC file name");
                }
                if (m_files.size() > std::numeric_limits<uint32>::max()) {
                    throw std::runtime_error("Too many GTOC metadata records");
                }
                file_index = static_cast<uint32>(m_files.size());
                m_files.push_back({
                    read_u32(m_data, metadata),
                    read_u32(m_data, metadata + 4),
                    read_u32(m_data, metadata + 8),
                    std::string_view(name, static_cast<size_t>(end - name))
                });
                metadata_indices.emplace(metadata, file_index);
            }
            archive.members.push_back({file_index, offset});
            cursor += 8;
        }
    }
}

bool GTOCArchive::has(const uint64 &key) {
    const auto &it = m_hash_remap.find(key);
    return it != m_hash_remap.end();
}

std::unique_ptr<IO::File> GTOCArchive::get(const uint64 &key) {
    auto db = AssetDB::get_instance();
    const auto &it = m_hash_remap.find(key);
    auto search_hash = key;
    if (it != m_hash_remap.end()) {
        search_hash = it->second;
    }
    for (const auto &archive: m_file.archives()) {
        for (const auto &member: archive.members) {
            const auto &file = m_file.files()[member.file_index];
            if (file.hash == search_hash) {
                if (member.offset == 0) {
                    return nullptr;
                }


                const auto file_info = db->get_file(archive.hash, AssetDB::HashType::Lookup3);
                if (!file_info.has_value()) {
                    return nullptr;
                }
                const auto sarc = m_manager.get(file_info->murmur);
                if (!sarc) {
                    return nullptr;
                }
                sarc->set_position(member.offset);
                auto mem_file = std::make_unique<IO::MemoryFile>(file.size);
                sarc->read(mem_file->buffer().data(), file.size);
                return std::move(mem_file);
            }
        }
    }
    return nullptr;
}

std::string_view GTOCArchive::name() const {
    return m_name;
}

const uint64 &GTOCArchive::key() const {
    return m_hash;
}

bool GTOCArchive::foreach_file(const std::function<bool(const ArchiveEntry &)> &callback) {
    ArchiveEntry entry{};
    for (const auto &file: m_file.files()) {
        entry.size = file.size;
        entry.key = asset_path_hash(file.name);
        if (!callback(entry)) {
            return false;
        }
    }
    return true;
}
