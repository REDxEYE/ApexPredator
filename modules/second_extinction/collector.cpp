#include "tools/collector_policy.h"

#include <array>
#include <sstream>
#include <string>
#include <unordered_map>

#include "apex/asset_db.h"
#include "apex/hashes.h"
#include "apex/rtpc.h"
#include "platform/archive_manager.h"
#include "utils/hash_helper.h"

namespace {
void ingest_location_archives(AssetDB &db, ArchiveManager<u64> &archives) {
    auto world = archives.get(asset_path_hash("locations/world.bin"));
    if (!world) return;

    const auto root = RuntimeNode::RootNode(world);
    constexpr uint32 file_hash = const_hash_string("file");
    constexpr uint32 extension_list_hash = 0xA64E1E84; // RTPC extension list for near/far locations

    struct LocationArchiveName {
        uint32 lookup3;
        std::string path;
    };
    std::unordered_map<uint64, LocationArchiveName> names;

    for (const auto &location: root.children()) {
        if (!location.has(file_hash) || !location.is<std::string>(file_hash)) continue;
        const auto &stem = location.get<std::string>(file_hash);
        if (stem.empty()) continue;

        const auto add_name = [&](const std::string_view extension) {
            std::string path;
            path.reserve(stem.size() + extension.size());
            path.append(stem);
            path.append(extension);
            const auto hashes = string_hashes(path);
            names.try_emplace(hashes.murmur, LocationArchiveName{hashes.lookup3, std::move(path)});
        };

        add_name(".bl");
        if (location.has(extension_list_hash) &&
            location.is<std::string>(extension_list_hash)) {
            const auto &extensions = location.get<std::string>(extension_list_hash);
            auto ext_stream = std::stringstream{extensions};
            while (ext_stream.good()) {
                std::string extension;
                ext_stream >> extension;
                add_name(extension);
            }
        }
    }
    // Register real TAB entries before GTOC traversal needs lookup3 -> Murmur.
    archives.foreach_file([&](const Archive<u64>::ArchiveEntry &entry) {
        if (const auto it = names.find(entry.key); it != names.end()) {
            const StringHashes hashes{it->second.lookup3, entry.key};
            db.kv_put(hashes, it->second.path);
            db.files_put(hashes, it->second.path, entry.size, entry.parent);
        }
        return true;
    });
}
} // namespace

CollectorPolicy collector_policy() {
    static constexpr std::array strings{
        std::string_view{"./../strings/second_extinction/filelist.txt"}
    };
    static constexpr std::array gtocs{
        std::string_view{"sarc.0.gtoc"},
        std::string_view{"resourcesets/expentities.gtoc"}
    };
    return {"./../hashes.db", strings, gtocs, true, true, ingest_location_archives};
}
