#include <utility>

std::pair<const char *, const char *> havok_generator_output_paths() {
    return {"../modules/generation_zero/src/havok/generated",
            "../modules/generation_zero/include/havok/generated"};
}
