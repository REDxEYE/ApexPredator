#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "havok/legacy/sdk2014/packfile.hpp"

namespace HavokTypes {
    std::unordered_map<std::string_view, Havok2014::ClassKind> &havok_type_info();

    // Havok 2013.2 packfile layouts; these are decoded values, not TAG0 types.
    struct hkStringPtr {
        std::string stringAndFlag;
        nlohmann::json to_json() const;
    };

    struct hkVector4f {
        std::array<float, 4> value{};
        nlohmann::json to_json() const;
    };

    struct hkQuaternionf {
        hkVector4f vec;
        nlohmann::json to_json() const;
    };

    struct hkQsTransform {
        hkVector4f translation;
        hkQuaternionf rotation;
        hkVector4f scale;
        nlohmann::json to_json() const;
    };

    struct hkaBone {
        hkStringPtr name;
        bool lockTranslation = false;
        nlohmann::json to_json() const;
    };

    struct hkaSkeleton_Partition {
        hkStringPtr name;
        int16_t startBoneIndex = 0;
        uint16_t numBones = 0;
        nlohmann::json to_json() const;
    };

    struct hkaSkeleton {
        uint16_t memSizeAndFlags = 0;
        uint16_t refCount = 0;
        hkStringPtr name;
        std::vector<int16_t> parentIndices;
        std::vector<hkaBone> bones;
        std::vector<hkQsTransform> referencePose;
        std::vector<float> referenceFloats;
        std::vector<hkStringPtr> floatSlots;
        std::vector<hkaSkeleton_Partition> partitions;
        // Nonempty localFrames are unsupported and rejected while decoding.
        nlohmann::json to_json() const;
    };

    void init_havok_type_info();
    std::vector<hkaSkeleton> read_skeletons(std::span<const uint8_t> bytes);
}
