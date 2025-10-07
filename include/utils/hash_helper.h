// Created by RED on 03.10.2025.

#ifndef APEXPREDATOR_HASH_HELPER_H
#define APEXPREDATOR_HASH_HELPER_H

#include "lookup3.h"
#include "string.h"
#define JENKINS_HL_LIT(s, seed) \
(hashlittle((const void*)(s), sizeof(s) - 1u, (uint32_t)(seed)))

/* Runtime (null-terminated string at runtime) */
static inline uint32_t jenkins_hl_str(const char *s, const uint32_t seed) {
    return hashlittle((const void*)s, strlen(s), seed);
}

/* Auto: pick literal vs runtime on GCC/Clang */
#if defined(__GNUC__) || defined(__clang__)
#  define JENKINS_HL(s, seed) \
(__builtin_constant_p(s) ? JENKINS_HL_LIT((s), (seed)) \
: jenkins_hl_str((s), (seed)))
#else
#  define JENKINS_HL(s, seed) jenkins_hl_str((s), (seed))
#endif

static inline uint32 hash_string(const String *str) {
    return hashlittle(String_data(str), str->size, 0);
}
static inline uint32 hash_cstring(const char *str) {
    return hashlittle(str, strlen(str), 0);
}

#endif //APEXPREDATOR_HASH_HELPER_H