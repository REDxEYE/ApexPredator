#include <utility>

std::pair<const char *, const char *> havok_generator_output_paths() {
    return {"../modules/second_extinction/src/havok/generated",
            "../modules/second_extinction/include/havok/generated"};
}
