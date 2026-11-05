#include <utility>

#include "apex/adf/adf.h"
#include "apex/adf/sti.h"
#include "apex/adf/mad_max_builtin_adf.hpp"

void register_builtin_adf_types(STI::TypeLibrary &lib) {
    for (const auto [data, size] : builtin_adfs) {
        auto adf = ADF::ADFFile::from_buffer(data, size);
        STI::register_types_from_adf(lib, adf);
    }
}

std::pair<const char *, const char *> adf_generator_output_paths() {
    return {"../modules/mad_max/src/apex/adf/generated",
            "../modules/mad_max/include/apex/adf/generated"};
}
