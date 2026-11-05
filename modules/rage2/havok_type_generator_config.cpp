#include <utility>

std::pair<const char *, const char *> havok_generator_output_paths() {
    return {"../modules/rage2/src/havok/generated",
            "../modules/rage2/include/havok/generated"};
}
