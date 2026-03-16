// Created by RED on 30.01.2026.

#ifndef APEXPREDATOR_SPLINE_H
#define APEXPREDATOR_SPLINE_H
#include "glm/glm.hpp"

#include "havok/generated/havok_types.h"

// Taken from PredatorCZ/Spike and PredatorCZ/HavokLib

enum class SplineTrackType { DYNAMIC, STATIC, IDENTITY };

enum class QuantizationType:uint32 {
    QT_8bit,
    QT_16bit,
    QT_32bit,
    QT_40bit,
    QT_48bit,
    QT_24bit,
    QT_16bitQuat,
    QT_Uncompressed,
};

enum class TransformType:uint32 {
    PosX,
    PosY,
    PosZ,
    Rotation,
    ScaleX,
    ScaleY,
    ScaleZ
};

struct TransformTypePos {
    static constexpr auto X = TransformType::PosX;
    static constexpr auto Y = TransformType::PosY;
    static constexpr auto Z = TransformType::PosZ;
};


struct TransformTypeScale {
    static constexpr auto X = TransformType::ScaleX;
    static constexpr auto Y = TransformType::ScaleY;
    static constexpr auto Z = TransformType::ScaleZ;
};

enum class FlagOffset : uint8 {
    staticX = 1 << 0,
    staticY = 1 << 1,
    staticZ = 1 << 2,
    staticW = 1 << 3,
    splineX = 1 << 4,
    splineY = 1 << 5,
    splineZ = 1 << 6,
    splineW = 1 << 7,
};

constexpr FlagOffset operator&(const FlagOffset a, const FlagOffset b) {
    return static_cast<FlagOffset>(std::to_underlying(a) & std::to_underlying(b));
}

constexpr bool has_flag(const FlagOffset value, const FlagOffset flag) {
    return std::to_underlying(value & flag) != 0;
}

struct TransformMask {
    uint8 quantizationTypes;
    FlagOffset positionTypes;
    uint8 rotationTypes;
    FlagOffset scaleTypes;

    [[nodiscard]] QuantizationType GetPosQuantizationType() const {
        return static_cast<QuantizationType>(quantizationTypes & 3);
    }

    [[nodiscard]] QuantizationType GetRotQuantizationType() const {
        return static_cast<QuantizationType>(((quantizationTypes >> 2) & 0xf) + 2);
    }

    [[nodiscard]] QuantizationType GetScaleQuantizationType() const {
        return static_cast<QuantizationType>((quantizationTypes >> 6) & 3);
    }

    [[nodiscard]] SplineTrackType GetSubTrackType(const TransformType type) const {
        switch (type) {
            case TransformType::PosX:
                if (has_flag(positionTypes, FlagOffset::staticX))
                    return SplineTrackType::STATIC;
                if (has_flag(positionTypes, FlagOffset::splineX))
                    return SplineTrackType::DYNAMIC;
                return SplineTrackType::IDENTITY;
            case TransformType::PosY:
                if (has_flag(positionTypes, FlagOffset::staticY))
                    return SplineTrackType::STATIC;
                if (has_flag(positionTypes, FlagOffset::splineY))
                    return SplineTrackType::DYNAMIC;
                return SplineTrackType::IDENTITY;
            case TransformType::PosZ:
                if (has_flag(positionTypes, FlagOffset::staticZ))
                    return SplineTrackType::STATIC;
                if (has_flag(positionTypes, FlagOffset::splineZ))
                    return SplineTrackType::DYNAMIC;
                return SplineTrackType::IDENTITY;
            case TransformType::Rotation:
                if (rotationTypes & 0xf0)
                    return SplineTrackType::DYNAMIC;
                if (rotationTypes & 0xf)
                    return SplineTrackType::STATIC;
                return SplineTrackType::IDENTITY;
            case TransformType::ScaleX:
                if (has_flag(scaleTypes, FlagOffset::staticX))
                    return SplineTrackType::STATIC;
                if (has_flag(scaleTypes, FlagOffset::splineX))
                    return SplineTrackType::DYNAMIC;
                return SplineTrackType::IDENTITY;
            case TransformType::ScaleY:
                if (has_flag(scaleTypes, FlagOffset::staticY))
                    return SplineTrackType::STATIC;
                if (has_flag(scaleTypes, FlagOffset::splineY))
                    return SplineTrackType::DYNAMIC;
                return SplineTrackType::IDENTITY;
            case TransformType::ScaleZ:
                if (has_flag(scaleTypes, FlagOffset::staticZ))
                    return SplineTrackType::STATIC;
                if (has_flag(scaleTypes, FlagOffset::splineZ))
                    return SplineTrackType::DYNAMIC;
                return SplineTrackType::IDENTITY;
        }

        return SplineTrackType::IDENTITY; // Warning fodder
    }
};

static_assert(sizeof(TransformMask) == 4, "TransformMask size is not 4");

template<class C>
struct ISplineTrack {
    virtual bool IsStatic() = 0;

    virtual C GetValue(float localFrame) = 0;

    virtual ~ISplineTrack() = default;
};

template<class C>
struct SplineStaticTrack : ISplineTrack<C> {
    C item;
    C GetValue(float) override { return item; }
    bool IsStatic() override { return true; }
};

struct SplineDynamicTrackVector : ISplineTrack<glm::vec3> {
    std::vector<float> tracks[3];
    std::vector<uint8> knots;
    uint8 degree;

    glm::vec3 GetValue(float localFrame) override;

    bool IsStatic() override { return knots.empty(); }
};

struct SplineDynamicTrackQuat : ISplineTrack<glm::vec4> {
    std::vector<glm::vec4> track;
    std::vector<uint8> knots;
    uint8 degree;

    glm::vec4 GetValue(float localFrame) override;

    bool IsStatic() override { return false; }
};

struct TransformTrack {
    template<class C>
    using TrackType = std::unique_ptr<ISplineTrack<C> >;
    TrackType<glm::vec3> pos;
    TrackType<glm::vec4> rotation;
    TrackType<glm::vec3> scale;
};

struct TransformSplineBlock {
    void Assign(IO::File &buffer, size_t numTracks, size_t numFloatTracks);

    [[nodiscard]] std::tuple<glm::vec3, glm::quat, glm::vec3> GetValue(size_t trackID, float time) const;

private:
    std::vector<TransformMask> masks;
    std::vector<TransformTrack> tracks;
};

struct hkaSplineDecompressor {
    std::vector<TransformSplineBlock> blocks;

    // TODO floats
    void Assign(const HavokTypes::hkaSplineCompressedAnimation *input);
};

inline std::tuple<glm::vec3, glm::quat, glm::vec3> TransformSplineBlock::GetValue(
    const size_t trackID, const float time) const {
    const auto tmp = tracks[trackID].rotation->GetValue(time);

    // GLM quat as XYZW, while PredatorCZ/HavokLib quats are WXYZ
    glm::quat rotation = {tmp.w, tmp.x, tmp.y, tmp.z};
    glm::vec3 translation = tracks[trackID].pos->GetValue(time);
    glm::vec3 scale = tracks[trackID].scale->GetValue(time);

    return {translation, rotation, scale};
}

#endif //APEXPREDATOR_SPLINE_H
