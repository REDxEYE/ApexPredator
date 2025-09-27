// Created by RED on 26.09.2025.
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
#include "apex/package/archive.h"
#include "apex/adf/builtin_adf.h"
#include "utils/lookup3.h"

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

void collect_hash_strings(DynamicArray_String *all_tabs, STI_TypeLibrary *lib) {
    Archive ar = {0};
    ADF adf = {0};
    FILE *hash_file = fopen("./../hashes.txt", "w");
    for (int i = 0; i < all_tabs->count; ++i) {
        String *tab_path = DA_at(all_tabs, i);
        printf("Processing types from %s\n", String_data(tab_path));
        Archive_open(&ar, tab_path);

        for (int i = 0; i < ar.entries.count; ++i) {
            TabEntry *entry = DA_at(&ar.entries, i);
            MemoryBuffer mb = {0};
            if (!Archive_get_data(&ar, entry->hash, &mb)) {
                printf("File not found\n");
                continue;
            }
            if (mb.data[0] == ' ' && mb.data[1] == 'F' && mb.data[2] == 'D' && mb.data[3] == 'A') {
                ADF_from_buffer(&adf, (Buffer *) &mb, lib);
                uint32 hash = 0;
                uint32 hash2 = 0;
                for (int j = 0; j < adf.hash_strings.count; ++j) {
                    hash = 0;
                    hash2 = 0;
                    String *hash_str = DA_at(&adf.hash_strings, j);
                    hashlittle2(String_data(hash_str), hash_str->size, &hash, &hash2);
                    fprintf(hash_file, "0x%08X %s\n", hash, String_data(hash_str));
                }

                ADF_free(&adf);
            }
            mb.close(&mb);
        }
        Archive_free(&ar);
    }
    fclose(hash_file);
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

    collect_hash_strings(&all_tabs, &lib);

    STI_TypeLibrary_free(&lib);
    String_free(&game_root);
    String_free(&tmp);
    DA_free_with_inner(&all_tabs, {String_free(it);});
    return 0;
}
