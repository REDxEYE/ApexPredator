#include "utils/memory_tracker.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#else
#include <unistd.h>
#endif

#include <stdio.h>

#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "utils/string.h"
#include "utils/path.h"
#include "apex/adf/adf.h"
#include "apex/package/tab_archive.h"
#include "apex/adf/builtin_adf.h"
#include "apex/adf/sti.h"
#include "platform/logger.h"

void import_adf(const ADF *adf, STI_TypeLibrary *lib) {
    for (uint32 i = 0; i < DA_size(&adf->types); ++i) {
        const ADFType *adf_type = DA_at(&adf->types, i);
        STI_TypeLibrary_register_adf_type(lib, adf, adf_type);
    }
}

void import_builtin_adf(const uint8 *data, const int64 size, STI_TypeLibrary *lib) {
    ADF *b_adf = ADF_load_builtin_adf(data, size);
    import_adf(b_adf, lib);
    ADF_free(b_adf);
    mp_free(b_adf);
}


void collect_types(ArchiveManager *archive_manager, STI_TypeLibrary *lib) {
    STI_start_type_dump(lib);
    // @formatter:off
    import_builtin_adf(VEGETATIONINFO_ADF, sizeof(VEGETATIONINFO_ADF),lib);
    import_builtin_adf(STRINGLOOKUP_ADF_TYPE_MEMORY, sizeof(STRINGLOOKUP_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(XLS_ADF_TYPE_MEMORY, sizeof(XLS_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(LIGHTINFO_ADF_TYPE_MEMORY, sizeof(LIGHTINFO_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(SHADER_FORMAT_LIBRARY_ARR, sizeof(SHADER_FORMAT_LIBRARY_ARR),lib);
    import_builtin_adf(MODEL_ADF_TYPE_MEMORY, sizeof(MODEL_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(LANDSCAPE_ADF_TYPE_LIBRARY_STREAMPATCH0, sizeof(LANDSCAPE_ADF_TYPE_LIBRARY_STREAMPATCH0),lib);
    import_builtin_adf(LANDSCAPE_ADF_TYPE_LIBRARY_STREAMPATCH1, sizeof(LANDSCAPE_ADF_TYPE_LIBRARY_STREAMPATCH1),lib);
    import_builtin_adf(LANDSCAPE_ADF_TYPE_LIBRARY_STREAMPATCH4, sizeof(LANDSCAPE_ADF_TYPE_LIBRARY_STREAMPATCH4),lib);
    import_builtin_adf(TERRAINOCCLUDERSTYPE_ADF, sizeof(TERRAINOCCLUDERSTYPE_ADF),lib);
    import_builtin_adf(MODELCOLLECTION_ADF_TYPE_MEMORY, sizeof(MODELCOLLECTION_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(ADF_TYPE_LIBRARY_OCCLUDER, sizeof(ADF_TYPE_LIBRARY_OCCLUDER),lib);
    import_builtin_adf(NGRAPHSCRIPT_ADF_TYPE_MEMORY, sizeof(NGRAPHSCRIPT_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(ROAD_GRAPH_TYPE_LIBRARY, sizeof(ROAD_GRAPH_TYPE_LIBRARY),lib);
    import_builtin_adf(ROUTES_TYPE_LIBRARY, sizeof(ROUTES_TYPE_LIBRARY),lib);
    import_builtin_adf(GAME_DATA_COLLECTION_ADF_TYPELIBRARY, sizeof(GAME_DATA_COLLECTION_ADF_TYPELIBRARY),lib);
    import_builtin_adf(ANIMAL_BITMAP_DATA_ADF_TYPE_LIBRARY, sizeof(ANIMAL_BITMAP_DATA_ADF_TYPE_LIBRARY),lib);
    import_builtin_adf(DOWNLOAD_CACHE_DATA_ADF_TYPELIBRARY, sizeof(DOWNLOAD_CACHE_DATA_ADF_TYPELIBRARY),lib);
    import_builtin_adf(SAVE_GAME_DATA_ADF_TYPELIBRARY, sizeof(SAVE_GAME_DATA_ADF_TYPELIBRARY),lib);
    import_builtin_adf(RAGDOLL_SETTINGS_ADF_TYPE_MEMORY, sizeof(RAGDOLL_SETTINGS_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(FORCE_PULSE_TUNING_ADF_TYPE_MEMORY, sizeof(FORCE_PULSE_TUNING_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(SCOPE_TUNING_ADF_TYPE_MEMORY, sizeof(SCOPE_TUNING_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(ARCTUNE_ADF_TYPE_MEMORY, sizeof(ARCTUNE_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(AI_TUNING_ADF_TYPE_MEMORY, sizeof(AI_TUNING_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(SPLINES_ADF_TYPE_MEMORY_1, sizeof(SPLINES_ADF_TYPE_MEMORY_1),lib);
    import_builtin_adf(AISYS_TUNING_ADF_TYPE_MEMORY, sizeof(AISYS_TUNING_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(TERRAINSYSTEMTYPES_ADF_TYPE_LIBRARY_INFO,sizeof(TERRAINSYSTEMTYPES_ADF_TYPE_LIBRARY_INFO),lib);
    import_builtin_adf(TERRAINSYSTEM_ADF_TYPE_LIBRARY_INFO, sizeof(TERRAINSYSTEM_ADF_TYPE_LIBRARY_INFO),lib);
    import_builtin_adf(MODEL_ADF_TYPE_MEMORY_0, sizeof(MODEL_ADF_TYPE_MEMORY_0),lib);
    import_builtin_adf(RUNTIME_EFFECT_LIBRARY, sizeof(RUNTIME_EFFECT_LIBRARY),lib);
    import_builtin_adf(PFX_ADF_TYPE_MEMORY, sizeof(PFX_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(GRAPH_ADF_TYPE_MEMORY, sizeof(GRAPH_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(EXP_PROB_ADF_TYPE_MEMORY, sizeof(EXP_PROB_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(VEHICLEPHYSICSSETTINGS_ADF_TYPE_MEMORY, sizeof(VEHICLEPHYSICSSETTINGS_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(VEHICLEPIPELINE_ADF_TYPE_MEMORY, sizeof(VEHICLEPIPELINE_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(PERCEPTION_ADF_TYPE_MEMORY, sizeof(PERCEPTION_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(WATERTUNE_ADF_TYPE_MEMORY, sizeof(WATERTUNE_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(PLAYER_SETTINGS_ADF_TYPE_MEMORY, sizeof(PLAYER_SETTINGS_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(AMMO_TUNING_ADF_TYPE_MEMORY, sizeof(AMMO_TUNING_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(CUSTOM_MOVEMENT_TUNING_ADF_TYPE_MEMORY, sizeof(CUSTOM_MOVEMENT_TUNING_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(CAMERA_TUNING_ADF_TYPE_MEMORY, sizeof(CAMERA_TUNING_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(CHAINTUNE_ADF_TYPE_MEMORY, sizeof(CHAINTUNE_ADF_TYPE_MEMORY),lib);
    import_builtin_adf(HP_MISSIONS_ADF_TYPE_MEMORY, sizeof(HP_MISSIONS_ADF_TYPE_MEMORY),lib);
    // @formatter:on

    ADF adf = {0};
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
            ADF_from_buffer(&adf, (Buffer *) &mb);
            import_adf(&adf, lib);
            ADF_free(&adf);
        }
        else if (mb.data[0] == 'A' && mb.data[1] == 'A' && mb.data[2] == 'F' && mb.data[3] == '\0') {
            static AAFArchive aaf_archive = {0};
            AAFArchive_from_buffer(&aaf_archive, (Buffer *) &mb);
            MemoryBuffer *section_buffer = MemoryBuffer_new();
            if (!AAFArchive_get_data(&aaf_archive, section_buffer)) {
                GLog_Error("Failed to get AAF section %i", i);
                return;
            }
            if (section_buffer->data[4] == 'S' && section_buffer->data[5] == 'A' &&
                section_buffer->data[6] == 'R' && section_buffer->data[7] == 'C') {
                static SArchive sarc = {0};
                SArchive_init(&sarc, (Buffer *) section_buffer, entry->path_hash); // sarc is now owner of buffer
                DynamicArray_ArchiveEntry sarc_entries = {0};
                DA_init(&sarc_entries, ArchiveEntry, 16);
                Archive_get_all_entries((Archive *) &sarc, &sarc_entries);
                for (int j = 0; j < sarc_entries.count; ++j) {
                    ArchiveEntry *aaf_entry = DA_at(&sarc_entries, j);
                    MemoryBuffer *tmp = MemoryBuffer_new();
                    if (Archive_get_file((Archive *) &sarc, aaf_entry->path, tmp)) {
                        if (tmp->data[0] == ' ' && tmp->data[1] == 'F' && tmp->data[2] == 'D' && tmp->data[3] == 'A') {
                            ADF_from_buffer(&adf, (Buffer *) tmp);
                            import_adf(&adf, lib);
                            ADF_free(&adf);
                        }
                    }
                    Buffer_close((Buffer *) tmp);
                }
                DA_free(&sarc_entries);
                Archive_free((Archive *) &sarc);
            }
            AAFArchive_free(&aaf_archive);
        }
        Buffer_close((Buffer *) &mb);
    }

    String namespace = {0};
    String_from_cstr(&namespace, "ADF_TYPES");

    String header_path = {0};

    String_from_cstr(&header_path, "D:/projects/cpp/ApexPredator/include/apex/adf/adf_types.h");
    Path_convert_to_wsl(&header_path);
    FILE *header_file = fopen(String_cstr(&header_path), "w");
    String_free(&header_path);

    String_from_cstr(&header_path, "D:/projects/cpp/ApexPredator/src/apex/adf/adf_types.c");
    Path_convert_to_wsl(&header_path);
    FILE *impl_file = fopen(String_cstr(&header_path), "w");
    String_free(&header_path);

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
    mp_init();
    if (argc < 2) {
        printf("USAGE: %s <path_to_game_root>\n", argv[0]);
        return 0;
    }

    //     while (!TracyCIsConnected) {
    // #ifdef _WIN32
    //         Sleep(100); /* Windows */
    // #else
    //         usleep(10000);
    // #endif
    //         printf("\rWaiting for tracy;");
    //     }
    //     printf("\n");

    STI_TypeLibrary lib = {0};
    String game_root = {0};
    STI_TypeLibrary_init(&lib);

    ArchiveManager manager = {0};
    ArchiveManager_init(&manager);

    String_from_cstr(&game_root, argv[1]);
    Path_convert_to_wsl(&game_root);
    TabArchives_init(&manager, &game_root);
    collect_types(&manager, &lib);

    ArchiveManager_free(&manager);
    STI_TypeLibrary_free(&lib);
    String_free(&game_root);
    return 0;
}
