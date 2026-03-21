// Created by RED on 17.09.2025.

#include "platform/file/file.h"

size_t IO::File::remaining() {
    auto cur_pos = get_position();
    return get_size() - cur_pos;
}

void IO::File::align(const size_t alignment) {
    if (alignment == 0) {
        return;
    }
    const auto cur_pos = get_position();
    const auto aligned_pos = (cur_pos + alignment - 1) & ~(alignment - 1);
    set_position(aligned_pos, std::ios::beg);
}

std::vector<uint8> IO::File::read(const size_t size) {
    std::vector<uint8> data(size);
    const size_t bytesRead = read(data.data(), size);
    if (bytesRead < size) {
        data.resize(bytesRead); // Resize to actual bytes read
    }
    return std::move(data);
}


void IO::File::read_cstring(std::string &str) {
    str.clear();
    while (true) {
        char c;
        const size_t bytesRead = read(&c, sizeof(c));
        if (bytesRead == 0) {
            break; // End of buffer
        }
        if (c == '\0') {
            break; // Null terminator found
        }
        str += c;
    }
}

std::string IO::File::read_cstring() {
    std::string result;
    read_cstring(result);
    return std::move(result);
}

void IO::File::read_string(const uint32 size, std::string &str) {
    str.clear();
    str.resize(size);
    const auto read_bytes = read(str.data(), size);
    if (read_bytes < size) {
        str.resize(read_bytes);
        throw std::runtime_error("Failed to read the expected number of bytes for string");
    }
}
