// Created by RED on 17.09.2025.

#include "platform/file/file_buffer.h"

const std::fstream & IO::NativeFile::stream() const {
    return m_stream;
}

void IO::NativeFile::set_position(const std::streamoff position, const std::ios::seekdir origin) {
    m_stream.seekg(position, origin);
}

std::streamsize IO::NativeFile::get_position() {
    return m_stream.tellg();
}

size_t IO::NativeFile::read(void *dst, const std::streamsize size) {
    m_stream.read(static_cast<char *>(dst), static_cast<std::streamsize>(size));
    return m_stream.gcount();
}

size_t IO::NativeFile::write(const void *src, std::streamsize size) {
    m_stream.write(static_cast<const char *>(src), static_cast<std::streamsize>(size));
    return size;
}

size_t IO::NativeFile::get_size() {
    const auto current_pos = m_stream.tellg();
    m_stream.seekg(0, std::ios::end);
    const auto size = m_stream.tellg();
    m_stream.seekg(current_pos, std::ios::beg);
    return size;
}

void IO::NativeFile::close() {
    if (m_stream.is_open()) {
        m_stream.close();
    }
}

size_t IO::NativeFile::skip(uint32 size) {
    m_stream.seekg(size, std::ios::cur);
    return size;
}

const std::filesystem::path & IO::NativeFile::path() const {
    return m_path;
}

std::span<const uint8> IO::NativeFile::cbuffer() {
    throw std::runtime_error("FileBuffer does not support direct buffer access");
}
