#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/dynamic_array.h"
#include "utils/string.h"
#include "utils/path.h"
#include "utils/buffer/buffer.h"

#include "apex/package/tab.h"
#include "apex/adf/adf.h"
#include "apex/package/tab_archive.h"
#include "apex/adf/builtin_adf.h"


typedef struct {
    uint32 hash;
    String archive_name;
    String file_name;
} TabToArch;

DYNAMIC_ARRAY_STRUCT(TabToArch, TabToArch);

DynamicArray_TabToArch all_entries;

#ifdef WIN32
#include <Windows.h>

void find_tab_files(const char *dir, DynamicArray_String *tab_files) {
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", dir);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;

        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s\\%s", dir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            find_tab_files(full_path, tab_files);
        } else {
            const char *ext = strrchr(fd.cFileName, '.');
            if (ext && strcmp(ext, ".tab") == 0) {
                String *tmp = DA_append_get(tab_files);
                String_from_cstr(tmp, full_path);
            }
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
}

#else
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

static int is_dir_path(const char *fullpath, const struct dirent *ent) {



// Use d_type if available and reliable; otherwise lstat
#ifdef DT_DIR
if (ent&& ent->d_type!= DT_UNKNOWN) {
        return ent->d_type == DT_DIR;
    }
#endif
struct stat st;
    if (lstat(fullpath, &st)== 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

void find_tab_files(const char *dir, DynamicArray_String *tab_files) {
    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    char full_path[PATH_MAX];

    while ((ent = readdir(d)) != NULL) {
        const char *name = ent->d_name;

        if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
            continue;

        int n = snprintf(full_path, sizeof full_path, "%s/%s", dir, name);
        if (n < 0 || (size_t) n >= sizeof full_path)
            continue; // path too long, skip

        if (is_dir_path(full_path, ent)) {
            find_tab_files(full_path, tab_files);
        } else {
            const char *ext = strrchr(name, '.');
            if (ext && strcmp(ext, ".tab") == 0) {
                String_from_cstr(DA_append_get(tab_files), full_path);

            }
        }
    }
    closedir(d);
}

#endif

void collect_types(DynamicArray_String *all_tabs, STI_TypeLibrary* lib) {
    TabArchive ar = {0};
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
    ADF_load_builtin_adf(lib, TERRAINSYSTEMTYPES_ADF_TYPE_LIBRARY_INFO, sizeof(TERRAINSYSTEMTYPES_ADF_TYPE_LIBRARY_INFO));
    ADF_load_builtin_adf(lib, TERRAINSYSTEM_ADF_TYPE_LIBRARY_INFO, sizeof(TERRAINSYSTEM_ADF_TYPE_LIBRARY_INFO));
    ADF_load_builtin_adf(lib, MODEL_ADF_TYPE_MEMORY_0, sizeof(MODEL_ADF_TYPE_MEMORY_0));
    ADF_load_builtin_adf(lib, RUNTIME_EFFECT_LIBRARY, sizeof(RUNTIME_EFFECT_LIBRARY));
    ADF_load_builtin_adf(lib, PFX_ADF_TYPE_MEMORY, sizeof(PFX_ADF_TYPE_MEMORY));

    for (int i = 0; i < all_tabs->count; ++i) {
        String *tab_path = DA_at(all_tabs, i);
        printf("Processing types from %s\n", String_data(tab_path));
        TabArchive_open(&ar, tab_path);

        for (int i = 0; i < ar.entries.count; ++i) {
            TabEntry *entry = DA_at(&ar.entries, i);
            MemoryBuffer mb = {0};
            if (!TabArchive_get_data(&ar, entry->hash, &mb)) {
                printf("File not found\n");
                continue;
            }
            if (mb.data[0] == ' ' && mb.data[1] == 'F' && mb.data[2] == 'D' && mb.data[3] == 'A') {
                ADF_from_buffer(&adf, (Buffer *) &mb, lib);
                ADF_free(&adf);
            }
            mb.close(&mb);
        }

        TabArchive_free(&ar);
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
    DynamicArray_String all_tabs;
    DA_init(&all_tabs, String, 64);

    String_from_cstr(&tmp, argv[1]);
    Path_convert_to_wsl(&game_root, &tmp);
    find_tab_files(String_data(&game_root), &all_tabs);

    collect_types(&all_tabs, &lib);

    STI_TypeLibrary_free(&lib);
    String_free(&game_root);
    String_free(&tmp);
    DA_free_with_inner(&all_tabs, {String_free(it);});
    return 0;
}