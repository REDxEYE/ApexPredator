#include <cstdio>

#include "games.hpp"
#include "apex/asset_db.h"
#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "redscore/platform/file/file.h"

#include "apex/package/tab_archive.h"
#include "havok/havok_codegen.h"
#include "platform/app_state.h"
#include "redscore/platform/logger.h"

void process_havok_file(Havok::CodeGen::TypeLibrary &lib, std::unique_ptr<IO::File> &&buffer) {
    const Havok::Tag::TagFile tag_file(std::move(buffer));
    lib.register_types(tag_file);
}

void collect_types(ApexAppState &app_state, Havok::CodeGen::TypeLibrary &lib) {
    auto &manager = app_state.manager();
    static auto visited_files = std::unordered_set<u64>();

    app_state.manager().foreach_file([&manager, &lib](const Archive<u64>::ArchiveEntry &archive_entry) {
        if (archive_entry.size <= 8) {
            return true;
        }
        if (visited_files.contains(archive_entry.key)) {
            return true;
        }
        visited_files.insert(archive_entry.key);

        auto buffer = manager.get(archive_entry.key);
        if (!buffer) {
            GLog_Warning("Failed to read file {}", find_name(archive_entry.key).value_or("Unknown"));
            return true;
        }

        std::vector<uint8> first_buffer(8);
        buffer->read_exact<uint8>(first_buffer);
        buffer->set_position(0);

        if (std::memcmp(first_buffer.data(), AAF_MAGIC, 4) == 0) {
            AAFArchive aaf_archive(std::move(buffer));

            auto aaf_buffer = aaf_archive.get_data();

            aaf_buffer->read_exact<uint8>(first_buffer);
            aaf_buffer->set_position(0);

            if (std::memcmp(first_buffer.data() + 4, "SARC", 4) == 0) {
                SArchive sarc(archive_entry.key, std::move(aaf_buffer));
                sarc.foreach_file(
                    [&archive_entry, &sarc, &first_buffer, &lib](const Archive<u64>::ArchiveEntry &sarc_entry) {
                        auto sarc_buffer = sarc.get(sarc_entry.key);

                        sarc_buffer->read_exact<uint8>(first_buffer);
                        sarc_buffer->set_position(0);

                        if (!sarc_buffer) {
                            GLog_Warning("Failed to read file {} from SARC {}",
                                         find_name(sarc_entry.key).value_or("Unknown"),
                                         find_name(archive_entry.key).value_or("Unknown")
                            );
                            return true;
                        }

                        if (memcmp(first_buffer.data() + 4, "TAG0", 4) == 0) {
                            process_havok_file(lib, std::move(sarc_buffer));
                        }
                        return true;
                    });
            }
        } else if (memcmp(first_buffer.data() + 4, "TAG0", 4) == 0) {
            process_havok_file(lib, std::move(buffer));
        }
        return true;
    });
    printf("\n");

    Havok::CodeGen::generate_code(lib,
#if GAME==GAME_GENERATION_ZERO
                                  "../modules/generation_zero/src/havok/generated",
                                  "../modules/generation_zero/include/havok/generated"
#elif GAME==GAME_RAGE2
                                  "../modules/rage2/src/havok/generated",
                                          "../modules/rage2/include/havok/generated"
#elif GAME==GAME_SECOND_EXTINCTION
                                  "../modules/second_extinction/src/havok/generated",
                                          "../modules/second_extinction/include/havok/generated"
#else
#error "Unsupported game"
#endif
    );
}


/*TODO use template specialization in havok code gen

    template<typename, typename>
    struct hkcdStaticMeshTree;

    template<>
    struct hkcdStaticMeshTree<hkcdStaticMeshTreeCommonConfig<hkUint32, hkUint64, 11, 21>, hknpCompressedMeshShapeTreeDataRun>: hkcdStaticMeshTreeBase {
    hkArray<hkUint32, hkContainerHeapAllocator> packedVertices; // offset: 112, size: 16
    hkArray<hkUint64, hkContainerHeapAllocator> sharedVertices; // offset: 128, size: 16
    hkArray<hknpCompressedMeshShapeTreeDataRun, hkContainerHeapAllocator> primitiveDataRuns; // offset: 144, size: 16

    // void read(IO::File& buffer, Havok::Tag::TagFile& tag_file) override;
    // void print(std::ostream &os) const override;
    // void to_json(std::ostream &os) const override;
    };
*/

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        printf("USAGE: %s <path_to_game_root> <db_path>\n", argv[0]);
        return 0;
    }
    ApexAppState app_state(argv[1]);
    app_state.mount_archives();
    AssetDB db(argv[2]);
    AssetDB::set_instance(&db);
    Havok::CodeGen::TypeLibrary type_library;


    auto mount_gtoc = [&app_state](const std::string_view name) {
        auto buffer = app_state.manager().get(name);
        if (!buffer) {
            return;
        }
        auto gtoc_archive = std::make_unique<
            GTOCArchive>(app_state.manager(), std::move(buffer), asset_path_hash(name));
        app_state.manager().mount(std::move(gtoc_archive));
    };
    mount_gtoc("sarc.0.gtoc");
    mount_gtoc("resourcesets/expentities.gtoc");

    // auto buffer = app_state.manager().get(1615997716);
    // process_havok_file(type_library, std::move(buffer));
    //
    // Havok::CodeGen::generate_code(type_library,
    //                               "../modules/generation_zero/src/havok/generated",
    //                               "../modules/generation_zero/include/havok/generated");

    collect_types(app_state, type_library);

    return 0;
}
