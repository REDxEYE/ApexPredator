// Created by RED on 07.10.2025.

#ifndef APEXPREDATOR_HASHES_H
#define APEXPREDATOR_HASHES_H

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>


#include "int_def.h"

void set_db_path(const std::filesystem::path &path);
std::optional<std::string> find_name(uint64 key);
bool check_hash_presence(uint64 key);
void store_hash_name(uint64 key, const std::string_view& value);
//
void search_file_table(std::string_view pattern, std::vector<std::string> &result);
std::optional<uint32> get_file_parent(uint64 key);
std::optional<std::string> get_file_parent(uint64 key, uint64& out_parent);

std::filesystem::path get_export_path(const std::filesystem::path &base_export_path, const uint32 hash, std::string_view ext);

#endif //APEXPREDATOR_HASHES_H
