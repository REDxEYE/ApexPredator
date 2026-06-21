#include <cstdio>

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
    std::vector<ArchiveEntry> all_entries;
    auto& manager = app_state.manager();
    manager.all_entries(all_entries);

    for (uint32 i = 0; i < all_entries.size(); ++i) {
        if (i > 0 && i % 100 == 0) {
            std::cout<< std::format("Processing file {}/{}\r", i, all_entries.size());
            std::flush(std::cout);
            // break;
        }
        const auto &entry = all_entries.at(i);
        auto buffer = manager.get(entry.path_hash);

        std::vector<uint8> first_buffer(8);
        buffer->read_exact<uint8>(first_buffer);
        buffer->set_position(0);

        if (std::memcmp(first_buffer.data(), AAF_MAGIC, 4) == 0) {
            AAFArchive aaf_archive(std::move(buffer));

            auto aaf_buffer = aaf_archive.get_data();

            aaf_buffer->read_exact<uint8>(first_buffer);
            aaf_buffer->set_position(0);

            if (std::memcmp(first_buffer.data() + 4, "SARC", 4) == 0) {
                SArchive sarc(entry.path_hash, std::move(aaf_buffer));
                std::vector<ArchiveEntry> sarc_entries;
                sarc.all_entries(sarc_entries);

                for (auto &sarc_entry: sarc_entries) {
                    auto sarc_buffer = sarc.get(sarc_entry.path_hash);

                    sarc_buffer->read_exact<uint8>(first_buffer);
                    sarc_buffer->set_position(0);

                    if (!sarc_buffer) {
                        GLog_Warning("Failed to read file {} from SARC {}",
                                     find_name(sarc_entry.path_hash).value_or("Unknown"),
                                     find_name(entry.path_hash).value_or("Unknown"));
                        continue;
                    }

                    if (memcmp(first_buffer.data() + 4, "TAG0", 4) == 0) {
                        process_havok_file(lib, std::move(sarc_buffer));
                    }
                }
            }
        }
        else if (memcmp(first_buffer.data() + 4, "TAG0", 4) == 0) {
            process_havok_file(lib, std::move(buffer));
        }
    }
    printf("\n");

    Havok::CodeGen::generate_code(lib,
                                  "../src/havok/generated",
                                  "../include/havok/generated");
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
    AssetDB db(argv[2]);
    AssetDB::set_instance(&db);
    Havok::CodeGen::TypeLibrary type_library;

    // auto buffer = app_state.manager().get(1615997716);
    // process_havok_file(type_library, std::move(buffer));
    //
    // Havok::CodeGen::generate_code(type_library,
    //                               "../src/havok/generated",
    //                               "../include/havok/generated");

    collect_types(app_state, type_library);

    return 0;
}
