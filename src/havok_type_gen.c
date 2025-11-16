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
#include "havok/havok_codegen.h"
#include "utils/hash_helper.h"

void process_havok_file(HavokTypeLib* lib, Buffer* buffer) {
    TagFile tag_file={0};
    TagFile_from_buffer(&tag_file, buffer);
    HavokTypeLib_copy_from_tag_file(lib, &tag_file);
    TagFile_free(&tag_file);
}

void collect_types(ArchiveManager *archive_manager, HavokTypeLib *lib) {
    DynamicArray_ArchiveEntry all_entries = {0};
    ArchiveManager_get_all_entries(archive_manager, &all_entries);
    for (int i = 0; i < all_entries.count; ++i) {
        if (i>0 && i%100==0) {
            printf("Processing file %i/%i\r", i, all_entries.count);
            fflush(stdout);
            break;
        }
        ArchiveEntry *entry = DA_at(&all_entries, i);
        MemoryBuffer mb = {0};
        if (!ArchiveManager_get_file_by_hash(archive_manager, entry->path_hash, &mb)) {
            printf("File not found\n");
            return;
        }

        if (mb.data[0] == 'A' && mb.data[1] == 'A' && mb.data[2] == 'F' && mb.data[3] == '\0') {
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
                    if (Archive_get_file((Archive *) sarc, aaf_entry->path, tmp)) {
                        if (memcmp(tmp->data + 4, "TAG0", 4) == 0) {
                            process_havok_file(lib, (Buffer*)tmp);
                        }
                    }
                    tmp->close(tmp);
                }
                DA_free(&sarc_entries);
                Archive_free((Archive *) sarc);
            }
            AAFArchive_free(&aaf_archive);
        }else if (memcmp(mb.data + 4, "TAG0", 4) == 0) {
            process_havok_file(lib, (Buffer*)&mb);
        }
        mb.close(&mb);
    }
    printf("\n");

    String namespace = {0};
    String_from_cstr(&namespace, "HAVOK_TYPES");

    String header_path_tmp = {0};
    String header_path = {0};

    String_from_cstr(&header_path_tmp, "D:/projects/cpp/ApexPredator/include/havok/havok_generated.h");
    Path_convert_to_wsl(&header_path, &header_path_tmp);
    String_free(&header_path_tmp);
    FILE *header_file = fopen(String_data(&header_path), "w");

    String_from_cstr(&header_path_tmp, "D:/projects/cpp/ApexPredator/src/havok/havok_generated.c");
    Path_convert_to_wsl(&header_path, &header_path_tmp);
    String_free(&header_path_tmp);
    FILE *impl_file = fopen(String_data(&header_path), "w");

    String_free(&header_path);

    String header_rel_path = {};
    String_append_cstr(&header_rel_path, "havok/havok_generated.h");

    HavokTypeLib_generate_code(lib, &namespace, header_file, &header_rel_path, impl_file);

    String_free(&header_rel_path);
    String_free(&namespace);
    fclose(header_file);
    fclose(impl_file);
    DA_free(&all_entries);
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        printf("USAGE: %s <path_to_game_root>\n", argv[0]);
        return 0;
    }

    HavokTypeLib lib = {0};
    String tmp = {0};
    String game_root = {0};
    HavokTypeLib_init(&lib);

    ArchiveManager manager = {0};
    ArchiveManager_init(&manager);

    String_from_cstr(&tmp, argv[1]);
    Path_convert_to_wsl(&game_root, &tmp);
    TabArchives_init(&manager, &game_root);
    collect_types(&manager, &lib);

    ArchiveManager_free(&manager);
    HavokTypeLib_free(&lib);
    String_free(&game_root);
    String_free(&tmp);
    return 0;
}
