// Created by RED on 27.09.2025.

#ifndef APEXPREDATOR_BASE64_H
#define APEXPREDATOR_BASE64_H

#include "int_def.h"

static const char BASE64_TBL[64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
Computes the exact number of bytes required to Base64-encode a binary buffer,
excluding any optional trailing NUL. For an input of length n, the result is
ceil(n/3)*4. Returns 0 when n==0.
*/
static inline uint64 base64_encoded_size(size_t n) {
    return n ? (((n + 2) / 3) * 4) : 0;
}

/*
Encodes `src[0..len)` into Base64 and writes exactly base64_encoded_size(len)
bytes to `dst`. The output is not NUL-terminated. Returns the number of bytes
written. The caller must ensure `dst` has sufficient capacity.
*/
static inline size_t base64_encode(const uint8_t* src, size_t len, char* dst) {
    size_t i = 0, o = 0;
    while (i + 3 <= len) {
        uint32_t v = ((uint32_t)src[i] << 16) | ((uint32_t)src[i + 1] << 8) | (uint32_t)src[i + 2];
        dst[o++] = BASE64_TBL[(v >> 18) & 63];
        dst[o++] = BASE64_TBL[(v >> 12) & 63];
        dst[o++] = BASE64_TBL[(v >> 6) & 63];
        dst[o++] = BASE64_TBL[v & 63];
        i += 3;
    }
    if (len - i == 1) {
        uint32_t v = ((uint32_t)src[i] << 16);
        dst[o++] = BASE64_TBL[(v >> 18) & 63];
        dst[o++] = BASE64_TBL[(v >> 12) & 63];
        dst[o++] = '=';
        dst[o++] = '=';
    } else if (len - i == 2) {
        uint32_t v = ((uint32_t)src[i] << 16) | ((uint32_t)src[i + 1] << 8);
        dst[o++] = BASE64_TBL[(v >> 18) & 63];
        dst[o++] = BASE64_TBL[(v >> 12) & 63];
        dst[o++] = BASE64_TBL[(v >> 6) & 63];
        dst[o++] = '=';
    }
    return o;
}

#endif //APEXPREDATOR_BASE64_H