#pragma once

#include <filesystem>
#include <span>
#include <string_view>

#include "redscore/int_def.h"

class AssetDB;
template<class T> class ArchiveManager;

struct CollectorPolicy {
    std::filesystem::path database_path;
    std::span<const std::string_view> string_files;
    std::span<const std::string_view> initial_gtocs;
    bool defer_gtocs = false;
    bool ingest_discovered_entities = false;
    void (*ingest_locations)(AssetDB &, ArchiveManager<u64> &) = nullptr;
};

// Exactly one game's implementation is linked into each collector executable.
CollectorPolicy collector_policy();
