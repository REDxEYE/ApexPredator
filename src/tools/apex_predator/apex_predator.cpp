#include "redscore/utils/memory_tracker.h"
#include "commands.h"
#include "apex/adf/generated/adf_types.h"

#include "CLI/CLI.hpp"
#include "havok/generated/havok_types.h"
#include "tracy/Tracy.hpp"


int main(int argc, const char *argv[]) {
    //     while (!TracyIsConnected) {
    // #ifdef _WIN32
    //         Sleep(100); /* Windows */
    // #else
    //         usleep(10000);
    // #endif
    //         printf("\rWaiting for tracy;");
    //     }
    //     printf("\n");


    mp_init();
    init_havok_type_info();
    init_adf_type_info();


    CLI::App app{"ApexPredator asset tools"};
    app.require_subcommand(1);

    ExtractCommand export_command("extract", "Extract assets.");
    export_command.register_(app);

    /* ---------------- extract-anims ---------------- */

    ExtractAnimationCommand extract_anims_command("extract-anims",
                                                  "Extract animations from a Havok animation container.");
    extract_anims_command.register_(app);

    /* ---------------- search ---------------- */

    SearchCommand search_command("search", "Search for assets by hash or path pattern.");
    search_command.register_(app);

    /* ---------------- convert ---------------- */

    ConvertCommand convert_command("convert", "Convert png->ddsc");
    convert_command.register_(app);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }
    // } catch (const std::exception &e) {
    //     std::cerr << "ApexPredator crashed!" << std::endl;
    //     std::cerr << "Cause: " << e.what() << std::endl;
    //     return 1;
    // }

    mp_shutdown();
    return 0;
}
