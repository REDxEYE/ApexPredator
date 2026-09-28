#include "commands.h"

int main(int argc, const char *argv[]) {
    CLI::App app{"ApexPredator modular asset tools"};
    app.require_subcommand(1);
    ExtractCommand extract("extract", "Extract assets using a game module.");
    ExtractAnimationCommand animations("extract-anims", "Export animations using a game module.");
    SearchCommand search("search", "Search an asset database using a game module.");
    ModulesCommand modules("modules", "List game modules and optionally probe a game root.");
    ConvertCommand convert("convert", "Convert textures (not implemented).");
    ExtractEverythingCommand everything("extract-all", "Extract all assets (not implemented).");
    extract.register_(app); animations.register_(app); search.register_(app); modules.register_(app);
    convert.register_(app); everything.register_(app);
    try { app.parse(argc, argv); }
    catch (const CLI::ParseError &e) { return app.exit(e); }
    catch (const std::exception &e) { std::cerr << "Error: " << e.what() << '\n'; return 1; }
    return 0;
}
