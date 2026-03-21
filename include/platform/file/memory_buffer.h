// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_MEMORY_BUFFER_H
#define APEXPREDATOR_MEMORY_BUFFER_H
#include <span>

#include "platform/buffer/buffer.h"
#include "platform/file/file.h"

namespace IO {
    class MemoryViewFile;

    class MemoryFile : public File {
    public:
        ~MemoryFile() override {
            close();
        }

        // explicit MemoryBuffer(const std::vector<uint8> &data)
        //     : m_data(data) {
        // }
        explicit MemoryFile(std::vector<uint8> &&data)
            : m_data(std::move(data)) {
        }

        explicit MemoryFile(Buffer data)
            : m_data(std::move(data)) {
        }

        explicit MemoryFile(const size_t size) {
            m_data.resize(size);
        }

        void set_position(std::streamoff position, std::ios::seekdir origin) override;

        std::streamsize get_position() override;

        size_t read(void *dst, std::streamsize size) override;

        size_t write(const void *src, std::streamsize size) override;

        size_t get_size() override;

        void close() override;

        size_t skip(uint32 size) override;

        [[nodiscard]] MemoryViewFile take_span(size_t size, size_t offset);
        [[nodiscard]] MemoryViewFile take_view(size_t size, size_t offset);

        std::span<const uint8> cbuffer() override;

        [[nodiscard]] Buffer &buffer() noexcept { return m_data; }
        [[nodiscard]] const Buffer &buffer() const noexcept { return m_data; }

        void resize(size_t new_size);

    private:
        MemoryFile() = default;

        std::streamsize m_position{0};
        Buffer m_data;
    };

    class MemoryViewFile : public File {
    public:
        ~MemoryViewFile() override {
            close();
        }

        explicit MemoryViewFile(uint8 *data, const size_t size) {
            m_data = ConstByteBufferView(data, size);
        }

        explicit MemoryViewFile(const uint8 *data, const size_t size) : m_data(data, size) {
        }

        explicit MemoryViewFile(ConstByteBufferView data):m_data(data){};

        explicit MemoryViewFile(const std::span<const uint8> data) : m_data(data.data(), data.size()) {
        }

        void set_position(std::streamoff position, std::ios::seekdir origin) override;

        std::streamsize get_position() override;

        size_t read(void *dst, std::streamsize size) override;

        size_t write(const void *src, std::streamsize size) override;

        size_t get_size() override;

        void close() override;

        size_t skip(uint32 size) override;

        std::span<const uint8> cbuffer() override;

    private:
        std::streamsize m_position{0};
        ConstByteBufferView m_data;
    };
}
#endif //APEXPREDATOR_MEMORY_BUFFER_H
