// Created by RED on 07.10.2025.

#ifndef APEXPREDATOR_HASHES_H
#define APEXPREDATOR_HASHES_H

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>


#include "int_def.h"
#include "utils/sqlite_wrapper.h"

void set_db_path(const std::filesystem::path &path);
const std::filesystem::path& get_db_path();

assetdb_t *get_assets_db();
void close_assets_db();
std::optional<std::string_view> find_name32_sv(uint32 key);
std::optional<std::string_view> find_name64_sv(uint64 key);
std::optional<std::string> find_name32(uint32 key);
std::optional<std::string> find_name64(uint64 key);
bool check_hash32_presence(uint32 key);
bool check_hash64_presence(uint64 key);
void store_hash32_name(uint32 key, const std::string_view& value);
void store_hash64_name(uint64 key, const std::string_view& value);

void search_file_table(std::string_view pattern, std::vector<std::string> &result);

std::optional<uint32> get_file_parent(uint64 key);
std::optional<std::string> get_file_parent(uint64 key, uint64& out_parent);

std::filesystem::path get_export_path(const std::filesystem::path &base_export_path, const uint32 hash, std::string_view ext);

#endif //APEXPREDATOR_HASHES_H
