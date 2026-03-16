// Created by RED on 20.09.2025.

#ifndef APEXPREDATOR_STI_SHARED_H
#define APEXPREDATOR_STI_SHARED_H

#include "int_def.h"
#include <format>

enum class CompType: uint8{
    None = 0,
    zlib = 1,
    lz4f = 2,
    zstd = 3
} ;

template<>
struct std::formatter<CompType> : std::formatter<std::string_view> {
    auto format(const CompType comp_type, std::format_context& ctx) const {
        std::string_view name;
        switch (comp_type) {
            case CompType::None: name = "None"; break;
            case CompType::zlib: name = "zlib"; break;
            case CompType::lz4f: name = "lz4f"; break;
            case CompType::zstd: name = "zstd"; break;
            default: name = "Unknown"; break;
        }
        return std::formatter<std::string_view>::format(name, ctx);
    }
};

struct CompressedHeader{
    char ident[4];
    uint8 a;
    CompType comp_type;
    uint8 c;
    uint8 d;
    uint64 decomp_size;
} ;


#endif //APEXPREDATOR_STI_SHARED_H
