#include "apex/rbmdl/lambert.hpp"
#include "redscore/platform/file/memory_file.h"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using Apex::RBMdl::LambertMaterialV4;

void check(bool condition) {
    if (!condition) throw std::runtime_error("Lambert material conversion regression");
}

void append_u32(std::vector<uint8> &bytes, uint32 value) {
    for (int i = 0; i < 4; ++i)
        bytes.push_back(static_cast<uint8>(value >> (8 * i)));
}

void append_f32(std::vector<uint8> &bytes, float value) {
    uint32 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    append_u32(bytes, bits);
}

void check_legacy_defaults(const LambertMaterialV4 &material) {
    check(material.vertex_format == 0 && material.position_scale == 1.0f);
    for (int i = 0; i < 4; ++i)
        check(material.uv_scale[i] == 1.0f && material.packed[i] == 0xFF);
    check(material.unknown_float == 1.0f);
    check(material.channel_selectors[0] == 0 && material.channel_selectors[1] == 0);
}
}

int main() {
    try {
        {
            std::vector<uint8> bytes;
            append_u32(bytes, 0x05);
            IO::MemoryViewFile file(bytes.data(), bytes.size());
            const auto material = LambertMaterialV4::read_v0(file);
            check_legacy_defaults(material);
            check(material.flags == 0x45 && material.depth_offset == 0.0f);
            check(file.get_position() == 4);
        }
        {
            std::vector<uint8> bytes;
            append_u32(bytes, 0x82);
            append_f32(bytes, -0.125f);
            IO::MemoryViewFile file(bytes.data(), bytes.size());
            const auto material = LambertMaterialV4::read_v2(file);
            check_legacy_defaults(material);
            check(material.flags == 0x82 && material.depth_offset == -0.125f);
            check(file.get_position() == 8);
        }
        {
            std::vector<uint8> bytes;
            append_u32(bytes, 0x102);
            append_f32(bytes, 0.5f);
            append_u32(bytes, 1);
            append_f32(bytes, 3.0f);
            for (float scale : {2.0f, 3.0f, 4.0f, 5.0f}) append_f32(bytes, scale);
            append_f32(bytes, 0.75f);
            bytes.insert(bytes.end(), {12, 34, 56, 78});
            IO::MemoryViewFile file(bytes.data(), bytes.size());
            const auto material = LambertMaterialV4::read_v3(file);
            check(material.flags == 0x102 && material.depth_offset == 0.5f);
            check(material.vertex_format == 1 && material.position_scale == 3.0f);
            for (int i = 0; i < 4; ++i) check(material.uv_scale[i] == i + 2.0f);
            check(material.unknown_float == 0.75f);
            check(material.packed[0] == 12 && material.packed[1] == 34 &&
                  material.packed[2] == 56 && material.packed[3] == 78);
            check(material.channel_selectors[0] == 0 && material.channel_selectors[1] == 0);
            check(file.get_position() == 40);
        }
        {
            std::vector<uint8> bytes;
            append_u32(bytes, 1);
            append_f32(bytes, 6.0f);
            for (float scale : {2.0f, 3.0f, 4.0f, 5.0f}) append_f32(bytes, scale);
            append_f32(bytes, 0.75f);
            bytes.insert(bytes.end(), {12, 34, 56, 78});
            append_u32(bytes, 0x180);
            append_f32(bytes, 0.25f);
            bytes.insert(bytes.end(), {2, 3, 7, 8});
            IO::MemoryViewFile file(bytes.data(), bytes.size());
            const auto material = LambertMaterialV4::read_v4(file);
            check(material.vertex_format == 1 && material.position_scale == 6.0f);
            check(material.flags == 0x180 && material.depth_offset == 0.25f);
            check(material.channel_selectors[0] == 2 && material.channel_selectors[1] == 3);
            check(material.trailing_unknown[0] == 7 && material.trailing_unknown[1] == 8);
            check(file.get_position() == 44);
        }
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
