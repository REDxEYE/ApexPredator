// Created by RED on 26.09.2025.
#include <ranges>
#include <span>
#include <unordered_map>
#include <unordered_set>

#include "games.hpp"

#include "apex/hashes.h"
#include "apex/gtoc.h"
#include "apex/rtpc.h"
#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "apex/adf/adf.h"
// #include "apex/adf/generated/adf_types.h"
// #include "havok/tag_file/havok_tag_file.h"
// #include "exporter/havok_export.h"
#include "apex/package/tab_archive.h"
#include "redscore/platform/file/memory_file.h"
#include "utils/hash_helper.h"
#include "apex/asset_db.h"
#include "apex/avtx.h"
#include "platform/app_state.h"

typedef struct Context {
    AssetDB &db;
    ArchiveManager<u64> &archives;
} Context;

bool visit_adf_file(std::unique_ptr<IO::File> &&file) {
    ADF::ADFFile adf = ADF::ADFFile::from_buffer(std::move(file));
    return true;
}

bool visit_ptpc_nodes(Context &ctx, const RuntimeNode &runtime_node) {
    for (const auto &prop: runtime_node.props() | std::views::values) {
        const auto &value = prop.value();
        if (std::holds_alternative<std::string>(value)) {
            const auto &str = std::get<std::string>(value);
            ctx.db.kv_put(string_hashes(str), str);
        }
    }

    for (const auto &child: runtime_node.children()) {
        visit_ptpc_nodes(ctx, child);
    }
    return true;
}
#if GAME==GAME_SECOND_EXTINCTION
static std::vector<std::unique_ptr<GTOCArchive> > pending_gtocs;
#endif

static uint64 total_files = 0;
static uint64 total_named = 0;

static std::unordered_map<uint32, uint32> container_tags;
// An embedded entity definition names its .ee container only when its stem
// hashes to that GTOC archive. Catalog names alone do not imply TAB payloads.
void ingest_entity_archives(AssetDB &db, ArchiveManager<u64> &archives, const GTOCFile &gtoc) {
    struct NamedArchive {
        StringHashes hashes;
        std::string path;
    };
    std::unordered_map<u64, NamedArchive> physical_names;

    for (const auto &archive: gtoc.archives()) {
        for (const auto &[index, offset]: archive.members) {
            if (offset == 0) continue;
            const std::string_view member = gtoc.files()[index].name;
            constexpr std::string_view adf_extension = ".epe_adf";
            constexpr std::string_view epe_extension = ".epe";
            const auto suffix = member.ends_with(adf_extension)
                                    ? adf_extension
                                    : member.ends_with(epe_extension)
                                          ? epe_extension
                                          : std::string_view{};
            if (suffix.empty()) continue;

            std::string path(member.substr(0, member.size() - suffix.size()));
            path += ".ee";
            const auto hashes = string_hashes(path);
            if (hashes.lookup3 != archive.hash) continue;
            db.kv_put(hashes, path);
            physical_names.try_emplace(hashes.murmur, NamedArchive{hashes, std::move(path)});
            break;
        }
    }

    if (!physical_names.empty()) {
        archives.foreach_file([&](const Archive<u64>::ArchiveEntry &entry) {
            if (entry.parent != 0) return true; // Only installed TAB entries are containers.
            if (const auto it = physical_names.find(entry.key); it != physical_names.end()) {
                db.files_put(it->second.hashes, it->second.path, entry.size, entry.parent);
            }
            return true;
        });
    }
}


// ReSharper disable once CppDFAConstantFunctionResult
bool visit_archive_file(Context &ctx, std::unique_ptr<IO::File> &&file, const Archive<u64>::ArchiveEntry &entry) {
    if (file->get_size() <= 8)
        return true;
    if (entry.i % 500 == 0)
        GLog_Info("[{}/{}]", entry.total, entry.i);

    std::vector<uint8> first_buffer(8);
    file->read_exact<uint8>(first_buffer);
    file->set_position(0);

    auto file_name = find_name(entry.key);
    if (file_name.has_value()) {
        ctx.db.files_put(string_hashes(*file_name), *file_name, file->get_size(), entry.parent);
    }
    const uint32 first_tag = *reinterpret_cast<uint32 *>(first_buffer.data());

    if (!container_tags.empty() && container_tags.contains(first_tag)) {
        uint32 lookup3_hash = container_tags[first_tag];
        if (!ctx.db.get_file(lookup3_hash, AssetDB::HashType::Lookup3).has_value()) {
            GLog_Info("[{}] Found container for tag {} for archive {}|{}", container_tags.size(), first_tag,
                      lookup3_hash, entry.key);
            // Remove so once we found all we wont be looking for it again
            ctx.db.files_put({lookup3_hash, entry.key}, "", file->get_size(), 0);
        }
        container_tags.erase(first_tag);
    }


    if (std::memcmp(first_buffer.data(), ADF_MAGIC, 4) == 0) {
        // GLog_Info("Found ADF: {}", entry.key);
        // visit_adf_file(std::move(file));
    } else if (std::memcmp(first_buffer.data(), GTOC_MAGIC, 4) == 0) {
        GTOCFile toc(*file);
        for (const auto &archive: toc.archives()) {
            for (const auto &[file_index, file_offset]: archive.members) {
                const auto &l_entry = toc.files()[file_index];

                const auto hashes = string_hashes(l_entry.name);
                if (hashes.lookup3 != l_entry.hash) {
                    GLog_Error("Hash mismatch for file: {}", l_entry.name);
                }
                total_files++;
                total_named++;
                ctx.db.kv_put(hashes, l_entry.name);
                if (file_offset == 0) {
                    ctx.db.files_put(hashes, l_entry.name, l_entry.size, 0);
                } else {
                    ctx.db.files_put(hashes, l_entry.name, l_entry.size, entry.key);
                }
            }
        }

        file->set_position(0);
#if GAME==GAME_SECOND_EXTINCTION
        ingest_entity_archives(ctx.db, ctx.archives, toc);
        // if (collector_gtocs.contains(entry.key)) return true;
        // collector_gtocs.try_emplace(entry.key, std::move(toc));
#endif
        auto gtoc_archive = std::make_unique<GTOCArchive>(ctx.archives, std::move(file), entry.key);
#if GAME==GAME_SECOND_EXTINCTION
        pending_gtocs.emplace_back(std::move(gtoc_archive));
#else
        ctx.archives.mount(std::move(gtoc_archive));
#endif
    } else if (std::memcmp(first_buffer.data(), AAF_MAGIC, 4) == 0) {
        // GLog_Info("Found AAF: {}", entry.key);
        AAFArchive aaf_archive(std::move(file));

        std::unique_ptr<IO::File> section_buffer = aaf_archive.get_data();

        auto sarc = std::make_unique<SArchive>(entry.key, std::move(section_buffer));

        u64 total = sarc->entries().size();
        total_files += total;
        total_named += total;
        for (const auto &[i, arc_entry]: sarc->entries() | std::views::enumerate) {
            ctx.db.kv_put(string_hashes(arc_entry.name), arc_entry.name);
            ctx.db.files_put(string_hashes(arc_entry.name), arc_entry.name, arc_entry.size, entry.key);

            if (auto arc_file = sarc->get(arc_entry.hash)) {
                visit_archive_file(ctx, std::move(arc_file), {
                                       arc_entry.hash, entry.key, arc_entry.size, total, static_cast<u64>(i)
                                   });
            }
        }
    } else if (std::memcmp(first_buffer.data() + 4, SARC_MAGIC, 4) == 0) {
        auto sarc = std::make_unique<SArchive>(entry.key, std::move(file));
        GLog_Info("Found bare SARC: {}", entry.key);
        u64 total = sarc->entries().size();
        total_files += total;
        total_named += total;
        for (const auto &[i, arc_entry]: sarc->entries() | std::views::enumerate) {
            ctx.db.kv_put(string_hashes(arc_entry.name), arc_entry.name);
            ctx.db.files_put(string_hashes(arc_entry.name), arc_entry.name, arc_entry.size, entry.key);

            if (auto arc_file = sarc->get(arc_entry.hash)) {
                visit_archive_file(ctx, std::move(arc_file), {
                                       arc_entry.hash, entry.key, arc_entry.size, total, static_cast<u64>(i)
                                   });
            }
        }
    } else if (std::memcmp(first_buffer.data(), RTPC_MAGIC, 4) == 0) {
        // GLog_Info("Found RTPC: {}", entry.key);
        const RuntimeNode root_node = RuntimeNode::RootNode(file);
        visit_ptpc_nodes(ctx, root_node);
        // } else if (std::memcmp(first_buffer.data(), HAVOK_MAGIC, 4) == 0) {
        //     GLog_Info("Found Havok: {}", entry.key);
        //     Havok::Tag::TagFile tag_file(std::move(file));
        //     for (const auto &tf_type: tag_file.types()) {
        //         ctx.db.kv_put(string_hashes(tf_type->name), tf_type->name);
        //         for (const auto &member: tf_type->members) {
        //             ctx.db.kv_put(string_hashes(member.name), member.name);
        //         }
        //     }
    } else if (std::memcmp(first_buffer.data(), AVTX_MAGIC, 4) == 0) {
        if (file_name.has_value()) {
            const auto base_name = std::filesystem::path(*file_name);
            for (int i = 0; i < 10; i++) {
                auto atx_path = base_name;
                atx_path.replace_extension(std::format("atx{}", i));
                if (uint64 key = asset_path_hash(atx_path.string()); ctx.archives.has(key)) {
                    ctx.db.kv_put(string_hashes(atx_path.generic_string()), atx_path.generic_string());
                    ctx.db.files_put(string_hashes(atx_path.generic_string()), atx_path.generic_string(),
                                     file->get_size(), ctx.archives.get_parent_key_for(key));
                }
            }
        }
    }
    return true;
}

void ingest_strings_file(AssetDB &db, const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) return;
    std::ifstream f(path);
    if (!f.is_open()) {
        GLog_Error("Failed to open file: {}", path.string());
        return;
    }
    std::string line;
    GLog_Info("Ingesting strings from file: {}", path.string());
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        db.kv_put(string_hashes(line), line);
    }
}


#if GAME==GAME_SECOND_EXTINCTION
void ingest_location_archives(AssetDB &db, ArchiveManager<u64> &archives) {
    auto world = archives.get(asset_path_hash("locations/world.bin"));
    if (!world) return;

    const auto root = RuntimeNode::RootNode(world);
    constexpr uint32 file_hash = const_hash_string("file");
    constexpr uint32 location_type_hash = const_hash_string("location_type");
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
#endif

int main(int argc, const char *argv[]) {
#if GAME==GAME_GENERATION_ZERO
    const auto db_path = std::filesystem::path("./../hashes.db");
#elif GAME==GAME_RAGE2
    const auto db_path = std::filesystem::path("./../rage2_hashes.db");
#elif GAME==GAME_SECOND_EXTINCTION
    const auto db_path = std::filesystem::path("./../second_extinction_hashes.db");
#elif GAME==GAME_JUST_CAUSE_2
    const auto db_path = std::filesystem::path("./../jc2_hashes.db");
#else
#error "Unsupported game"
#endif
    if (std::filesystem::exists(db_path)) {
        static AssetDB assetdb(db_path);
        AssetDB::set_instance(&assetdb);
    } else {
        static AssetDB assetdb = AssetDB::create_new(db_path);
        AssetDB::set_instance(&assetdb);
    }

    auto &assetdb = *AssetDB::get_instance();

    // init_havok_type_info();
    // init_adf_type_info();

    if (argc < 2) {
        printf("USAGE: %s <path_to_game_root>\n", argv[0]);
        return 0;
    }

#if GAME==GAME_GENERATION_ZERO
    ingest_strings_file(assetdb, "./../strings/generation_zero/file_locations.txt");
    ingest_strings_file(assetdb, "./../strings/generation_zero/filenames.txt");
    ingest_strings_file(assetdb, "./../strings/generation_zero/cross_game.txt");
    ingest_strings_file(assetdb, "./../strings/generation_zero/game_dump_clean.txt");
#elif GAME==GAME_RAGE2
    ingest_strings_file(assetdb, "./../strings/generation_zero/file_locations.txt");
    ingest_strings_file(assetdb, "./../strings/generation_zero/filenames.txt");
    ingest_strings_file(assetdb, "./../strings/generation_zero/cross_game.txt");
    ingest_strings_file(assetdb, "./../strings/generation_zero/game_dump_clean.txt");
    ingest_strings_file(assetdb, "./../strings/rage2/filelist.txt");
    ingest_strings_file(assetdb, "./../strings/rage2/rage2_exe_strings.txt");
#elif GAME==GAME_SECOND_EXTINCTION
    ingest_strings_file(assetdb, "./../strings/second_extinction/filelist.txt");
#elif GAME==GAME_JUST_CAUSE_2
    ingest_strings_file(assetdb, "./../strings/just_cause_2/just_cause_2_exe_strings.txt");
#endif
    ApexAppState app_state(argv[1]);
    app_state.mount_archives();
#if GAME==GAME_SECOND_EXTINCTION
    ingest_location_archives(assetdb, app_state.manager());
#endif

    Context context = {
        .db = *AssetDB::get_instance(),
        .archives = app_state.manager()
    };

    auto mount_gtoc = [&](const std::string_view name) {
        auto &manager = app_state.manager();
        auto buffer = manager.get(name);
        if (!buffer) return;
        context.db.files_put(string_hashes(name), name, buffer->get_size(), 0);
        context.db.kv_put(string_hashes(name), name);
        GTOCFile gtoc{*buffer};
        ingest_entity_archives(context.db, manager, gtoc);
        for (const auto &[hash, tag, members]: gtoc.archives()) {
            bool has_container = false;
            for (const auto &[_, offset]: members) {
                if (offset != 0) {
                    has_container = true;
                    break;
                }
            }
            if (has_container && !context.db.get_file(hash, AssetDB::HashType::Lookup3).has_value()) {
                container_tags.emplace(tag, hash);
            }
        }
        buffer->set_position(0);
        auto gtoc_archive = std::make_unique<GTOCArchive>(manager, std::move(buffer), asset_path_hash(name));
#if GAME==GAME_SECOND_EXTINCTION
        pending_gtocs.emplace_back(std::move(gtoc_archive));
#else
        manager.mount(std::move(gtoc_archive));
#endif
    };
    mount_gtoc("sarc.0.gtoc");
#if GAME==GAME_RAGE2 || GAME==GAME_SECOND_EXTINCTION
    mount_gtoc("resourcesets/expentities.gtoc");
#endif


    if (container_tags.empty()) {
        GLog_Info("No pending containers");
    }


    const auto visit_entry = [&](const Archive<u64>::ArchiveEntry &entry, std::unique_ptr<IO::File> file)-> bool {
        const auto name_opt = context.db.get_file_name(entry.key, AssetDB::HashType::Murmur);
        std::string name;
        if (name_opt.has_value()) {
            name = name_opt.value();
        } else {
            name = std::format("<0x{:08X}>", entry.key);
        }

        // GLog_Info("Processing file: {}", name);
        if (!file) {
            GLog_Error("Failed to read file: {} - {}", entry.key, name);
            return true; // Just skip file
        }
        visit_archive_file(context, std::move(file), entry);
        // if (!assetdb.get_file(entry.key).has_value()) {
        // #if GAME==GAME_GENERATION_ZERO
        //             assetdb.files_put({static_cast<uint32>(entry.key), 0}, "", entry.size, entry.parent);
        // #else
        //             assetdb.files_put({0, entry.key}, "", entry.size, entry.parent);
        // #endif
        // }
        return true;
    };

    app_state.manager().foreach_file([&](const Archive<u64>::ArchiveEntry &entry) {
        total_files += 1;
        const auto name_opt = context.db.get_file_name(entry.key, AssetDB::HashType::Murmur);
        if (name_opt.has_value()) {
            total_named += 1;
        }
        return visit_entry(entry, app_state.manager().get(entry.key));
    });
#if GAME==GAME_SECOND_EXTINCTION
    GLog_Info("Processing pending GTOCs");
    for (auto &pending: pending_gtocs) {
        if (!pending) continue;
        auto *mounted = pending.get();
        const auto gtoc_key = mounted->key();
        GLog_Info("Processing GTOC: {}", mounted->name());
        if (!app_state.manager().is_mounted(gtoc_key)) {
            app_state.manager().mount(std::move(pending));
        }

        const auto &toc = mounted->toc();
        const uint64 total = toc.files().size();
        uint64 i = 0;
        for (const auto &archive: toc.archives()) {
            // Resolve and decompress the container once for all embedded members.
            std::unique_ptr<IO::File> container;
            if (const auto file = context.db.get_file(archive.hash, AssetDB::HashType::Lookup3);
                file && file->murmur != 0) {
                container = app_state.manager().get(file->murmur);
            }
            const auto bytes = container ? container->cbuffer() : std::span<const uint8>{};
            for (const auto &[file_index, offset]: archive.members) {
                ++i;
                if (offset == 0) continue;
                const auto &member = toc.files()[file_index];
                const Archive<u64>::ArchiveEntry entry{
                    asset_path_hash(member.name), gtoc_key, member.size, total, i
                };
                if (!container) {
                    visit_entry(entry, nullptr);
                    continue;
                }
                if (offset > bytes.size() || member.size > bytes.size() - offset) {
                    GLog_Error("GTOC member exceeds container: {}", member.name);
                    visit_entry(entry, nullptr);
                    continue;
                }
                auto data = std::make_unique<IO::MemoryViewFile>(bytes.data() + offset, member.size);
                visit_entry(entry, std::move(data));
            }
        }
    }
#endif


    GLog_Info("Total files: {}", total_files);
    GLog_Info("Total named: {}", total_named);
    GLog_Info("Total unnamed: {}", total_files - total_named);
    GLog_Info("Coverage: {:.4f}%", (static_cast<float32>(total_named) / total_files)*100.f);

    if (!container_tags.empty()) {
        for (const auto &[tag, l3_hash]: container_tags) {
            if (const auto path = context.db.kv_get(l3_hash, AssetDB::HashType::Lookup3)) {
                GLog_Error("Unable to find container for {}|{} (catalog path: {})", tag, l3_hash, *path);
            } else {
                GLog_Error("Unable to find container for {}|{}", tag, l3_hash);
            }
        }
    }

    return 0;
}
