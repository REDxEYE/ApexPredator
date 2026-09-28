// Created by RED on 26.09.2025.
#include <ranges>
#include <unordered_set>

#include "games.hpp"

#include "apex/hashes.h"
#include "apex/gtoc.h"
#include "apex/rtpc.h"
#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "apex/adf/adf.h"
#include "apex/adf/generated/adf_types.h"
#include "apex/package/tab_archive.h"
#include "exporter/havok_export.h"
#include "utils/hash_helper.h"
#include "apex/asset_db.h"
#include "apex/avtx.h"

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

bool visit_archive_file(Context &ctx, std::unique_ptr<IO::File> &&file, uint64 self_hash, uint64 parent_hash = 0) {
    std::vector<uint8> first_buffer(8);
    file->read_exact<uint8>(first_buffer);
    file->set_position(0);

    auto file_name = find_name(self_hash);
    if (file_name.has_value()) {
        ctx.db.files_put(string_hashes(*file_name), *file_name, file->get_size(), parent_hash);
    }


    if (std::memcmp(first_buffer.data(), ADF_MAGIC, 4) == 0) {
        GLog_Info("Found ADF: {}", self_hash);
        visit_adf_file(std::move(file));
    } else if (std::memcmp(first_buffer.data(), GTOC_MAGIC, 4) == 0) {
        GLog_Info("Found GTOC/STOC: {}", self_hash);
        const GTOCFile toc(*file);

        for (const auto &archive: toc.archives()) {
            for (const auto &[file_index, _]: archive.members) {
                const auto &entry = toc.files()[file_index];
                const auto hashes = string_hashes(entry.name);
                if (hashes.lookup3 != entry.hash) {
                    GLog_Error("Hash mismatch for file: {}", entry.name);
                }
                ctx.db.kv_put(hashes, entry.name);
                ctx.db.files_put(hashes, entry.name, entry.size, self_hash);
            }
        }

        file->set_position(0);
        GTOCArchive archive(ctx.archives, std::move(file), self_hash);
        archive.foreach_file([&archive, &ctx, self_hash](const Archive<unsigned long>::ArchiveEntry &archive_entry) {
            auto inner_file = archive.get(archive_entry.key);
            if (!inner_file) {
                return true;
            }
            return visit_archive_file(ctx, std::move(inner_file), archive_entry.key, self_hash);
        });

    } else if (std::memcmp(first_buffer.data(), AAF_MAGIC, 4) == 0) {
        GLog_Info("Found AAF: {}", self_hash);
        AAFArchive aaf_archive(std::move(file));

        std::unique_ptr<IO::File> section_buffer = aaf_archive.get_data();

        auto sarc = std::make_unique<SArchive>(self_hash, std::move(section_buffer));

        for (const auto &arc_entry: sarc->entries()) {
            ctx.db.kv_put(string_hashes(arc_entry.name), arc_entry.name);
            ctx.db.files_put(string_hashes(arc_entry.name), arc_entry.name, arc_entry.size, self_hash);

            if (auto arc_file = sarc->get(arc_entry.hash)) {
                visit_archive_file(ctx, std::move(arc_file), arc_entry.hash, self_hash);
            }
        }
    } else if (std::memcmp(first_buffer.data(), RTPC_MAGIC, 4) == 0) {
        GLog_Info("Found RTPC: {}", self_hash);
        const RuntimeNode root_node = RuntimeNode::RootNode(file);
        visit_ptpc_nodes(ctx, root_node);
    } else if (std::memcmp(first_buffer.data(), HAVOK_MAGIC, 4) == 0) {
        GLog_Info("Found Havok: {}", self_hash);
        Havok::Tag::TagFile tag_file(std::move(file));
        for (const auto &tf_type: tag_file.types()) {
            ctx.db.kv_put(string_hashes(tf_type->name), tf_type->name);
            for (const auto &member: tf_type->members) {
                ctx.db.kv_put(string_hashes(member.name), member.name);
            }
        }
    } else if (std::memcmp(first_buffer.data(), AVTX_MAGIC, 4) == 0) {
        if (file_name.has_value()) {
            const auto base_name = std::filesystem::path(*file_name);
            for (int i = 0; i < 10; i++) {
                auto atx_path = base_name;
                atx_path.replace_extension(std::format("atx{}", i));
                if (uint64 key = asset_path_hash(atx_path.string()); ctx.archives.has(key)) {
                    ctx.db.kv_put(string_hashes(atx_path.generic_string()), atx_path.generic_string());
                    ctx.db.files_put(string_hashes(atx_path.generic_string()), atx_path.generic_string(),
                                     file->get_size(), 0);
                }
            }
        }
    }
    return true;
}

void ingest_strings_file(AssetDB &db, const std::filesystem::path &path) {
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


int main(int argc, const char *argv[]) {
#if GAME==GAME_GENERATION_ZERO
    const auto db_path = std::filesystem::path("./../hashes.db");
#elif GAME==GAME_RAGE2
    const auto db_path = std::filesystem::path("./../rage2_hashes.db");
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

    init_havok_type_info();
    init_adf_type_info();

    if (argc < 2) {
        printf("USAGE: %s <path_to_game_root>\n", argv[0]);
        return 0;
    }

#if GAME==GAME_GENERATION_ZERO
    // ingest_strings_file(assetdb, "./../gz_strings/strings_general.txt");
    ingest_strings_file(assetdb, "./../gz_strings/file_locations.txt");
    ingest_strings_file(assetdb, "./../gz_strings/filenames.txt");
    ingest_strings_file(assetdb, "./../gz_strings/cross_game.txt");
    ingest_strings_file(assetdb, "./../gz_strings/game_dump_clean.txt");
#elif GAME==GAME_RAGE2
    ingest_strings_file(assetdb, "./../gz_strings/file_locations.txt");
    ingest_strings_file(assetdb, "./../gz_strings/filenames.txt");
    ingest_strings_file(assetdb, "./../gz_strings/cross_game.txt");
    ingest_strings_file(assetdb, "./../gz_strings/game_dump_clean.txt");
    ingest_strings_file(assetdb, "./../rage_strings/filelist.txt");
    ingest_strings_file(assetdb, "./../rage_strings/rage2_exe_strings.txt");
#endif
    ApexAppState app_state(argv[1]);
    app_state.mount_archives();

    auto mount_gtoc = [&app_state](const std::string_view name) {
        auto buffer = app_state.manager().get(name);
        if (!buffer) {
            return;
        }
        auto gtoc_archive = std::make_unique<
            GTOCArchive>(app_state.manager(), std::move(buffer), asset_path_hash(name));
        app_state.manager().mount(std::move(gtoc_archive));
    };
    // mount_gtoc("sarc.0.gtoc");
    // mount_gtoc("resourcesets/expentities.gtoc");

    Context context = {
        .db = *AssetDB::get_instance(),
        .archives = app_state.manager()
    };

    static uint64 total_files = 0;
    static uint64 total_named = 0;
    app_state.manager().foreach_file([&](const Archive<u64>::ArchiveEntry &entry)-> bool {
        total_files += 1;
        auto name_opt = find_name(entry.key);
        if (name_opt.has_value()) {
            total_named += 1;
        }

        const auto name = name_opt.value_or(std::format("<{:08X}>", entry.key));
        GLog_Info("Processing file: {}", name);
        auto file = app_state.manager().get(entry.key);
        if (!file) {
            GLog_Error("Failed to read file: {} - {}", entry.key, name);
            return true; // Just skip file
        }
        visit_archive_file(context, std::move(file), entry.key);
        return true;
    });


    GLog_Info("Total files: {}", total_files);
    GLog_Info("Total named: {}", total_named);
    GLog_Info("Total unnamed: {}", total_files - total_named);
    GLog_Info("Coverage: {:.4f}", (static_cast<float32>(total_named) / total_files)*100.f);
    return 0;
}
