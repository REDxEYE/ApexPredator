// Created by RED on 18.02.2026.

#ifndef APEXPREDATOR_SIMPLE_FILEIO_H
#define APEXPREDATOR_SIMPLE_FILEIO_H
#include "int_def.h"

#include <filesystem>
#include <span>

void write_file(const std::filesystem::path& path, const std::span<const uint8> &data);

#endif //APEXPREDATOR_SIMPLE_FILEIO_H
