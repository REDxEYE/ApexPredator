#include <ranges>

#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "apex/adf/adf.h"
#include "apex/package/tab_archive.h"
#include "apex/adf/builtin_adf.h"
#include "apex/adf/sti.h"
#include "platform/app_state.h"
#include "platform/logger.h"
#include "tracy/Tracy.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#endif


void collect_types(AppState &app_state, STI::TypeLibrary &lib) {
    for (uint32 i = 0; i < std::size(builtin_adfs); ++i) {
        const auto &[data, size] = builtin_adfs[i];
        auto adf = ADF::ADFFile::from_buffer(data, size);
        STI::register_types_from_adf(lib, adf);
    }

    std::vector<ArchiveEntry> all_entries;
    auto &manager = app_state.manager();
    manager.all_entries(all_entries);
    for (auto [id, entry]: all_entries | std::views::enumerate) {
        // if (id!=0 && id%10000==0) {
        //     break;
        // }
        auto file = manager.get_file(entry.path_hash);

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
                    auto sarc_buffer = sarc.get_file(sarc_entry.path_hash);

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

    while (!TracyIsConnected) {
#ifdef _WIN32
        Sleep(100); /* Windows */
#else
        usleep(10000);
#endif
        printf("\rWaiting for tracy;");
    }
    printf("\n");

    set_db_path(argv[2]);

    AppState app_state(argv[1]);

    STI::TypeLibrary type_library;

    collect_types(app_state, type_library);

    STI::generate_code(type_library,
                       "D:/projects/cpp/ApexPredator/src/apex/adf/generated",
                       "D:/projects/cpp/ApexPredator/include/apex/adf/generated");
    close_assets_db();

    return 0;
}
