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
#include "havok/havok_codegen.h"
#include "havok/generated/havok_generated.h"
#include "platform/logger.h"
#include "utils/hash_helper.h"
#include "utils/sqlite_wrapper.h"

typedef struct Context {
    assetdb_t *db;
    STI_TypeLibrary *sti_lib;
    Havok_TypeLibrary *havok_lib;
} Context;

bool visit_adf_file(const Context *ctx, MemoryBuffer *mb) {
    ADF adf = {0};
    ADF_from_buffer(&adf, (Buffer *) mb);
    // for (int i = 0; i < adf.header.instance_count; ++i) {
    //     const ADFInstance *instance = DA_at(&adf.instances, i);
    //     void *instance_data = ADF_read_instance(&adf, ctx->sti_lib, instance, mb);
    //     // if (instance->type_hash == STI_TYPE_HASH_TerrainPatch) {
    //     //     TerrainPatch* ter = instance_data;
    //     //     ADF_print_instance(ctx->sti_lib, instance, instance_data, 0);
    //     // }
    //
    //     ADF_free_instance(ctx->sti_lib, instance, instance_data);
    // }
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
        visit_adf_file(ctx, mb);
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

            store_file_parent_sv(entry->hash, StringView_from_cstr(entry->name), self_hash);

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
    else if (memcmp(mb->data, "TAG0", 4) == 0) {
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

bool visit_all_files(const Archive *ar, const ArchiveEntry *ae, void *ctx) {
    MemoryBuffer mb = {0};
    if (String_size(&ae->path)>0) {
        assetdb_kv_put_u32(((Context *) ctx)->db, ae->path_hash, String_cstr(&ae->path));
    }
    const StringView asset_path = find_name32_sv(ae->path_hash);

    store_file_parent_sv(ae->path_hash, sv_is_not_null(asset_path) ? asset_path : StringView_from_string(&ae->path), 0);

    Archive_get_file_by_hash((Archive *) ar, ae->path_hash, &mb);
    String *name = find_name32(ae->path_hash);
    if (name != NULL) {
        printf("Visiting %s from archive %s\n", String_cstr(name), String_cstr(Archive_get_name(ar)));
        String_free(name);
    }
    else
        printf("Visiting file: %08X from archive %s\n", ae->path_hash, String_cstr(Archive_get_name(ar)));
    visit_archive_file(ctx, &mb, ae->path_hash);
    mb.close(&mb);

    return true;
}


int main(int argc, const char *argv[]) {
    if (argc < 2) {
        printf("USAGE: %s <path_to_game_root>\n", argv[0]);
        return 0;
    }

    assetdb_t *db = get_assets_db();

    FILE *f = fopen("./../gz_strings/strings_procmon.txt", "r");
    if (f) {
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            String *tmp = String_new_from_cstr(line);
            uint32 hash = hash_string(tmp);
            assetdb_kv_put_u32(db, hash, String_cstr(tmp));
            String_free(tmp);
        }
        fclose(f);
    }
    f = fopen("./../gz_strings/file_locations.txt", "r");
    if (f) {
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            String *tmp = String_new_from_cstr(line);
            uint32 hash = hash_string(tmp);
            assetdb_kv_put_u32(db, hash, String_cstr(tmp));
            String_free(tmp);
        }
        fclose(f);
    }
    f = fopen("./../gz_strings/filenames.txt", "r");
    if (f) {
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            String *tmp = String_new_from_cstr(line);
            uint32 hash = hash_string(tmp);
            assetdb_kv_put_u32(db, hash, String_cstr(tmp));
            String_free(tmp);
        }
        fclose(f);
    }
    f = fopen("./../gz_strings/cross_game.txt", "r");
    if (f) {
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            String *tmp = String_new_from_cstr(line);
            uint32 hash = hash_string(tmp);
            assetdb_kv_put_u32(db, hash, String_cstr(tmp));
            String_free(tmp);
        }
        fclose(f);
    }

    ArchiveManager archive_manager = {0};
    ArchiveManager_init(&archive_manager);

    String game_root = {0};
    String_from_cstr(&game_root, argv[1]);
    Path_convert_to_wsl(&game_root);
    TabArchives_init(&archive_manager, &game_root);

    STI_TypeLibrary lib = {0};
    STI_TypeLibrary_init(&lib);
    Havok_TypeLibrary havok_lib = {0};
    Havok_TypeLibrary_init(&havok_lib);

    STI_ADF_TYPES_register_functions(&lib);

    Context context = {
        .db = db,
        .sti_lib = &lib,
        .havok_lib = &havok_lib
    };

    ArchiveManager_foreach_file(&archive_manager, visit_all_files, &context);

    STI_TypeLibrary_free(&lib);
    Havok_TypeLibrary_free(&havok_lib);

    assetdb_close(db);

    String_free(&game_root);
    return 0;
}
