#include "tools/collector_policy.h"

#include <array>

CollectorPolicy collector_policy() {
    static constexpr std::array strings{
        std::string_view{"./../strings/mad_max/mad_max_exe_strings.txt"}
    };
    static constexpr std::array gtocs{std::string_view{"sarc.0.gtoc"}};
    return {"./../hashes.db", strings, gtocs};
}
