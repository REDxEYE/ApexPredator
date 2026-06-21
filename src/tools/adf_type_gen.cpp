#include <ranges>

#include "apex/asset_db.h"
#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "apex/adf/adf.h"
#include "apex/package/tab_archive.h"
#include "apex/adf/builtin_adf.h"
#include "apex/adf/sti.h"
#include "platform/app_state.h"
#include "redscore/platform/logger.h"
#include "tracy/Tracy.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#endif


void collect_types(ApexAppState &app_state, STI::TypeLibrary &lib) {
    for (auto [data, size] : builtin_adfs) {
        auto adf = ADF::ADFFile::from_buffer(data, size);
        STI::register_types_from_adf(lib, adf);
    }
    // return;
    std::vector<ArchiveEntry> all_entries;
    auto &manager = app_state.manager();
    manager.all_entries(all_entries);
    const u32 total_count = all_entries.size();
    for (auto [id, entry]: all_entries | std::views::enumerate) {
        // if (id!=0 && id%5000==0) {
        //     break;
        // }
        if (id%1000==0) {
            GLog_Info("{}/{}",id, total_count);
        }

        auto file = manager.get(entry.path_hash);

        if (!file) {
            GLog_Warning("Failed to read file {}", find_name(entry.path_hash).value_or("Unknown"));
            continue;
        }

        std::vector<uint8> first_buffer(8);
        file->read_exact<uint8>(first_buffer);
        file->set_position(0);

        if (std::memcmp(first_buffer.data(), ADF_MAGIC, 4) == 0) {
            auto adf_file = ADF::ADFFile::from_buffer(std::move(file));
            STI::register_types_from_adf(lib, adf_file);
        }
        else if (std::memcmp(first_buffer.data(), AAF_MAGIC, 4) == 0) {
            AAFArchive aaf_archive(std::move(file));

            auto aaf_buffer = aaf_archive.get_data();

            aaf_buffer->read_exact<uint8>(first_buffer);
            aaf_buffer->set_position(0);

            if (std::memcmp(first_buffer.data() + 4, "SARC", 4) == 0) {
                SArchive sarc(entry.path_hash, std::move(aaf_buffer));
                std::vector<ArchiveEntry> sarc_entries;
                sarc.all_entries(sarc_entries);

                for (auto &sarc_entry: sarc_entries) {
                    auto sarc_buffer = sarc.get(sarc_entry.path_hash);

                    if (!sarc_buffer) {
                        GLog_Warning("Failed to read file {} from SARC {}",
                                     find_name(sarc_entry.path_hash).value_or("Unknown"),
                                     find_name(entry.path_hash).value_or("Unknown"));
                        continue;
                    }

                    sarc_buffer->read_exact<uint8>(first_buffer);
                    sarc_buffer->set_position(0);

                    if (std::memcmp(first_buffer.data(), ADF_MAGIC, 4) == 0) {
                        auto adf_file = ADF::ADFFile::from_buffer(std::move(sarc_buffer));
                        STI::register_types_from_adf(lib, adf_file);
                    }
                }
            }
        }
    }
}

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        printf("USAGE: %s <path_to_game_root> <path_to_hashes.db>\n", argv[0]);
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

    ApexAppState app_state(argv[1]);

    AssetDB db(argv[2]);
    AssetDB::set_instance(&db);

    STI::TypeLibrary type_library;

    collect_types(app_state, type_library);

    STI::generate_code(type_library,
                       "../src/apex/adf/generated",
                       "../include/apex/adf/generated");

    return 0;
}
