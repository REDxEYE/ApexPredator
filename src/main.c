#include <stdio.h>
#include "apex/adf/adf_types.h"
#include "apex/package/tab_archive.h"
#include "havok/havok_codegen.h"
#include "havok/havok_generated.h"
#include "platform/archive_manager.h"
#include "utils/string.h"
#include "utils/path.h"
#include "utils/hash_helper.h"
#include "utils/gltf/cgltf_helper.h"

#include "exporter/amf_export.h"
#include "exporter/common_export.h"

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        printf("USAGE: %s <path_to_game_root> <path_to_file> [extra_path]\n", argv[0]);
        return 0;
    }
    ArchiveManager manager = {0};
    ArchiveManager_init(&manager);

    STI_TypeLibrary lib = {0};
    Havok_TypeLibrary havok_lib = {0};
    HavokTypeLib_init(&havok_lib);
    STI_TypeLibrary_init(&lib);
    STI_ADF_TYPES_register_functions(&lib);
    HAVOK_TYPES_register_functions(&havok_lib);


    String tmp = {0};
    String ar_path = {0};
    String game_root = {0};
    String_from_cstr(&tmp, argv[1]);
    Path_convert_to_wsl(&game_root, &tmp);
    TabArchives_init(&manager, &game_root);
    String export_path = {};
    String_from_cstr(&export_path, "../exported");
    String file_path = {};
    GLTFContext context = {0};
    GLTFContext_init(&context, "root");


    String_from_cstr(&file_path, argv[2]);
    String_from_cstr(&file_path, "editor/entities/characters/machines/dreadnought/drea_classb_load01.ee");
    export_file(&context, &manager, &lib, &havok_lib, &file_path, hash_string(&file_path), &export_path);
    String_from_cstr(&file_path, "editor/entities/characters/machines/dreadnought/drea_classb_load01.epe");
    export_file(&context, &manager, &lib, &havok_lib, &file_path, hash_string(&file_path), &export_path);
    if (argc >= 4) {
        String_from_cstr(&file_path, argv[3]);
        export_file(&context, &manager, &lib, &havok_lib, &file_path, hash_string(&file_path), &export_path);
    }
    // String_from_cstr(&file_path, "animations/skeletons/characters/dreadnought.bsk");
    // export_file(&context, &manager, &lib, &havok_lib, &file_path, hash_string(&file_path), &export_path);

    GLTFContext_write_and_free(&context);

    // String_from_cstr(&file_path, "models/characters/machines/hunter/hunter_base_prototype.modelc");
    // export_file(&manager, &lib, &file_path, hash_string(&file_path), &export_path);

    ArchiveManager_free(&manager);
    STI_TypeLibrary_free(&lib);
    HavokTypeLib_free(&havok_lib);
    String_free(&ar_path);
    String_free(&tmp);
    String_free(&game_root);
    String_free(&export_path);
    String_free(&file_path);
    return 0;
}
