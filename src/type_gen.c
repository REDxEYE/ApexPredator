#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "utils/dynamic_array.h"
#include "utils/string.h"
#include "utils/path.h"
#include "utils/buffer/buffer.h"

#include "apex/package/tab.h"
#include "apex/adf/adf.h"
#include "apex/package/tab_archive.h"
#include "apex/adf/builtin_adf.h"

void collect_types(ArchiveManager *archive_manager, STI_TypeLibrary *lib) {
    ADF adf = {0};
    STI_start_type_dump(lib);
    ADF_load_builtin_adf(lib, VEGETATIONINFO_ADF, sizeof(VEGETATIONINFO_ADF));
    ADF_load_builtin_adf(lib, STRINGLOOKUP_ADF_TYPE_MEMORY, sizeof(STRINGLOOKUP_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, XLS_ADF_TYPE_MEMORY, sizeof(XLS_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, LIGHTINFO_ADF_TYPE_MEMORY, sizeof(LIGHTINFO_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, SHADER_FORMAT_LIBRARY_ARR, sizeof(SHADER_FORMAT_LIBRARY_ARR));
    ADF_load_builtin_adf(lib, MODEL_ADF_TYPE_MEMORY, sizeof(MODEL_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, LANDSCAPE_ADF_TYPE_LIBRARY_STREAMPATCH0, sizeof(LANDSCAPE_ADF_TYPE_LIBRARY_STREAMPATCH0));
    ADF_load_builtin_adf(lib, LANDSCAPE_ADF_TYPE_LIBRARY_STREAMPATCH1, sizeof(LANDSCAPE_ADF_TYPE_LIBRARY_STREAMPATCH1));
    ADF_load_builtin_adf(lib, LANDSCAPE_ADF_TYPE_LIBRARY_STREAMPATCH4, sizeof(LANDSCAPE_ADF_TYPE_LIBRARY_STREAMPATCH4));
    ADF_load_builtin_adf(lib, TERRAINOCCLUDERSTYPE_ADF, sizeof(TERRAINOCCLUDERSTYPE_ADF));
    ADF_load_builtin_adf(lib, MODELCOLLECTION_ADF_TYPE_MEMORY, sizeof(MODELCOLLECTION_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, ADF_TYPE_LIBRARY_OCCLUDER, sizeof(ADF_TYPE_LIBRARY_OCCLUDER));
    ADF_load_builtin_adf(lib, NGRAPHSCRIPT_ADF_TYPE_MEMORY, sizeof(NGRAPHSCRIPT_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, ROAD_GRAPH_TYPE_LIBRARY, sizeof(ROAD_GRAPH_TYPE_LIBRARY));
    ADF_load_builtin_adf(lib, ROUTES_TYPE_LIBRARY, sizeof(ROUTES_TYPE_LIBRARY));
    ADF_load_builtin_adf(lib, GAME_DATA_COLLECTION_ADF_TYPELIBRARY, sizeof(GAME_DATA_COLLECTION_ADF_TYPELIBRARY));
    ADF_load_builtin_adf(lib, ANIMAL_BITMAP_DATA_ADF_TYPE_LIBRARY, sizeof(ANIMAL_BITMAP_DATA_ADF_TYPE_LIBRARY));
    ADF_load_builtin_adf(lib, DOWNLOAD_CACHE_DATA_ADF_TYPELIBRARY, sizeof(DOWNLOAD_CACHE_DATA_ADF_TYPELIBRARY));
    ADF_load_builtin_adf(lib, SAVE_GAME_DATA_ADF_TYPELIBRARY, sizeof(SAVE_GAME_DATA_ADF_TYPELIBRARY));
    ADF_load_builtin_adf(lib, RAGDOLL_SETTINGS_ADF_TYPE_MEMORY, sizeof(RAGDOLL_SETTINGS_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, FORCE_PULSE_TUNING_ADF_TYPE_MEMORY, sizeof(FORCE_PULSE_TUNING_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, SCOPE_TUNING_ADF_TYPE_MEMORY, sizeof(SCOPE_TUNING_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, ARCTUNE_ADF_TYPE_MEMORY, sizeof(ARCTUNE_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, AI_TUNING_ADF_TYPE_MEMORY, sizeof(AI_TUNING_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, SPLINES_ADF_TYPE_MEMORY_1, sizeof(SPLINES_ADF_TYPE_MEMORY_1));
    ADF_load_builtin_adf(lib, AISYS_TUNING_ADF_TYPE_MEMORY, sizeof(AISYS_TUNING_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, TERRAINSYSTEMTYPES_ADF_TYPE_LIBRARY_INFO,
                         sizeof(TERRAINSYSTEMTYPES_ADF_TYPE_LIBRARY_INFO));
    ADF_load_builtin_adf(lib, TERRAINSYSTEM_ADF_TYPE_LIBRARY_INFO, sizeof(TERRAINSYSTEM_ADF_TYPE_LIBRARY_INFO));
    ADF_load_builtin_adf(lib, MODEL_ADF_TYPE_MEMORY_0, sizeof(MODEL_ADF_TYPE_MEMORY_0));
    ADF_load_builtin_adf(lib, RUNTIME_EFFECT_LIBRARY, sizeof(RUNTIME_EFFECT_LIBRARY));
    ADF_load_builtin_adf(lib, PFX_ADF_TYPE_MEMORY, sizeof(PFX_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, GRAPH_ADF_TYPE_MEMORY, sizeof(GRAPH_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, EXP_PROB_ADF_TYPE_MEMORY, sizeof(EXP_PROB_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, VEHICLEPHYSICSSETTINGS_ADF_TYPE_MEMORY, sizeof(VEHICLEPHYSICSSETTINGS_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, VEHICLEPIPELINE_ADF_TYPE_MEMORY, sizeof(VEHICLEPIPELINE_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, PERCEPTION_ADF_TYPE_MEMORY, sizeof(PERCEPTION_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, WATERTUNE_ADF_TYPE_MEMORY, sizeof(WATERTUNE_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, PLAYER_SETTINGS_ADF_TYPE_MEMORY, sizeof(PLAYER_SETTINGS_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, AMMO_TUNING_ADF_TYPE_MEMORY, sizeof(AMMO_TUNING_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, CUSTOM_MOVEMENT_TUNING_ADF_TYPE_MEMORY, sizeof(CUSTOM_MOVEMENT_TUNING_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, CAMERA_TUNING_ADF_TYPE_MEMORY, sizeof(CAMERA_TUNING_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, CHAINTUNE_ADF_TYPE_MEMORY, sizeof(CHAINTUNE_ADF_TYPE_MEMORY));
    ADF_load_builtin_adf(lib, HP_MISSIONS_ADF_TYPE_MEMORY, sizeof(HP_MISSIONS_ADF_TYPE_MEMORY));

    DynamicArray_ArchiveEntry all_entries = {0};
    ArchiveManager_get_all_entries(archive_manager, &all_entries);
    for (int i = 0; i < all_entries.count; ++i) {
        ArchiveEntry *entry = DA_at(&all_entries, i);
        MemoryBuffer mb = {0};
        if (!ArchiveManager_get_file_by_hash(archive_manager, entry->path_hash, &mb)) {
            printf("File not found\n");
            return;
        }

        if (mb.data[0] == ' ' && mb.data[1] == 'F' && mb.data[2] == 'D' && mb.data[3] == 'A') {
            ADF_from_buffer(&adf, (Buffer *) &mb, lib);
        } else if (mb.data[0] == 'A' && mb.data[1] == 'A' && mb.data[2] == 'F' && mb.data[3] == '\0') {
            AAFArchive aaf_archive = {0};
            AAFArchive_from_buffer(&aaf_archive, (Buffer *) &mb);
            MemoryBuffer *section_buffer = MemoryBuffer_new();
            if (!AAFArchive_get_data(&aaf_archive, section_buffer)) {
                printf("[ERROR]: Failed to get AAF section %i\n", i);
                return;
            }
            if (section_buffer->data[4] == 'S' && section_buffer->data[5] == 'A' &&
                section_buffer->data[6] == 'R' && section_buffer->data[7] == 'C') {
                SArchive *sarc = SArchive_new((Buffer *) section_buffer); // sarc is now owner of buffer
                DynamicArray_ArchiveEntry sarc_entries = {0};
                DA_init(&sarc_entries, ArchiveEntry, 16);
                Archive_get_all_entries((Archive *) sarc, &sarc_entries);
                for (int j = 0; j < sarc_entries.count; ++j) {
                    ArchiveEntry *aaf_entry = DA_at(&sarc_entries, j);
                    MemoryBuffer *tmp = MemoryBuffer_new();
                    if (Archive_get_file((Archive*)sarc, aaf_entry->path, tmp)) {
                        if (tmp->data[0] == ' ' && tmp->data[1] == 'F' && tmp->data[2] == 'D' && tmp->data[3] == 'A') {
                            ADF_from_buffer(&adf, (Buffer *) tmp, lib);
                            ADF_free(&adf);
                        }
                    }
                    tmp->close(tmp);
                }
                DA_free(&sarc_entries);
                Archive_free((Archive*)sarc);
            }
            AAFArchive_free(&aaf_archive);
        }
        mb.close(&mb);
    }

    String namespace = {0};
    String_from_cstr(&namespace, "ADF_TYPES");

    String header_path_tmp = {0};
    String header_path = {0};

    String_from_cstr(&header_path_tmp, "D:/projects/cpp/ApexPredator/include/apex/adf/adf_types.h");
    Path_convert_to_wsl(&header_path, &header_path_tmp);
    String_free(&header_path_tmp);
    FILE *header_file = fopen(String_data(&header_path), "w");

    String_from_cstr(&header_path_tmp, "D:/projects/cpp/ApexPredator/src/apex/adf/adf_types.c");
    Path_convert_to_wsl(&header_path, &header_path_tmp);
    String_free(&header_path_tmp);
    FILE *impl_file = fopen(String_data(&header_path), "w");

    String_free(&header_path);

    String header_rel_path = {};
    String_append_cstr(&header_rel_path, "apex/adf/adf_types.h");
    STI_TypeLibrary_generate_types(lib, &namespace, header_file, &header_rel_path, impl_file);

    String_free(&header_rel_path);
    String_free(&namespace);
    fclose(header_file);
    fclose(impl_file);
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        printf("USAGE: %s <path_to_game_root>\n", argv[0]);
        return 0;
    }

    STI_TypeLibrary lib = {0};
    String tmp = {0};
    String game_root = {0};
    STI_TypeLibrary_init(&lib);

    ArchiveManager manager = {0};
    ArchiveManager_init(&manager);

    String_from_cstr(&tmp, argv[1]);
    Path_convert_to_wsl(&game_root, &tmp);
    TabArchives_init(&manager, &game_root);
    collect_types(&manager, &lib);

    ArchiveManager_free(&manager);
    STI_TypeLibrary_free(&lib);
    String_free(&game_root);
    String_free(&tmp);
    return 0;
}
