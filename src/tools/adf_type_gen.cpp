#include <ranges>
#include <unordered_set>
#include <utility>

#include "apex/asset_db.h"
#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "apex/adf/adf.h"
#include "platform/archive_manager.h"
#include "apex/package/tab_archive.h"

#include "apex/adf/sti.h"
#include "platform/app_state.h"
#include "modules/archive_mount.h"
#include "redscore/platform/logger.h"
#include "tracy/Tracy.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#endif

// Each generator target links its game's configuration implementation.
void register_builtin_adf_types(STI::TypeLibrary &lib);
std::pair<const char *, const char *> adf_generator_output_paths();


void collect_types(ApexAppState &app_state, STI::TypeLibrary &lib) {
    register_builtin_adf_types(lib);

    auto &manager = app_state.manager();

    static auto visited_files = std::unordered_set<u64>();

    app_state.manager().foreach_file([&manager, &lib](const Archive<u64>::ArchiveEntry &archive_entry) {
        if (archive_entry.size<=8) {
            return true;
        }
        if (visited_files.contains(archive_entry.key)) {
            return true;
        }
        visited_files.insert(archive_entry.key);

        auto file = manager.get(archive_entry.key);
        if (!file) {
            GLog_Warning("Failed to read file {}", find_name(archive_entry.key).value_or("Unknown"));
            return true;
        }

        std::vector<uint8> first_buffer(8);
        file->read_exact<uint8>(first_buffer);
        file->set_position(0);

        if (std::memcmp(first_buffer.data(), ADF_MAGIC, 4) == 0) {
            auto adf_file = ADF::ADFFile::from_buffer(std::move(file));
            STI::register_types_from_adf(lib, adf_file);
        } else if (std::memcmp(first_buffer.data(), AAF_MAGIC, 4) == 0) {
            AAFArchive aaf_archive(std::move(file));

            auto aaf_buffer = aaf_archive.get_data();

            aaf_buffer->read_exact<uint8>(first_buffer);
            aaf_buffer->set_position(0);

            if (std::memcmp(first_buffer.data() + 4, "SARC", 4) == 0) {
                SArchive sarc(archive_entry.key, std::move(aaf_buffer));

                sarc.foreach_file(
                    [&archive_entry, &sarc, &first_buffer, &lib](const Archive<u64>::ArchiveEntry &sarc_entry) {
                        auto sarc_buffer = sarc.get(sarc_entry.key);

                        if (!sarc_buffer) {
                            GLog_Warning("Failed to read file {} from SARC {}",
                                         find_name(sarc_entry.key).value_or("Unknown"),
                                         find_name(archive_entry.key).value_or("Unknown")
                            );
                            return true;
                        }

                        sarc_buffer->read_exact<uint8>(first_buffer);
                        sarc_buffer->set_position(0);

                        if (std::memcmp(first_buffer.data(), ADF_MAGIC, 4) == 0) {
                            auto adf_file = ADF::ADFFile::from_buffer(std::move(sarc_buffer));
                            STI::register_types_from_adf(lib, adf_file);
                        }
                        return true;
                    });
            }
        }

        return true;
    });
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
    mount_game_archives(app_state);

    AssetDB db(argv[2]);
    AssetDB::set_instance(&db);

    STI::TypeLibrary type_library;

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

    collect_types(app_state, type_library);

    const auto [source_path, include_path] = adf_generator_output_paths();
    STI::generate_code(type_library, source_path, include_path);
    return 0;
}
