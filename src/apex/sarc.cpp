// Created by RED on 02.10.2025.

#include "apex/sarc.h"

#include <cstring>
#include <format>
#include <ranges>

#include "tracy/Tracy.hpp"
#include "apex/hashes.h"
#include "platform/logger.h"
#include "utils/hash_helper.h"


SArchive::SArchive(const uint32 m_hash, std::unique_ptr<IO::File> buffer): m_hash(m_hash),
                                                                m_buffer(std::move(buffer)) {
    ZoneScoped
    m_header = m_buffer->read_pod<SArcHeader>();
    if (std::memcmp(m_header.ident, "SARC", 4) != 0) {
        throw std::runtime_error("Invalid SARC magic");
    }
    if (m_header.version2==2) {
        throw std::runtime_error("SARC version 2 is not supported");
    }
    if (m_header.version2==3) {
        const auto strings_size = m_buffer->read_pod<uint32>();
        m_strings.resize(strings_size);
        m_buffer->read_exact(m_strings);
        const uint32 entry_count = (m_header.dir_block_len - 4/* strings_size int */ - strings_size) / 20;

        m_entries.reserve(entry_count);
        for (uint32 i = 0; i < entry_count; ++i) {
            const auto name_offset = m_buffer->read_pod<uint32>();

            SArcEntry entry{
                .name = std::string_view(&m_strings[name_offset]),
                .offset =  m_buffer->read_pod<uint32>(),
                .size = m_buffer->read_pod<uint32>(),
                .hash = m_buffer->read_pod<uint32>(),
                .ext_hash = m_buffer->read_pod<uint32>(),
            };
            if (hash_string(entry.name) != entry.hash) {
                throw std::runtime_error("SARC entry hash mismatch for file " + std::string(entry.name));
            }
            m_entries[entry.hash] = entry;
        }
    }
}

bool SArchive::has_file(const std::string_view path) const {
    const uint32 hash = hash_string(path);
    return m_entries.contains(hash);
}

bool SArchive::has_file(const uint32 hash) const{
    return m_entries.contains(hash);
}

std::unique_ptr<IO::File> SArchive::get_file(const std::string_view path) {
    return get_file(hash_string(path));
}

std::unique_ptr<IO::File> SArchive::get_file(const uint32 hash) {
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

void SArchive::all_entries(std::vector<ArchiveEntry> &entries) const {
    entries.reserve(entries.size() + m_entries.size());
    for (const auto &entry: m_entries | std::views::values) {
        if (entry.offset == 0) {
            continue;
        }
        entries.emplace_back(entry.hash, entry.size);
    }
}

std::string SArchive::get_name() const {
    if (const auto name = find_name(m_hash)) {
        return std::string(*name);
    }
    return std::format("SARC 0x%08X", m_hash);
}

uint32 SArchive::hash() {
    return m_hash;
}
