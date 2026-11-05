#include <utility>

std::pair<const char *, const char *> havok_generator_output_paths() {
    return {"../modules/mad_max/src/havok/tag_generated",
            "../modules/mad_max/include/havok/tag_generated"};
}
