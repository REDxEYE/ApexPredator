#include <stdbool.h>
#include <stdio.h>

#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "utils/dynamic_array.h"
#include "utils/string.h"
#include "utils/path.h"
#include "utils/buffer/buffer.h"

#include "apex/package/tab_archive.h"
#include "havok/havok_codegen.h"
#include "platform/logger.h"

void process_havok_file(Havok_TypeLibrary* lib, Buffer* buffer) {
    TagFile tag_file={0};
    TagFile_from_buffer(&tag_file, buffer);
    Havok_TypeLibrary_copy_from_tag_file(lib, &tag_file);
    TagFile_free(&tag_file);
}

void collect_types(ArchiveManager *archive_manager, Havok_TypeLibrary *lib) {
    DynamicArray_ArchiveEntry all_entries = {0};
    ArchiveManager_get_all_entries(archive_manager, &all_entries);
    for (int i = 0; i < all_entries.count; ++i) {
        if (i>0 && i%1000==0) {
            printf("Processing file %i/%i\r", i, all_entries.count);
            fflush(stdout);
            break;
        }
        ArchiveEntry *entry = DA_at(&all_entries, i);
        MemoryBuffer mb = {0};
        if (!ArchiveManager_get_file_by_hash(archive_manager, entry->path_hash, &mb)) {
            GLog_Error("File not found");
            return;
        }

        if (mb.data[0] == 'A' && mb.data[1] == 'A' && mb.data[2] == 'F' && mb.data[3] == '\0') {
            AAFArchive aaf_archive = {0};
            AAFArchive_from_buffer(&aaf_archive, (Buffer *) &mb);
            MemoryBuffer *section_buffer = MemoryBuffer_new();
            if (!AAFArchive_get_data(&aaf_archive, section_buffer)) {
                GLog_Error("Failed to get AAF section %i", i);
                return;
            }
            if (section_buffer->data[4] == 'S' && section_buffer->data[5] == 'A' &&
                section_buffer->data[6] == 'R' && section_buffer->data[7] == 'C') {
                SArchive *sarc = SArchive_new((Buffer *) section_buffer, entry->path_hash); // sarc is now owner of buffer
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
                mp_free(sarc);
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

    String header_path = {0};

    String_from_cstr(&header_path, "D:/projects/cpp/ApexPredator/include/havok/generated/havok_generated.h");
    Path_convert_to_wsl(&header_path);
    Path_ensure_parent_dirs(&header_path);
    FILE *header_file = fopen(String_cstr(&header_path), "w");
    // FILE *header_file = stdout;
    //
    String_from_cstr(&header_path, "D:/projects/cpp/ApexPredator/src/havok/generated/havok_generated.c");
    Path_convert_to_wsl(&header_path);
    Path_ensure_parent_dirs(&header_path);
    // FILE *impl_file = stdout;
    FILE *impl_file = fopen(String_cstr(&header_path), "w");

    String_free(&header_path);

    String header_rel_path = {};
    String_append_cstr(&header_rel_path, "havok/generated/havok_generated.h");

    Havok_TypeLibrary_generate_code(lib, &namespace, header_file, &header_rel_path, impl_file);

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

    Havok_TypeLibrary lib = {0};
    String game_root = {0};
    Havok_TypeLibrary_init(&lib);

    ArchiveManager manager = {0};
    ArchiveManager_init(&manager);

    String_from_cstr(&game_root, argv[1]);
    Path_convert_to_wsl(&game_root);
    TabArchives_init(&manager, &game_root);
    collect_types(&manager, &lib);

    ArchiveManager_free(&manager);
    Havok_TypeLibrary_free(&lib);
    String_free(&game_root);
    return 0;
}
