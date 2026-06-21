// Created by RED on 26.09.2025.
#include <ranges>

#include "apex/hashes.h"
#include "apex/rtpc.h"
#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "apex/adf/adf.h"
#include "apex/adf/generated/adf_types.h"
#include "apex/package/tab_archive.h"
#include "exporter/havok_export.h"
#include "utils/hash_helper.h"
#include "apex/asset_db.h"
#include "redscore/utils/memory_tracker.h"

typedef struct Context {
    AssetDB &db;
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
            ctx.db.kv_put(hash_string(str.c_str()), str.c_str());
        }
    }

    for (const auto &child: runtime_node.children()) {
        visit_ptpc_nodes(ctx, child);
    }
    return true;
}

bool visit_archive_file(Context &ctx, std::unique_ptr<IO::File> &&file, uint32 self_hash) {
    std::vector<uint8> first_buffer(8);
    file->read_exact<uint8>(first_buffer);
    file->set_position(0);

    if (std::memcmp(first_buffer.data(), ADF_MAGIC, 4) == 0) {
        visit_adf_file(std::move(file));
    }
    else if (std::memcmp(first_buffer.data(), AAF_MAGIC, 4) == 0) {
        AAFArchive aaf_archive(std::move(file));

        std::unique_ptr<IO::File> section_buffer = aaf_archive.get_data();

        auto sarc = std::make_unique<SArchive>(self_hash, std::move(section_buffer));

        for (const auto &arc_entry: sarc->entries()) {
            ctx.db.kv_put(arc_entry.hash, arc_entry.name.data());
            ctx.db.files_put(arc_entry.hash, arc_entry.name.data(), arc_entry.size, self_hash);

            if (auto arc_file = sarc->get(arc_entry.hash)) {
                visit_archive_file(ctx, std::move(arc_file), arc_entry.hash);
            }
        }
    }
    else if (std::memcmp(first_buffer.data(), RTPC_MAGIC, 4) == 0) {
        const RuntimeNode root_node = RuntimeNode::RootNode(file);
        visit_ptpc_nodes(ctx, root_node);
    }
    else if (std::memcmp(first_buffer.data(), HAVOK_MAGIC, 4) == 0) {
        Havok::Tag::TagFile tag_file(std::move(file));
        for (const auto & tf_type : tag_file.types()) {
            ctx.db.kv_put(hash_string(tf_type->name), tf_type->name.c_str());
            for (const auto & member : tf_type->members) {
                ctx.db.kv_put(hash_string(member.name), member.name.c_str());
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
    while (std::getline(f, line)) {
        const auto hash = hash_string(line);
        db.kv_put(hash, line.c_str());
    }
}


int main(int argc, const char *argv[]) {
    AssetDB assetdb("./../hashes.db");
    AssetDB::set_instance(&assetdb);

    mp_init();
    init_havok_type_info();
    init_adf_type_info();

    if (argc < 2) {
        printf("USAGE: %s <path_to_game_root>\n", argv[0]);
        return 0;
    }

    ingest_strings_file(assetdb, "./../gz_strings/strings_general.txt");
    ingest_strings_file(assetdb, "./../gz_strings/file_locations.txt");
    ingest_strings_file(assetdb, "./../gz_strings/filenames.txt");
    ingest_strings_file(assetdb, "./../gz_strings/cross_game.txt");
    ingest_strings_file(assetdb, "./../gz_strings/game_dump_clean.txt");

    ApexAppState app_state(argv[1]);


    Context context = {
        .db = assetdb,
    };


    app_state.manager().foreach_file([&](const Archive<u64>::ArchiveEntry &entry)-> bool {
        const auto name = find_name(entry.key).value_or(std::format("<{:08X}>", entry.key));

        
        // assetdb_files_put(ctx->db, entry->path_hash, StringView_cstr(asset_path), size, 0);

        GLog_Info("Processing file: {}", name);
        auto file = app_state.manager().get(entry.key);
        if (!file) {
            GLog_Error("Failed to read file: {}", name);
            return true; // Just skip file
        }
        visit_archive_file(context, std::move(file), entry.key);
        return true;
    });


    mp_shutdown();
    return 0;
}
