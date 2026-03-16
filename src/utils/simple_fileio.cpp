// Created by RED on 18.02.2026.

#include "utils/simple_fileio.h"
#include "platform/logger.h"
#include <fstream>

void write_file(const std::filesystem::path &path, const std::span<const uint8> &data) {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        GLog_Error("Failed to open file \"{}\" for writing", path.string());
        throw std::runtime_error(std::format("Failed to open file \"{}\" for writing", path.string()));
    }
    stream.write(reinterpret_cast<const char *>(data.data()), data.size());
     if (!stream) {
        GLog_Error("Failed to write to file \"{}\"", path.string());
        throw std::runtime_error(std::format("Failed to write to file \"{}\"", path.string()));
    }
    stream.close();
}
