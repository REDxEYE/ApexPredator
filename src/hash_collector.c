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
#include "havok/havok_generated.h"
#include "utils/hash_helper.h"
#include "utils/sqlite_wrapper.h"

typedef struct Context {
    kvdb_t *db;
    STI_TypeLibrary *sti_lib;
    Havok_TypeLibrary *havok_lib;
} Context;

bool visit_adf_file(Context *ctx, MemoryBuffer *mb) {
    ADF adf = {0};
    ADF_from_buffer(&adf, (Buffer *) mb, ctx->sti_lib);
    for (int i = 0; i < adf.header.instance_count; ++i) {
        const ADFInstance *instance = DA_at(&adf.instances, i);
        void *instance_data = ADF_read_instance(&adf, ctx->sti_lib, instance, mb);
        // if (instance->type_hash == STI_TYPE_HASH_TerrainPatch) {
        //     TerrainPatch* ter = instance_data;
        //     ADF_print_instance(ctx->sti_lib, instance, instance_data, 0);
        // }

        ADF_free_instance(ctx->sti_lib, instance, instance_data);
    }
    ADF_free(&adf);
    return true;
}

bool visit_ptpc_nodes(RuntimeNode * runtime_node, Context * ctx) {
    for (int i = 0; i < runtime_node->props.count; ++i) {
        const RuntimeProp *prop = DA_at(&runtime_node->props, i);
        if (prop->type == PROP_TYPE_STR) {
            const String* value = &prop->value.string_value;
            kv_put_u32(ctx->db, hash_string(value), String_data(value));
        }
    }
    for (int i = 0; i < runtime_node->children.count; ++i) {
        RuntimeNode* child_node = DA_at(&runtime_node->children, i);
        visit_ptpc_nodes(child_node, ctx);
    }
}

bool visit_archive_file(Context *ctx, MemoryBuffer *mb) {
    // if (memcmp(mb->data, ADF_MAGIC, 4) == 0) {
    //     visit_adf_file(ctx, mb);
    // }
    if (memcmp(mb->data, AAF_MAGIC, 4) == 0) {
        AAFArchive aaf_archive = {0};
        AAFArchive_from_buffer(&aaf_archive, (Buffer *) mb);
        MemoryBuffer *section_buffer = MemoryBuffer_new();
        if (!AAFArchive_get_data(&aaf_archive, section_buffer)) {
            printf("[ERROR]: Failed to get AAF section 0\n");
            return false;
        }

        SArchive *sarc = SArchive_new((Buffer *) section_buffer); // sarc is now owner of buffer

        for (int i = 0; i < sarc->entries.values.count; ++i) {
            const SArcEntry *entry = DA_at(&sarc->entries.values, i);
            kv_put_u32(ctx->db, entry->hash, String_data(&entry->name));
            MemoryBuffer file_mb = {0};
            if (Archive_get_file_by_hash((Archive *) sarc, entry->hash, &file_mb)) {
                visit_archive_file(ctx, &file_mb);
                file_mb.close(&file_mb);
            }
        }
        Archive_free((Archive *) sarc);
        AAFArchive_free(&aaf_archive);
    }else if (memcmp(mb->data, RTPC_MAGIC, 4) == 0) {
        RuntimeNode *root_node = RuntimeContainer_from_buffer((Buffer *) mb);
        if (root_node!=NULL){
            visit_ptpc_nodes(root_node, ctx);
            RuntimeNode_free(root_node);
        }
    }
    return true;
}

bool visit_all_files(const Archive *ar, const ArchiveEntry *ae, void *ctx) {
    MemoryBuffer mb = {0};
    if (ae->path!=NULL) {
        kv_put_u32(((Context *) ctx)->db, ae->path_hash, String_data(ae->path));
    }
    Archive_get_file_by_hash((Archive *) ar, ae->path_hash, &mb);
    printf("Visiting file: %08X from archive %s\n", ae->path_hash, String_data(Archive_get_name(ar)));
    visit_archive_file(ctx, &mb);
    mb.close(&mb);

    return true;
}


int main(int argc, const char *argv[]) {
    if (argc < 2) {
        printf("USAGE: %s <path_to_game_root>\n", argv[0]);
        return 0;
    }

    kvdb_t *db = get_hash_db();

    FILE *f = fopen("./../strings_procmon.txt", "r");
    if (f) {
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            String *tmp = String_new_from_cstr(line);
            uint32 hash = hash_string(tmp);
            kv_put_u32(db, hash, String_data(tmp));
            String_free(tmp);
        }
        fclose(f);
    }
    f = fopen("./../file_locations.txt", "r");
    if (f) {
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            String *tmp = String_new_from_cstr(line);
            uint32 hash = hash_string(tmp);
            kv_put_u32(db, hash, String_data(tmp));
            String_free(tmp);
        }
        fclose(f);
    }

    // const uint64 sti_hash_count = sizeof(STI_ADF_TYPES_hash_strings_hash)/sizeof(uint64);
    // for (int i = 0; i < sti_hash_count; ++i) {
    //     uint32 hash = (uint32)STI_ADF_TYPES_hash_strings_hash[i];
    //     const char* str = STI_ADF_TYPES_hash_strings_string[i];
    //     kv_put_u32(db, hash, str);
    // }
    //
    // typedef struct {
    //     uint32 hash;
    //     const char *name;
    // } KnownHashToName;
    //
    // extern KnownHashToName names2[];
    // extern uint32 names2_len;
    //
    // for (int i = 0; i < names2_len; ++i) {
    //     kv_put_u32(db, names2[i].hash, names2[i].name);
    // }

    ArchiveManager archive_manager = {0};
    ArchiveManager_init(&archive_manager);

    String tmp = {0};
    String game_root = {0};
    String_from_cstr(&tmp, argv[1]);
    Path_convert_to_wsl(&game_root, &tmp);
    TabArchives_init(&archive_manager, &game_root);

    STI_TypeLibrary lib = {0};
    STI_TypeLibrary_init(&lib);
    Havok_TypeLibrary havok_lib = {0};
    HavokTypeLib_init(&havok_lib);

    STI_ADF_TYPES_register_functions(&lib);
    HAVOK_TYPES_register_functions(&havok_lib);

    Context context = {
        .db = db,
        .sti_lib = &lib,
        .havok_lib = &havok_lib
    };

    ArchiveManager_foreach_file(&archive_manager, visit_all_files, &context);

    STI_TypeLibrary_free(&lib);
    HavokTypeLib_free(&havok_lib);

    kv_close(db);

    String_free(&game_root);
    String_free(&tmp);
    return 0;
}
