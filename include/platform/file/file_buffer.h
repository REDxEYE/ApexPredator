// Created by RED on 17.09.2025.

#ifndef APEXPREDATOR_FILE_BUFFER_H
#define APEXPREDATOR_FILE_BUFFER_H

#include <filesystem>

#include "fstream"

#include "platform/file/file.h"

namespace IO {
    class NativeFile : public File {
    public:
        ~NativeFile() override {
            close();
        }

        const std::fstream& stream() const;

        NativeFile() = default;

        void open_read(const std::filesystem::path& path) {
            close();
            m_stream.clear();
            m_path = path;
            m_stream.open(m_path, std::ios::in | std::ios::binary);
        }

        void open_write(const std::filesystem::path& path) {
            close();
            m_stream.clear();
            m_path = path;
            m_stream.open(m_path, std::ios::out | std::ios::binary);
        }

        explicit NativeFile(const std::filesystem::path &path, const std::ios::openmode mode)
            : m_path(path), m_stream(path, mode) {
        }

        void set_position(std::streamoff position, std::ios::seekdir origin) override;

        std::streamsize get_position() override;

        size_t read(void *dst, std::streamsize size) override;

        size_t write(const void *src, std::streamsize size) override;

        size_t get_size() override;

        void close() override;

        size_t skip(uint32 size) override;

        [[nodiscard]] const std::filesystem::path& path() const;

        std::span<const uint8> cbuffer() override;

    private:
        std::filesystem::path m_path{};
        std::fstream m_stream;
    };
}
#endif //APEXPREDATOR_FILE_BUFFER_H
