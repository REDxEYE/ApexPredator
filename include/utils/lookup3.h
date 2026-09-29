// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_LOOKUP3_H
#define APEXPREDATOR_LOOKUP3_H
#include <string_view>

#include "int_def.h"
#include "stdint.h"
#include "stddef.h"

uint32_t hashlittle(const void *key, size_t length, uint32_t initval);

constexpr uint32_t lookup3_rot(uint32_t x, uint32_t k) noexcept {
    return (x << k) | (x >> (32 - k));
}

constexpr void lookup3_mix(
    uint32_t& a,
    uint32_t& b,
    uint32_t& c
) noexcept {
    a -= c; a ^= lookup3_rot(c, 4);  c += b;
    b -= a; b ^= lookup3_rot(a, 6);  a += c;
    c -= b; c ^= lookup3_rot(b, 8);  b += a;
    a -= c; a ^= lookup3_rot(c, 16); c += b;
    b -= a; b ^= lookup3_rot(a, 19); a += c;
    c -= b; c ^= lookup3_rot(b, 4);  b += a;
}

constexpr void lookup3_final(
    uint32_t& a,
    uint32_t& b,
    uint32_t& c
) noexcept {
    c ^= b; c -= lookup3_rot(b, 14);
    a ^= c; a -= lookup3_rot(c, 11);
    b ^= a; b -= lookup3_rot(a, 25);
    c ^= b; c -= lookup3_rot(b, 16);
    a ^= c; a -= lookup3_rot(c, 4);
    b ^= a; b -= lookup3_rot(a, 14);
    c ^= b; c -= lookup3_rot(b, 24);
}

constexpr uint32_t const_hashlittle(
    const char* key,
    std::size_t length,
    uint32_t initval = 0
) noexcept {
    auto byte = [](const char* p, std::size_t i) constexpr -> uint32_t {
        return static_cast<unsigned char>(p[i]);
    };

    uint32_t a = 0xdeadbeef +
                 static_cast<uint32_t>(length) +
                 initval;
    uint32_t b = a;
    uint32_t c = a;

    while (length > 12) {
        a += byte(key, 0);
        a += byte(key, 1) << 8;
        a += byte(key, 2) << 16;
        a += byte(key, 3) << 24;

        b += byte(key, 4);
        b += byte(key, 5) << 8;
        b += byte(key, 6) << 16;
        b += byte(key, 7) << 24;

        c += byte(key, 8);
        c += byte(key, 9) << 8;
        c += byte(key, 10) << 16;
        c += byte(key, 11) << 24;

        lookup3_mix(a, b, c);

        key += 12;
        length -= 12;
    }

    switch (length) {
        case 12: c += byte(key, 11) << 24; [[fallthrough]];
        case 11: c += byte(key, 10) << 16; [[fallthrough]];
        case 10: c += byte(key, 9) << 8;   [[fallthrough]];
        case 9:  c += byte(key, 8);        [[fallthrough]];
        case 8:  b += byte(key, 7) << 24;  [[fallthrough]];
        case 7:  b += byte(key, 6) << 16;  [[fallthrough]];
        case 6:  b += byte(key, 5) << 8;   [[fallthrough]];
        case 5:  b += byte(key, 4);        [[fallthrough]];
        case 4:  a += byte(key, 3) << 24;  [[fallthrough]];
        case 3:  a += byte(key, 2) << 16;  [[fallthrough]];
        case 2:  a += byte(key, 1) << 8;   [[fallthrough]];
        case 1:  a += byte(key, 0); break;
        case 0:  return c;
    }

    lookup3_final(a, b, c);
    return c;
}
#endif //APEXPREDATOR_LOOKUP3_H
