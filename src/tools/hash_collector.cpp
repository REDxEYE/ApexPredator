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

typedef struct Context {
    assetdb_t *db;
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
            assetdb_kv_put_u32(ctx.db, hash_string(str.c_str()), str.c_str());
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
            assetdb_files_put(ctx.db, arc_entry.hash, arc_entry.name.data(), arc_entry.size, self_hash);
            assetdb_kv_put_u32(ctx.db, arc_entry.hash, arc_entry.name.data());

            if (auto arc_file = sarc->get_file(arc_entry.hash)) {
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
            assetdb_kv_put_u32(ctx.db, hash_string(tf_type->name), tf_type->name.c_str());
            for (const auto & member : tf_type->members) {
                assetdb_kv_put_u32(ctx.db, hash_string(member.name), member.name.c_str());
            }
        }
    }
    return true;
}

void ingest_strings_file(assetdb_t *db, const std::filesystem::path &path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        GLog_Error("Failed to open file: {}", path.string());
        return;
    }
    std::string line;
    while (std::getline(f, line)) {
        const auto hash = hash_string(line);
        assetdb_kv_put_u32(db, hash, line.c_str());
    }
}


int main(int argc, const char *argv[]) {
    mp_init();
    init_havok_type_info();
    init_adf_type_info();

    if (argc < 2) {
        printf("USAGE: %s <path_to_game_root>\n", argv[0]);
        return 0;
    }
    set_db_path("./../hashes.db");

    assetdb_t *db = get_assets_db();
    ingest_strings_file(db, "./../gz_strings/strings_general.txt");
    ingest_strings_file(db, "./../gz_strings/file_locations.txt");
    ingest_strings_file(db, "./../gz_strings/filenames.txt");
    ingest_strings_file(db, "./../gz_strings/cross_game.txt");
    ingest_strings_file(db, "./../gz_strings/game_dump_clean.txt");

    ApexAppState app_state(argv[1]);


    Context context = {
        .db = db,
    };


    app_state.manager().foreach_file([&](const ArchiveEntry &entry)-> bool {
        const auto name = find_name(entry.path_hash).value_or(std::format("<{:08X}>", entry.path_hash));

        // assetdb_files_put(ctx->db, entry->path_hash, StringView_cstr(asset_path), size, 0);

        GLog_Info("Processing file: {}", name);
        auto file = app_state.manager().get_file(entry.path_hash);
        if (!file) {
            GLog_Error("Failed to read file: {}", name);
            return true; // Just skip file
        }
        visit_archive_file(context, std::move(file), entry.path_hash);
        return true;
    });


    assetdb_close(db);
    mp_shutdown();
    return 0;
}
