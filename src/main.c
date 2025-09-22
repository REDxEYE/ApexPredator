#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "apex/package/tab.h"
#include "utils/dynamic_array.h"
#include "apex/adf/adf.h"
#include "apex/adf/adf_types.h"
#include "apex/package/archive.h"
#include "utils/string.h"
#include "utils/buffer/buffer.h"
#include "utils/buffer/file_buffer.h"
#include "utils/lookup3.h"

#include "apex/adf/adf_types.h"

typedef struct {
    uint32 hash;
    String archive_name;
    String file_name;
} TabToArch;

DYNAMIC_ARRAY_STRUCT(TabToArch, TabToArch);

DynamicArray_TabToArch all_entries;

#ifdef WIN32

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
                String *tmp = DA_append_get(tab_files);
                String_from_cstr(tmp, full_path);
            }
        }
    }

    closedir(d);
}

#endif

bool collect_entries(DynamicArray_String all_tabs) {
    FileBuffer file_buffer = {0};
    for (int tab_id = 0; tab_id < all_tabs.count; tab_id++) {
        String *tab_path = DA_at(&all_tabs, tab_id);
        printf("Processing %s\n", tab_path->buffer);
        assert(FileBuffer_open(&file_buffer, tab_path->buffer)==BUFFER_SUCCESS);

        TabHeader header;
        file_buffer.read(&file_buffer, &header, sizeof(header),NULL);
        TabEntry entry;

        BufferError error;
        uint32 entry_count = Buffer_remaining((Buffer *) &file_buffer, &error) / sizeof(TabEntry);
        for (int i = 0; i < entry_count; i++) {
            error = file_buffer.read(&file_buffer, &entry, sizeof(TabEntry), NULL);
            if (error < BUFFER_FAILED) {
                printf("Failed to read entry %d\n", i);
                return false;
            }
            TabToArch *all_entry = DA_append_get(&all_entries);
            all_entry->hash = entry.hash;
            String_from_cstr(&all_entry->archive_name, tab_path->buffer);
        }
    }
    return true;
}

void check_hash(char template_buffer[8192]) {
    uint32 hash = 0;
    uint32 init2 = 0;
    hashlittle2(template_buffer, strlen(template_buffer), &hash, &init2);
    for (int k = 0; k < all_entries.count; k++) {
        TabToArch *entry = &all_entries.items[k];
        if (entry->hash == hash) {
            printf("Hash %08X = %s\n", hash, template_buffer);
            String_from_cstr(&entry->file_name, template_buffer);
            break;
        }
    }
}
#ifdef WIN32
bool collect_hashes(DynamicArray_String all_tabs) {
    DA_init(&all_entries, TabToArch, 1024);
    const char *strings_path = "D:\\projects\\cpp\\ApexPredator\\strings_procmon.txt";
    const char *mapping_out = "D:\\projects\\cpp\\ApexPredator\\mapping_out.txt";
    FILE *strings_file = fopen(strings_path, "r");
    FILE *mapping_file = fopen(mapping_out, "w");

    if (!collect_entries(all_tabs)) return false;

    while (!feof(strings_file)) {
        static char buff[1024];
        char *string = fgets(buff, 1024, strings_file);
        if (string == NULL) {
            assert(feof(strings_file));
            break;
        }
        string[strlen(string) - 1] = '\0';
        uint32 hash = 0;
        uint32 init2 = 0;
        hashlittle2(string, strlen(string), &hash, &init2);
        for (int i = 0; i < all_entries.count; i++) {
            TabToArch *entry = &all_entries.items[i];
            if (entry->hash == hash) {
                String_from_cstr(&entry->file_name, string);
                break;
            }
        }
    }

    char template_buffer[8192];
    const char *template = "ai/tiles/%02i_%02i.navmeshc";
    for (int i = 0; i < 99; ++i) {
        for (int j = 0; j < 99; ++j) {
            snprintf(template_buffer, 8192, template, i, j);
            check_hash(template_buffer);
        }
    }
    printf("%s finished\n", template);

    template = " terrain/hp/patches/patch_%02i_%02i_%02i.streampatch";
    for (int i = 0; i < 99; ++i) {
        for (int j = 0; j < 99; ++j) {
            for (int k = 0; k < 99; ++k) {
                snprintf(template_buffer, 8192, template, i, j, k);
                check_hash(template_buffer);
            }
        }
    }


    for (int i = 0; i < all_entries.count; i++) {
        TabToArch *entry = &all_entries.items[i];
        fprintf(mapping_file, "0x%08X %s ", entry->hash, String_data(&entry->archive_name));
        if (entry->file_name.size == 0) {
            fprintf(mapping_file, "<NO_MATCH>\n");
        } else {
            fprintf(mapping_file, " %s\n", String_data(&entry->file_name));
        }
    }


    DA_free(&all_entries);
    fclose(mapping_file);
    fclose(strings_file);
    for (int i = 0; i < all_tabs.count; ++i) {
        String_free(&all_tabs.items[i]);
    }
    return true;
}

#endif

#ifdef WIN32
void String_convert_to_wsl(String *out, String *in) {
    String_copy_from(out, in);
}
#else
void String_convert_to_wsl(String *out, String *in) {
    String_init(out, in->size + 10);
    char *buffer = String_data(out);

    char drive = in->buffer[0];
    snprintf(buffer, out->capacity, "/mnt/%c%s", (drive > 'A' ? drive + ' ' : drive), in->buffer + 2);
    // Fix slashes
    for (int i = 0; i < out->capacity; ++i) {
        if (buffer[i] == '\\') {
            buffer[i] = '/';
        }
    }
    out->size = strlen(String_data(out));
}
#endif

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        printf("USAGE: %s <path_to_game_root>\n", argv[0]);
        return 0;
    }
    const char *game_root = argv[1];
    DynamicArray_String all_tabs;
    DA_init(&all_tabs, String, 64);
    find_tab_files(game_root, &all_tabs);
    Archive ar = {0};
    String tmp = {0};
    String ar_path = {0};
    String_from_cstr(
        &tmp,
        "C:\\Program Files (x86)\\Steam\\steamapps\\common\\GenerationZero\\archives_win64\\initial\\game0.tab");
    String_convert_to_wsl(&ar_path, &tmp);
    String_free(&tmp);

    Archive_open(&ar, &ar_path);
    String_free(&ar_path);


    MemoryBuffer mb = {0};
    if (!Archive_get_data(&ar, 0xE65813DD, &mb)) {
        printf("File not found\n");
        return 1;
    }

    // FILE *tmpf = fopen("test.bin", "wb");
    // fwrite(mb.data, 1, mb.size, tmpf);
    // fclose(tmpf);

    ADF adf = {};
    ADF_from_buffer(&adf, (Buffer *) &mb);
    STI_ADF_register_functions(&adf.type_library);

    String namespace = {0};
    String_from_cstr(&namespace, "ADF");
    // ADF_generate_readers(&adf, &namespace, stdout);
    String_free(&namespace);
    for (int i = 0; i < all_tabs.count; ++i) {
        // Archive
    }



    MemoryBuffer instance_memory = {};
    for (int i = 0; i < adf.header.instance_count; ++i) {
        ADFInstance *instance = DA_at(&adf.instances, i);
        MemoryBuffer_allocate(&instance_memory, instance->size);
        memcpy(instance_memory.data, mb.data+instance->offset, instance->size);
        STI_Type *type = DM_get(&adf.type_library.types, instance->type_hash);
        if (type == NULL) {
            printf("Unknown type hash %08X for instance %s\n", instance->type_hash,
                   String_data(&adf.strings.items[instance->name_id]));
            return false;
        }

        void *instance_data = malloc(instance->size);
        STI_ObjectMethods object_methods = *(STI_ObjectMethods *) DM_get(&adf.type_library.object_functions, instance->type_hash);
        if (object_methods.read == NULL) {
            printf("No read function for type hash %08X (%s)\n", instance->type_hash, String_data(&type->name));
            free(instance_data);
            return false;
        }
        if (!object_methods.read((Buffer*)&instance_memory, instance_data)) {
            printf("Failed to read instance %s of type %s\n", String_data(&adf.strings.items[instance->name_id]),
                   String_data(&type->name));
            free(instance_data);
            return false;
        }
        object_methods.print(instance_data, stdout, 0);
        object_methods.free(instance_data);
        instance_memory.close(&instance_memory);
        free(instance_data);
        // if (i>4)break;
    }

    // if (!collect_hashes(all_tabs)) {
    //     return 1;
    // }
    mb.close(&mb);
    ADF_free(&adf);
    Archive_free(&ar);
    DA_free(&all_tabs);

    return 0;
}
