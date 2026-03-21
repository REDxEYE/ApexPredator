// Created by RED on 17.09.2025.

#ifndef APEXPREDATOR_BUFFER_H
#define APEXPREDATOR_BUFFER_H
#include <span>
#include <vector>
#include "fstream"

#include "int_def.h"

namespace IO {
    class File {
    public:
        virtual ~File() = default;

        virtual void set_position(std::streamoff position, std::ios::seekdir origin) = 0;

        void set_position(const std::streamoff position) {
            set_position(position, std::ios::beg);
        }

        virtual std::streamsize get_position() = 0;

        virtual size_t remaining();

        virtual void align(size_t alignment);

        virtual size_t read(void *dst, std::streamsize size) = 0;

        virtual size_t write(const void *src, std::streamsize size) = 0;

        virtual std::vector<uint8> read(size_t size);

        template<typename T>
            requires std::is_trivially_copyable_v<T> && std::is_trivial_v<T>
        std::vector<T> read_exact(size_t size) {
            std::vector<T> data(size);
            read_exact(data);
            return std::move(data);
        }

        template<typename T>
            requires std::is_trivially_copyable_v<T> && std::is_trivial_v<T>
        void read_exact(std::vector<T> &data) {
            const size_t bytesRead = read(data.data(), data.size() * sizeof(T));
            if (bytesRead < data.size() * sizeof(T)) {
                throw std::runtime_error("Failed to read enough bytes for type array");
            }
        }

        template<typename T>
            requires std::is_trivially_copyable_v<T> && std::is_trivial_v<T> && (!std::is_const_v<T>)
        void read_exact(std::span<T> data) {
            const size_t bytesRead = read(data.data(), data.size() * sizeof(T));
            if (bytesRead < data.size() * sizeof(T)) {
                throw std::runtime_error("Failed to read enough bytes for type array");
            }
        }

        virtual size_t get_size() = 0;

        virtual void close() = 0;

        virtual size_t skip(uint32 size) = 0;

        template<typename T>
            requires std::is_trivially_copyable_v<T> && std::is_trivial_v<T>
        T read_pod() {
            T value;
            if (const size_t bytesRead = read(&value, sizeof(T)); bytesRead < sizeof(T)) {
                throw std::runtime_error("Failed to read enough bytes for type");
            }
            return value;
        }


        virtual void read_cstring(std::string &str);

        virtual std::string read_cstring();

        virtual void read_string(uint32 size, std::string &str);

        template<typename T>
            requires std::is_trivially_copyable_v<T> && std::is_trivial_v<T>
        void write(T data) {
            const size_t bytesWritten = write(&data, sizeof(T));
            if (bytesWritten < sizeof(T)) {
                throw std::runtime_error("Failed to write enough bytes for type");
            }
        }

        virtual std::span<const uint8> cbuffer() = 0;
        // virtual std::span<uint8> buffer() = 0;
    };
}
#endif //APEXPREDATOR_BUFFER_H
