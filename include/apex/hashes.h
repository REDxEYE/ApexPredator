// Created by RED on 07.10.2025.

#ifndef APEXPREDATOR_HASHES_H
#define APEXPREDATOR_HASHES_H

#include "int_def.h"
#include "utils/sqlite_wrapper.h"
#include "utils/string.h"


assetdb_t *get_assets_db();
void close_assets_db();
String *find_name32(uint32 key);
String *find_name64(uint64 key);
bool check_hash32_presence(uint32 key);
bool check_hash64_presence(uint64 key);
void store_hash32_name(uint32 key, const String* value);
void store_hash64_name(uint64 key, const String* value);

void store_file_parent(uint64 key, const String* path, uint64 parent);
bool get_file_parent(uint64 key, uint64 *out_parent, String **out_path);

#endif //APEXPREDATOR_HASHES_H
