// Created by RED on 02.10.2025.

#include "apex/sarc.h"

#include <cstring>
#include <format>
#include <ranges>

#include "tracy/Tracy.hpp"
#include "apex/hashes.h"
#include "redscore/platform/logger.h"
#include "utils/hash_helper.h"


SArchive::SArchive(const uint64 m_hash, std::unique_ptr<IO::File> buffer) : m_hash(m_hash),
                                                                            m_buffer(std::move(buffer)) {
    ZoneScoped
    m_header = m_buffer->read_pod<SArcHeader>();
    if (std::memcmp(m_header.ident, "SARC", 4) != 0) {
        throw std::runtime_error("Invalid SARC magic");
    }
    if (m_header.version2 == 2) {
        const auto entries_end = sizeof(SArcHeader) + m_header.dir_block_len - 12/*minimal entry size*/;
        m_strings.reserve(m_header.dir_block_len);
        while (m_buffer->get_position() < entries_end) {
            const auto name_len = m_buffer->read_u32();
            std::string name;
            m_buffer->read_string(name_len, name);
            const auto it = m_strings.insert(m_strings.end(), name.cbegin(), name.cend());
            const auto index = static_cast<std::size_t>(std::distance(m_strings.begin(), it));
            m_strings.emplace_back('\0');
            const auto offset = m_buffer->read_u32();
            const auto size = m_buffer->read_u32();
            const SArcEntry entry{
                .name = std::string_view(&m_strings[index]),
                .offset = offset,
                .size = size,
                .hash = static_cast<uint32>(hash_string(name)),
                .ext_hash = 0
            };
            m_entries[entry.hash] = entry;
        }
    } else if (m_header.version2 == 3) {
        const auto strings_size = m_buffer->read_u32();
        m_strings.resize(strings_size);
        m_buffer->read_exact(m_strings);
        const uint32 entry_count = (m_header.dir_block_len - 4/* strings_size int */ - strings_size) / 20;

        m_entries.reserve(entry_count);
        for (uint32 i = 0; i < entry_count; ++i) {
            const auto name_offset = m_buffer->read_u32();

            const SArcEntry entry{
                .name = std::string_view(&m_strings[name_offset]),
                .offset = m_buffer->read_u32(),
                .size = m_buffer->read_u32(),
                .hash = m_buffer->read_u32(),
                .ext_hash = m_buffer->read_u32(),
            };
            if (asset_path_hash(entry.name) != entry.hash) {
                throw std::runtime_error("SARC entry hash mismatch for file " + std::string(entry.name));
            }
            m_entries[entry.hash] = entry;
        }
    } else {
        throw std::runtime_error(std::format("SARC version {} is not supported", m_header.version2));
    }

    if (const auto name = find_asset_name(m_hash)) {
        m_name = name.value();
    } else {
        m_name = std::format("SARC 0x{:08X}", m_hash);
    }
}

bool SArchive::has(const std::string_view path) {
    const uint64 hash = asset_path_hash(path);
    return m_entries.contains(hash);
}

bool SArchive::has(const u64 &hash) {
    return m_entries.contains(hash);
}

std::unique_ptr<IO::File> SArchive::get(const std::string_view path) {
    return get(asset_path_hash(path));
}

std::unique_ptr<IO::File> SArchive::get(const u64 &hash) {
    ZoneScoped
    const auto it = m_entries.find(hash);
    if (it == m_entries.end()) {
        return nullptr;
    }
    const SArcEntry &entry = it->second;
    if (entry.offset == 0) {
        return nullptr;
    }
    const uint64 buffer_size = m_buffer->get_size();
    if (entry.offset + entry.size > buffer_size) {
        GLog_Error("Invalid SARC entry size for file %s", entry.name.data());
        return nullptr;
    }
    std::vector<uint8> buffer(entry.size);
    m_buffer->set_position(entry.offset, std::ios::beg);
    m_buffer->read_exact(buffer);
    return std::make_unique<IO::MemoryFile>(std::move(buffer));
}

const u64 &SArchive::get_parent_key() {
    return m_hash;
}


std::string_view SArchive::name() const {
    return m_name;
}

const u64 &SArchive::key() const {
    return m_hash;
}

bool SArchive::foreach_file(const std::function<bool(const ArchiveEntry &)> &callback) {
    auto total = m_entries.size();
    for (const auto &[i, entry]: m_entries | std::views::values | std::views::enumerate) {
        if (!callback({entry.hash, key(), entry.size, total, static_cast<uint64>(i)})) {
            break;
        }
    }
    return true;
}
