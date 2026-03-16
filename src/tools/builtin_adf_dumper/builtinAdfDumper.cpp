// Created by RED on 15.02.2026.
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <Windows.h>

#include "MinHook.h"

#include "patterns.h"

const uint64_t load_adf_func_addr = 0x0000000140E72C50;
const uint64_t image_base = 0x140000000;

const char *load_adf_func_pattern = "40 55 56 57 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24";
const char *hash_little_func_pattern = "48 89 5C 24 ? 48 89 7C 24 ? 41 81 C0 ? ? ? ? 48 8B FA 44";

const char *adf_output_file = "D:\\projects\\cpp\\ApexPredator\\include\\apex\\adf\\builtin_adf.h";
const char *hashes_output = "D:\\projects\\cpp\\ApexPredator\\gz_strings\\game_dump.txt";

FILE *g_hashes_file = NULL;
FILE *g_dump_file = NULL;

typedef __int64 (__fastcall *load_adf_fn)(__int64 *a1, __int64 raw_data, unsigned __int64 raw_data_size, char a4);

typedef uint64_t (__fastcall *hashlittle_fn)(const char *str, uint64_t str_len, int initial);

static load_adf_fn g_load_adf_fn_orig = NULL;
static hashlittle_fn g_hashlittle_fn_orig = NULL;

uint64_t *g_all_files = NULL;
uint32_t g_all_files_capacity = 100;
uint32_t g_all_files_count = 0;

bool string_is_printable(const char *str, const uint64_t str_len) {
    for (uint64_t i = 0; i < str_len; i++) {
        if (str[i] < 32 || str[i] > 126) {
            return false;
        }
    }
    return true;
}

uint64_t __fastcall hashlittle_hook(const char *str, const uint64_t str_len, const int initial) {
    if (*str && string_is_printable(str, str_len)) {
        fprintf(g_hashes_file, "%.*s\n", (int) str_len, str);
    }
    uint64_t hash = g_hashlittle_fn_orig(str, str_len, initial);
    return hash;
}

void dump_as_hex(uint8_t *data, uint64_t size) {
    fprintf(g_dump_file, "static unsigned char adf_%p[%llu] = {", (void *) data, size);
    // Split via newline every 512 bytes
    for (uint64_t i = 0; i < size; i++) {
        if (i % 16 == 0) {
            fprintf(g_dump_file, "\n    ");
        }
        fprintf(g_dump_file, "0x%02X, ", data[i]);
    }
    fprintf(g_dump_file, "\n};\n\n");
    fflush(g_dump_file);
}

bool already_present(const uint64_t ptr, const uint64_t size) {
    for (uint32_t i = 0; i < g_all_files_count; i++) {
        if (g_all_files[i] == ptr) {
            return true;
        }
    }
    return false;
}

__int64 __fastcall load_adf_hook(__int64 *a1, __int64 raw_data, unsigned __int64 raw_data_size, char a4) {
    printf("load_adf_hook called with raw_data=0x%p, raw_data_size=%llu\n", (void *) raw_data, raw_data_size);
    if (!already_present(raw_data, raw_data_size)) {
        dump_as_hex((uint8_t *) raw_data, raw_data_size);
        if (g_all_files_count + 1 >= g_all_files_capacity) {
            g_all_files_capacity *= 2;
            g_all_files = static_cast<uint64_t *>(realloc(g_all_files, g_all_files_capacity * sizeof(uint64_t)));
            if (g_all_files == NULL) {
                fprintf(stderr, "Failed to realloc g_all_files\n");
                exit(1);
            }
        }
        g_all_files[g_all_files_count] = raw_data;
        g_all_files_count++;
    }

    return g_load_adf_fn_orig(a1, raw_data, raw_data_size, a4);
}

bool hook_function(const char *pattern, const char *name, void *orig_slot, void *hook) {
    const Region self = get_module();
    const Region text_section = get_pe_section(self, ".text");
    if (text_section.base == 0) {
        fprintf(stderr, "Failed to find .text section\n");
        return false;
    }
    Pattern fn_pattern = {};
    pattern_compile(&fn_pattern, pattern);

    const uint8_t *fn_addr = pattern_find_first((void *) text_section.base, text_section.end - text_section.base,
                                                &fn_pattern);
    if (!fn_addr) {
        fprintf(stderr, "Failed to find %s by pattern\n", name);
        return false;
    }
    printf("Found %s at: 0x%p\n", name, fn_addr);

    if (MH_CreateHook((void*)fn_addr, hook, (void**)orig_slot) != MH_OK) {
        fprintf(stderr, "Failed to create hook for %s\n", name);
        return false;
    }

    if (MH_EnableHook((void*)fn_addr) != MH_OK) {
        fprintf(stderr, "Failed to enable hook for %s\n", name);
        return false;
    }

    return true;
}

bool install_hook() {
    const Region self = get_module();
    const Region text_section = get_pe_section(self, ".text");
    if (text_section.base == 0) {
        fprintf(stderr, "Failed to find .text section\n");
        return false;
    }
    if (MH_Initialize() != MH_OK) {
        fprintf(stderr, "Failed to initialize MinHook\n");
        return false;
    }
    if (!hook_function(load_adf_func_pattern, "load_adf", (void **) &g_load_adf_fn_orig, load_adf_hook)) {
        return false;
    }
    if (!hook_function(hash_little_func_pattern, "hashlittle", (void **) &g_hashlittle_fn_orig, hashlittle_hook)) {
        return false;
    }

    return true;
}

void remove_hook() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}


void write_headers() {
    fprintf(g_dump_file, "#ifndef APEXPREDATOR_BUILTIN_ADF_H\n#define APEXPREDATOR_BUILTIN_ADF_H\n\n");
}

void write_end() {
    fprintf(g_dump_file,
            "typedef struct{\n"
            "    const char* data;\n"
            "    uint64_t size;\n"
            "}BuiltinAdf;\n\n"
            "static BuiltinAdf builtin_adfs[] = {\n"
    );
    for (uint32_t i = 0; i < g_all_files_count; i++) {
        fprintf(g_dump_file, "    { (const char*)adf_%p, sizeof(adf_%p) },\n", (void *) g_all_files[i],
                (void *) g_all_files[i]);
    }
    fprintf(g_dump_file, "};\n");
    fprintf(g_dump_file, "#endif //APEXPREDATOR_BUILTIN_ADF_H\n");
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID *reserved) {
    (void) instance;
    (void) reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
        SetConsoleOutputCP(CP_UTF8);
        freopen("CON", "w", stdout);
        g_dump_file = fopen(adf_output_file, "w");
        g_hashes_file = fopen(hashes_output, "w");

        g_all_files = static_cast<uint64_t *>(calloc(100, sizeof(uint64_t)));
        if (g_dump_file == NULL) {
            fprintf(stderr, "Failed to open dump file\n");
            exit(1);
        }
        write_headers();

        if (!install_hook()) {
            fprintf(stderr, "Failed to install hook\n");
            exit(1);
        }
    }

    if (reason == DLL_PROCESS_DETACH) {
        remove_hook();
        write_end();
        free(g_all_files);
        fclose(g_dump_file);
        fclose(g_hashes_file);
        FreeConsole();
    }

    return TRUE;
}
