#include "tools/collector_policy.h"

#include <array>

CollectorPolicy collector_policy() {
    static constexpr std::array strings{
        std::string_view{"./../strings/generation_zero/file_locations.txt"},
        std::string_view{"./../strings/generation_zero/filenames.txt"},
        std::string_view{"./../strings/generation_zero/cross_game.txt"},
        std::string_view{"./../strings/generation_zero/game_dump_clean.txt"}
    };
    static constexpr std::array gtocs{std::string_view{"sarc.0.gtoc"}};
    return {"./../hashes.db", strings, gtocs};
}
