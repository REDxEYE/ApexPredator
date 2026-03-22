// Created by RED on 01.10.2025.

#include "apex/aaf/aaf.h"

#include <string>

#include "redscore/platform/file/memory_buffer.h"
#include "tracy/Tracy.hpp"
#include "utils/zlib_wrapper.h"


AAFArchive::AAFArchive(std::unique_ptr<IO::File> buffer) : m_buffer(std::move(buffer)) {
    ZoneScoped
    m_header = m_buffer->read_pod<AAFHeader>();

    if (strncmp(m_header.ident, "AAF", 3) != 0) {
        throw std::runtime_error("Invalid AAF format");
    }

    if (m_header.version != 1) {
        throw std::runtime_error("Unsupported AAF version: " + std::to_string(m_header.version));
    }

    m_sections.reserve(m_header.section_count);
    uint64 total_size = 0;
    for (uint32 i = 0; i < m_header.section_count; ++i) {
        auto start = m_buffer->get_position();
        auto &[header, section_buffer] = m_sections.emplace_back();
        header = m_buffer->read_pod<AAFSectionHeader>();
        if (memcmp(header.magic, "EWAM", 4) != 0) {
            throw std::runtime_error("Invalid AAF section magic");
        }
        section_buffer.resize(header.compressed_size);
        m_buffer->read_exact(section_buffer);
        total_size += header.uncompressed_size;
        m_buffer->set_position(start+header.total_size);
    }

    if (total_size != m_header.uncompressed_size) {
        throw std::runtime_error(
            "AAF archive uncompressed size mismatch, expected: " + std::to_string(m_header.uncompressed_size) +
            ", actual: " + std::to_string(total_size));
    }
}

std::unique_ptr<IO::File> AAFArchive::get_data() {
    ZoneScoped
    auto out = std::vector<uint8>(m_header.uncompressed_size);
    uint64 offset = 0;
    for (size_t index = 0; index < m_sections.size(); ++index) {
        const auto &[header, buffer] = m_sections[index];
        if (buffer.empty()) {
            throw std::runtime_error("Empty AAF section data");
        }

        const int res = inflate_exact_raw(buffer.data(), buffer.size(), out.data() + offset,
                                    header.uncompressed_size, nullptr, nullptr);
        if (res != Z_OK) {
            throw std::runtime_error("Failed to decompress AAF section " + std::to_string(index) + ", zlib error: " + std::to_string(res));
        }
        offset += header.uncompressed_size;
    }

    return std::make_unique<IO::MemoryFile>(std::move(out));
}
