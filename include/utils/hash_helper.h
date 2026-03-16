// Created by RED on 03.10.2025.

#ifndef APEXPREDATOR_HASH_HELPER_H
#define APEXPREDATOR_HASH_HELPER_H
#include <string>
#include <filesystem>

#include "int_def.h"


uint32 hash_string(const std::string &str);

uint32 hash_string(const std::filesystem::path &str);

uint32 hash_string(const char *str);

uint32 hash_string(std::string_view sv);

#endif //APEXPREDATOR_HASH_HELPER_H
