// Created by RED on 26.09.2025.
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "apex/hashes.h"
#include "apex/rtpc.h"
#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "utils/dynamic_array.h"
#include "utils/string.h"
#include "utils/path.h"
#include "apex/adf/adf.h"
#include "apex/adf/adf_types.h"
#include "apex/package/tab_archive.h"
#include "exporter/havok_export.h"
#include "havok/havok_codegen.h"
#include "havok/generated/havok_generated.h"
#include "platform/logger.h"
#include "utils/hash_helper.h"
#include "utils/sqlite_wrapper.h"

typedef struct Context {
    assetdb_t *db;
    Havok_TypeLibrary *havok_lib;
} Context;

bool visit_adf_file(MemoryBuffer *mb) {
    ADF adf = {0};
    ADF_from_buffer(&adf, (Buffer *) mb);
    ADF_free(&adf);
    return true;
}

bool visit_ptpc_nodes(Context *ctx, RuntimeNode *runtime_node) {
    for (int i = 0; i < runtime_node->props.count; ++i) {
        const RuntimeProp *prop = DA_at(&runtime_node->props, i);
        if (prop->type == PROP_TYPE_STR) {
            const String *value = &prop->value.string_value;
            assetdb_kv_put_u32(ctx->db, hash_string(value), String_cstr(value));
        }
    }
    for (int i = 0; i < runtime_node->children.count; ++i) {
        RuntimeNode *child_node = DA_at(&runtime_node->children, i);
        visit_ptpc_nodes(ctx, child_node);
    }
    return true;
}

bool visit_archive_file(Context *ctx, MemoryBuffer *mb, uint32 self_hash) {
    if (memcmp(mb->data, ADF_MAGIC, 4) == 0) {
        visit_adf_file(mb);
    }
    else if (memcmp(mb->data, AAF_MAGIC, 4) == 0) {
        AAFArchive aaf_archive = {0};
        AAFArchive_from_buffer(&aaf_archive, (Buffer *) mb);
        MemoryBuffer *section_buffer = MemoryBuffer_new();
        if (!AAFArchive_get_data(&aaf_archive, section_buffer)) {
            GLog_Error("Failed to get AAF section 0");
            return false;
        }

        SArchive *sarc = SArchive_new((Buffer *) section_buffer, self_hash); // sarc is now owner of buffer

        for (int i = 0; i < sarc->entries.values.count; ++i) {
            const SArcEntry *entry = DA_at(&sarc->entries.values, i);

            assetdb_files_put(ctx->db, entry->hash, entry->name, entry->size, self_hash);
            assetdb_kv_put_u32(ctx->db, entry->hash, entry->name);

            MemoryBuffer file_mb = {0};
            if (Archive_get_file_by_hash((Archive *) sarc, entry->hash, &file_mb)) {
                visit_archive_file(ctx, &file_mb, entry->hash);
                file_mb.close(&file_mb);
            }
        }
        Archive_free((Archive *) sarc);
        AAFArchive_free(&aaf_archive);
    }
    else if (memcmp(mb->data, RTPC_MAGIC, 4) == 0) {
        RuntimeNode *root_node = RuntimeContainer_from_buffer((Buffer *) mb);
        if (root_node != NULL) {
            visit_ptpc_nodes(ctx, root_node);
            RuntimeNode_free(root_node);
        }
    }
    else if (memcmp(mb->data, HAVOK_MAGIC, 4) == 0) {
        TagFile tag_file = {0};
        TagFile_from_buffer(&tag_file, (Buffer *) &mb);
        for (int i = 0; i < tag_file.types.count; ++i) {
            const HKTagType *tf_type = DA_at(&tag_file.types, i);
            assetdb_kv_put_u32(ctx->db, hash_string(&tf_type->name), String_cstr(&tf_type->name));
            for (int j = 0; j < tf_type->members.count; ++j) {
                const HKTagTypeMember *tf_member = DA_at(&tf_type->members, j);
                assetdb_kv_put_u32(ctx->db, hash_string(&tf_member->name), String_cstr(&tf_member->name));
            }
        }
        TagFile_free(&tag_file);
    }
    return true;
}

bool visit_all_files(const Archive *ar, const ArchiveEntry *entry, Context *ctx) {
    MemoryBuffer mb = {0};
    const StringView asset_path = String_size(&entry->path)>0? StringView_from_string(&entry->path) : find_name32_sv(entry->path_hash);
    if (sv_is_not_null(asset_path)) {
        assetdb_kv_put_u32(ctx->db, entry->path_hash, StringView_cstr(asset_path));
    }
    const uint64 size = entry->size;

    assetdb_files_put(ctx->db, entry->path_hash, StringView_cstr(asset_path), size, 0);

    Archive_get_file_by_hash((Archive *) ar, entry->path_hash, &mb);
    // if (sv_is_not_null(asset_path)) {
    //     printf("Visiting %s from archive %s\n", StringView_cstr(asset_path), String_cstr(Archive_get_name(ar)));
    // }
    // else
    //     printf("Visiting file: %08X from archive %s\n", entry->path_hash, String_cstr(Archive_get_name(ar)));
    visit_archive_file(ctx, &mb, entry->path_hash);
    mb.close(&mb);

    return true;
}

void ingest_strings_file(assetdb_t* db, const char* path) {
    FILE* f;
    const errno_t err = fopen_s(&f, path, "r");
    if (err != 0) {
        GLog_Error("Failed to open strings file: %s", path);
        return;
    }
    if (f) {
        static char line[1024];
        while (fgets(line, sizeof(line), f)) {
            const size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            String *tmp = String_new_from_cstr(line);
            const uint32 hash = hash_string(tmp);
            assetdb_kv_put_u32(db, hash, String_cstr(tmp));
            String_free(tmp);
        }
        fclose(f);
    }
}


int main(int argc, const char *argv[]) {
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


    ArchiveManager archive_manager = {0};
    ArchiveManager_init(&archive_manager);

    String game_root = {0};
    String_from_cstr(&game_root, argv[1]);
    Path_convert_to_wsl(&game_root);
    TabArchives_init(&archive_manager, &game_root);

    Havok_TypeLibrary havok_lib = {0};
    Havok_TypeLibrary_init(&havok_lib);

    STI_ADF_TYPES_register_functions();

    Context context = {
        .db = db,
        .havok_lib = &havok_lib
    };

    for (uint32 i = 0; i < archive_manager.archives.count; ++i) {
        const Archive *ar = archive_manager.archives.items[i];
        GLog_Info("Processing archive: %s (%u/%u)", String_cstr(Archive_get_name(ar)), i + 1, archive_manager.archives.count);
        if (!Archive_foreach_file(ar, (foreach_callback)visit_all_files, (void*)&context)) {
            break;
        }
    }

    Havok_TypeLibrary_free(&havok_lib);

    assetdb_close(db);

    String_free(&game_root);
    return 0;
}
