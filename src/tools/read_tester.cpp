#include <cstdio>

#include "apex/asset_db.h"
#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "apex/adf/adf.h"
#include "apex/adf/generated/adf_types.h"
#include "redscore/platform/file/file.h"

#include "apex/package/tab_archive.h"
#include "havok/generated/havok_types.h"
#include "havok/tag_file/havok_tag_file_get_item.h"
#include "platform/app_state.h"
#include "redscore/platform/logger.h"
#include "redscore/utils/memory_tracker.h"
#include "tracy/Tracy.hpp"

void test_havok(std::unique_ptr<IO::File> &&buffer, u64 hash) {
    try {
        Havok::Tag::TagFile tag_file(std::move(buffer));
        const auto item = Havok::Tag::get_item(tag_file, 1);
        const auto json_res = item->to_json();
        (void) json_res;
    } catch (const std::runtime_error &e) {
        fprintf(stderr, "typeid: %s\n", typeid(e).name());
        fprintf(stderr, "what: '%s'\n", e.what());
        GLog_Error("Failed to parse {} : {}", hash, e.what());
    }
}

void test_adf(std::unique_ptr<IO::File> &&buffer, u64 hash) {
    try {
        auto adf_file = ADF::ADFFile::from_buffer(std::move(buffer));
        for (int i = 0; i < adf_file.instances().size(); ++i) {
            try {
                const auto item = adf_file.read_instance(i);
                const auto json_res = item->to_json();
                (void) json_res;
            } catch (const std::runtime_error &e) {
                GLog_Error("Failed to parse instance {} of {} : {}", i, hash, e.what());
            }
        }
    } catch (const std::runtime_error &e) {
        fprintf(stderr, "typeid: %s\n", typeid(e).name());
        fprintf(stderr, "what: '%s'\n", e.what());
        GLog_Error("Failed to parse {} : {}", hash, e.what());
    }
}

void collect_types(ApexAppState &app_state) {
    std::vector<ArchiveEntry> all_entries;
    auto &manager = app_state.manager();
    manager.all_entries(all_entries);

    for (const auto&[path_hash, size] : all_entries) {
        auto buffer = manager.get(path_hash);

        std::vector<uint8> first_buffer(8);
        buffer->read_exact<uint8>(first_buffer);
        buffer->set_position(0);

        if (std::memcmp(first_buffer.data(), AAF_MAGIC, 4) == 0) {
            AAFArchive aaf_archive(std::move(buffer));

            auto aaf_buffer = aaf_archive.get_data();

            aaf_buffer->read_exact<uint8>(first_buffer);
            aaf_buffer->set_position(0);

            if (std::memcmp(first_buffer.data() + 4, "SARC", 4) == 0) {
                SArchive sarc(path_hash, std::move(aaf_buffer));
                std::vector<ArchiveEntry> sarc_entries;
                sarc.all_entries(sarc_entries);

                for (auto &sarc_entry: sarc_entries) {
                    auto sarc_buffer = sarc.get(sarc_entry.path_hash);

                    sarc_buffer->read_exact<uint8>(first_buffer);
                    sarc_buffer->set_position(0);

                    if (!sarc_buffer) {
                        GLog_Warning("Failed to read file {} from SARC {}",
                                     find_name(sarc_entry.path_hash).value_or("Unknown"),
                                     find_name(path_hash).value_or("Unknown"));
                        continue;
                    }

                    if (memcmp(first_buffer.data() + 4, "TAG0", 4) == 0) {
                        test_havok(std::move(sarc_buffer), sarc_entry.path_hash);
                    } else if (std::memcmp(first_buffer.data(), ADF_MAGIC, 4) == 0) {
                        test_adf(std::move(sarc_buffer), sarc_entry.path_hash);
                    }
                }
            }
        } else if (memcmp(first_buffer.data() + 4, "TAG0", 4) == 0) {
            test_havok(std::move(buffer), path_hash);
        } else if (std::memcmp(first_buffer.data(), ADF_MAGIC, 4) == 0) {
            test_adf(std::move(buffer), path_hash);
        }
    }
    printf("\n");
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        printf("USAGE: %s <path_to_game_root> <db_path>\n", argv[0]);
        return 0;
    }

    //     while (!TracyIsConnected) {
    // #ifdef _WIN32
    //         Sleep(100); /* Windows */
    // #else
    //         usleep(10000);
    // #endif
    //         printf("\rWaiting for tracy;");
    //     }
    //     printf("\n");

    // mp_init();
    init_havok_type_info();
    init_adf_type_info();

    ApexAppState app_state(argv[1]);
    AssetDB db(argv[2]);
    AssetDB::set_instance(&db);

    collect_types(app_state);

    return 0;
}
