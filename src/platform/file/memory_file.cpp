// Created by RED on 18.09.2025.

#include "platform/file/memory_buffer.h"
#include <cstdlib>
#include <cstring>

#include "platform/logger.h"

void IO::MemoryFile::set_position(const std::streamoff position, const std::ios::seekdir origin) {
    std::streamoff new_position = 0;
    switch (origin) {
        case std::ios::beg:
            if (position < 0) return;
            new_position = position;
            break;
        case std::ios::cur:
            if (position < 0 && (-position) > m_position) return;
            if (m_position + position > m_data.size()) return;
            new_position = m_position + position;
            break;
        case std::ios::end:
            if (position < 0 && (-position) > m_data.size()) return;
            new_position = m_data.size() + position;
            break;
        default:
            return; // Invalid seek direction
    }
    if (new_position > m_data.size()) {
        return;
    }
    m_position = new_position;
}

std::streamsize IO::MemoryFile::get_position() {
    return m_position;
}

size_t IO::MemoryFile::read(void *dst, const std::streamsize size) {
    if (m_position + size > m_data.size()) {
        throw std::runtime_error("Attempt to read beyond end of buffer");
    }

    if (size == 0) {
        return 0;
    }

    std::memcpy(dst, m_data.data() + m_position, size);
    m_position += size;
    return size;
}

size_t IO::MemoryFile::write(const void *src, const std::streamsize size) {
    if (m_position + size > m_data.size()) {
        m_data.resize(m_position + size);
    }

    memcpy(m_data.data() + m_position, src, size);
    m_position += size;
    return size;
}

size_t IO::MemoryFile::get_size() {
    return m_data.size();
}

void IO::MemoryFile::close() {
    m_data = Buffer{};
    m_position = 0;
}

size_t IO::MemoryFile::skip(const uint32 size) {
    if (m_position + size > m_data.size()) {
        throw std::runtime_error("Attempt to skip beyond end of buffer");
    }
    m_position += size;
    return size;
}

IO::MemoryViewFile IO::MemoryFile::take_span(const size_t size, const size_t offset) {
    return take_view(size, offset);
}

IO::MemoryViewFile IO::MemoryFile::take_view(const size_t size, const size_t offset) {
    if (offset + size > m_data.size()) {
        throw std::runtime_error("Attempt to take view beyond end of buffer");
    }
    return MemoryViewFile(m_data.view(offset, size));
}

std::span<const uint8> IO::MemoryFile::cbuffer() {
    return {m_data.data(), m_data.size()};
}

void IO::MemoryFile::resize(size_t new_size) {
    m_data.resize(new_size);
}

void IO::MemoryViewFile::set_position(const std::streamoff position, const std::ios::seekdir origin) {
    std::streamoff new_position = 0;
    switch (origin) {
        case std::ios::beg:
            if (position < 0) return;
            new_position = position;
            break;
        case std::ios::cur:
            if (position < 0 && (-position) > m_data.size()) return;
            if (m_position + position > m_data.size()) return;
            new_position = m_position + position;
            break;
        case std::ios::end:
            if (position < 0 && (-position) > m_data.size()) return;
            new_position = m_data.size() + position;
            break;
        default:
            return; // Invalid seek direction
    }
    if (new_position > m_data.size()) {
        return;
    }
    m_position = new_position;
}

std::streamsize IO::MemoryViewFile::get_position() {
    return m_position;
}

size_t IO::MemoryViewFile::read(void *dst, const std::streamsize size) {
    if (m_position + size > m_data.size()) {
        throw std::runtime_error("Attempt to read beyond end of buffer");
    }

    if (size == 0) {
        return 0;
    }

    memcpy(dst, m_data.data() + m_position, size);
    m_position += size;
    return size;
}

size_t IO::MemoryViewFile::write(const void *src, std::streamsize size) {
    throw std::runtime_error("Cannot write to a read-only MemoryViewBuffer");
}

size_t IO::MemoryViewFile::get_size() {
    return m_data.size();
}

void IO::MemoryViewFile::close() {
    m_data = ConstByteBufferView{};
    m_position = 0;
}

size_t IO::MemoryViewFile::skip(const uint32 size) {
    if (m_position + size > m_data.size()) {
        throw std::runtime_error("Attempt to skip beyond end of buffer");
    }
    m_position += size;
    return size;
}

std::span<const uint8> IO::MemoryViewFile::cbuffer() {
    return {m_data.data(), m_data.size()};
}
