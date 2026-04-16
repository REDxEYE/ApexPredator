// Created by RED on 30.01.2026.

// Taken from PredatorCZ/Spike and PredatorCZ/HavokLib

#include <cstring>
#include <ranges>
#include <numbers>

#include "havok/animations/spline.h"

constexpr float GetFraction(const size_t num_bits) {
    return 1.f / static_cast<float>((1ULL << num_bits) - 1);
}

glm::vec4 Read32Quat(IO::File &buffer) {
    constexpr uint32 rMask = (1u << 10) - 1u;
    constexpr float rFrac = GetFraction(10);
    constexpr float fPI2 = 0.50f * std::numbers::pi_v<float>;
    constexpr float fPI4 = 0.25f * std::numbers::pi_v<float>;
    constexpr float phiFrac = fPI2 / 511.0f;

    const auto cVal = buffer.read_pod<uint32>();

    float R = static_cast<float>((cVal >> 18) & rMask) * rFrac;
    R = 1.0f - (R * R);

    const auto phiTheta = static_cast<float>(cVal & 0x3FFFFu);

    float phi = floorf(sqrtf(phiTheta));
    float theta = 0.0f;

    if (phi > 0.0f) {
        theta = fPI4 * (phiTheta - (phi * phi)) / phi;
        phi = phiFrac * phi;
    }

    const float magnitude = sqrtf(1.0f - R * R);
    const float sPhi = sinf(phi);
    const float cPhi = cosf(phi);
    const float sTheta = sinf(theta);
    const float cTheta = cosf(theta);

    const glm::vec4 retVal0 =
            glm::vec4(sPhi, sPhi, cPhi, R) *
            glm::vec4(cTheta, sTheta, 1.0f, 1.0f) *
            glm::vec4(magnitude, magnitude, magnitude, 1.0f);

    const glm::bvec4 negate(
        (cVal & 0x10000000u) != 0,
        (cVal & 0x20000000u) != 0,
        (cVal & 0x40000000u) != 0,
        (cVal & 0x80000000u) != 0
    );

    return glm::mix(retVal0, -retVal0, negate);
}

static constexpr glm::uvec4 shr(const glm::uvec4 &v, const uint32_t bits) {
    return {v.x >> bits, v.y >> bits, v.z >> bits, v.w >> bits};
}

static glm::vec4 compute_w(glm::vec4 v) {
    const float dot = glm::dot(v, v);
    const float res0 = 1.0f - dot;
    const float res1 = res0 > 0.0001f ? std::sqrt(res0) : 0.0f;
    return {v.x, v.y, v.z, res1};
}

glm::vec4 Read40Quat(IO::File &buffer) {
    constexpr float fractal = 0.000345436f;
    constexpr glm::vec4 fract(fractal, fractal, fractal, 0.0f);

    uint64_t cVal0 =
            static_cast<uint64_t>(buffer.read_pod<uint8>()) |
            (static_cast<uint64_t>(buffer.read_pod<uint8>()) << 8) |
            (static_cast<uint64_t>(buffer.read_pod<uint8>()) << 16) |
            (static_cast<uint64_t>(buffer.read_pod<uint8>()) << 24) |
            (static_cast<uint64_t>(buffer.read_pod<uint8>()) << 32);

    const auto cVal1 = static_cast<uint32_t>(cVal0 >> 24);

    const glm::uvec4 packed(
        static_cast<uint32_t>(cVal0),
        static_cast<uint32_t>(cVal0),
        cVal1,
        0u
    );

    constexpr glm::uvec4 mul(1u << 20, 1u << 8, 1u << 20, 0u);

    const glm::uvec4 tmpVal = shr(packed * mul, 20);
    const glm::ivec4 tmpVal1 = glm::ivec4(tmpVal) - glm::ivec4((1 << 11) + 1);
    const glm::vec4 tmpVal2 = compute_w(glm::vec4(tmpVal1) * fract);

    const size_t resultShift = cVal0 >> 36 & 3;
    const glm::vec4 wmul(1.0f, 1.0f, 1.0f, ((cVal0 >> 38) & 1) ? -1.0f : 1.0f);
    const glm::vec4 retVal = wmul * tmpVal2;

    switch (resultShift) {
        case 0:
            return {retVal.w, retVal.x, retVal.y, retVal.z};
        case 1:
            return {retVal.x, retVal.w, retVal.y, retVal.z};
        case 2:
            return {retVal.x, retVal.y, retVal.w, retVal.z};
        default:
            return retVal;
    }
}

glm::vec4 Read48Quat(IO::File &buffer) {
    constexpr int mask = (1 << 15) - 1;
    constexpr float fractal = 0.000043161f;
    constexpr glm::vec4 fract(fractal, fractal, fractal, 0.0f);

    static_assert(sizeof(glm::i16vec3) == 6, "Invalid size");
    auto cVal = buffer.read_pod<glm::i16vec3>();

    const int resultShift = ((static_cast<uint16_t>(cVal.y) >> 14) & 2) |
                            ((static_cast<uint16_t>(cVal.x) >> 15) & 1);
    const bool rSign = (static_cast<uint16_t>(cVal.z) >> 15) != 0;

    const glm::ivec4 retVal0(cVal.x, cVal.y, cVal.z, 0);
    glm::vec4 retVal1 =
            glm::vec4((retVal0 & glm::ivec4(mask)) - glm::ivec4(mask >> 1)) * fract;

    retVal1 = compute_w(retVal1);

    const glm::vec4 wmul(1.0f, 1.0f, 1.0f, rSign ? -1.0f : 1.0f);
    const glm::vec4 retVal = wmul * retVal1;

    switch (resultShift) {
        case 0:
            return {retVal.w, retVal.x, retVal.y, retVal.z};
        case 1:
            return {retVal.x, retVal.w, retVal.y, retVal.z};
        case 2:
            return {retVal.x, retVal.y, retVal.w, retVal.z};
        default:
            return retVal;
    }
}

glm::vec4 ReadQuat(const QuantizationType qType, IO::File &buffer) {
    switch (qType) {
        case QuantizationType::QT_32bit:
            return Read32Quat(buffer);
        case QuantizationType::QT_40bit:
            return Read40Quat(buffer);
        case QuantizationType::QT_48bit:
            return Read48Quat(buffer);
        case QuantizationType::QT_Uncompressed:
            return buffer.read_pod<glm::vec4>();
        default:
            return {0.0f, 0.0f, 0.0f, 1.0f};
    }
}

// Algorithm A2.1 The NURBS Book 2nd edition, page 68
int FindKnotSpan(const int degree, const float value, const int cPointsSize,
                 const std::span<const uint8> &knots) {
    if (value >= static_cast<float>(knots[cPointsSize]))
        return cPointsSize - 1;

    int low = degree;
    int high = cPointsSize;
    int mid = (low + high) / 2;

    while (value < static_cast<float>(knots[mid]) || value >= static_cast<float>(knots[mid + 1])) {
        if (value < static_cast<float>(knots[mid]))
            high = mid;
        else
            low = mid;

        mid = (low + high) / 2;
    }

    return mid;
}

// Basis_ITS1, GetPoint_NR1, TIME-EFFICIENT NURBS CURVE EVALUATION ALGORITHMS,
// pages 64 & 65
template<class C>
C GetSinglePoint(const int knotSpanIndex, const int degree, const float frame,
                 const std::span<const uint8> &knots, std::vector<C> &cPoints) {
    float N[5] = {1.0f};

    for (int i = 1; i <= degree; i++)
        for (int j = i - 1; j >= 0; j--) {
            const float denom = static_cast<float>(knots[knotSpanIndex + i - j]) - static_cast<float>(knots[
                                    knotSpanIndex - j]);
            const float A = (denom != 0.0f) ? ((frame - static_cast<float>(knots[knotSpanIndex - j])) / denom) : 0.0f;
            const float tmp = N[j] * A;
            N[j + 1] += N[j] - tmp;
            N[j] = tmp;
        }

    C retVal{0};

    for (int i = 0; i <= degree; i++)
        retVal += cPoints[knotSpanIndex - i] * N[i];

    return retVal;
}

glm::vec4 SplineDynamicTrackQuat::GetValue(const float localFrame) {
    int knotSpan = FindKnotSpan(degree, localFrame, static_cast<int>(track.size()), knots);
    return GetSinglePoint(knotSpan, degree, localFrame, knots, track);
}

glm::vec3 SplineDynamicTrackVector::GetValue(const float localFrame) {
    glm::vec3 out;
    int knotSpan = -1;

    int cSize = static_cast<int>(tracks[0].size());

    if (cSize == 1)
        out.x = tracks[0][0];
    else {
        knotSpan = FindKnotSpan(degree, localFrame, cSize, knots);
        out.x = GetSinglePoint(knotSpan, degree, localFrame, knots, tracks[0]);
    }

    cSize = static_cast<int>(tracks[1].size());

    if (cSize == 1)
        out.y = tracks[1][0];
    else {
        if (knotSpan < 0)
            knotSpan = FindKnotSpan(degree, localFrame, cSize, knots);

        out.y = GetSinglePoint(knotSpan, degree, localFrame, knots, tracks[1]);
    }

    cSize = static_cast<int>(tracks[2].size());

    if (cSize == 1)
        out.z = tracks[2][0];
    else {
        if (knotSpan < 0)
            knotSpan = FindKnotSpan(degree, localFrame, cSize, knots);

        out.z = GetSinglePoint(knotSpan, degree, localFrame, knots, tracks[2]);
    }

    return out;
}

void ApplyPadding(IO::File &buffer, const int alignment = 4) {
    const size_t iterPos = buffer.get_position();
    const size_t result = iterPos & (alignment - 1);
    if (!result)
        return;
    buffer.skip(alignment - result);
}

void ApplyPadding(const char *&buffer, const int alignment = 4) {
    const size_t iterPos = reinterpret_cast<intptr_t>(buffer);
    const size_t result = iterPos & (alignment - 1);

    if (!result)
        return;

    buffer += alignment - result;
}

struct TrackBBOX {
    float min, max;
};

void TransformSplineBlock::Assign(IO::File &buffer, size_t numTracks,
                                  const size_t numFloatTracks) {
    masks.resize(numTracks);
    buffer.read_exact(masks);
    buffer.skip(numFloatTracks);
    ApplyPadding(buffer);

    tracks.resize(numTracks);

    for (auto [m, track]: std::views::zip(masks, tracks)) {
        auto MakeTrack = [&]<typename stype>(const QuantizationType qtype, const float defVal,
                                             stype) -> TransformTrack::TrackType<glm::vec3> {
            const bool useSpline =
                    m.GetSubTrackType(stype::X) == SplineTrackType::DYNAMIC ||
                    m.GetSubTrackType(stype::Y) == SplineTrackType::DYNAMIC ||
                    m.GetSubTrackType(stype::Z) == SplineTrackType::DYNAMIC;

            if (useSpline) {
                auto sTrack = std::make_unique<SplineDynamicTrackVector>();
                const auto numItems = buffer.read_pod<uint16>();
                sTrack->degree = buffer.read_pod<uint8>();
                const size_t bufferSkip = numItems + sTrack->degree + 2;
                sTrack->knots.resize(bufferSkip);
                buffer.read_exact(sTrack->knots);
                ApplyPadding(buffer);

                TrackBBOX extremes[3] = {};

                auto MakeSubTrack = [&](auto type, const size_t id) {
                    const auto ttype =
                            m.GetSubTrackType(static_cast<TransformType>(type));

                    if (ttype == SplineTrackType::DYNAMIC) {
                        extremes[id] = buffer.read_pod<TrackBBOX>();
                        sTrack->tracks[id].resize(numItems + 1);
                    }
                    else if (ttype == SplineTrackType::STATIC) {
                        sTrack->tracks[id].push_back(buffer.read_pod<float32>());
                    }
                    else {
                        sTrack->tracks[id].push_back(defVal);
                    }
                };

                MakeSubTrack(stype::X, 0);
                MakeSubTrack(stype::Y, 1);
                MakeSubTrack(stype::Z, 2);

                auto UnpackPoints8 = [&](auto type, const size_t id, const size_t sid) {
                    constexpr float fractal = 1.0f / 255.0f;
                    const auto ttype = m.GetSubTrackType(static_cast<TransformType>(type));
                    if (ttype == SplineTrackType::DYNAMIC) {
                        const float dVar = static_cast<float>(buffer.read_pod<uint8>()) * fractal;
                        sTrack->tracks[id][sid] = extremes[id].min + (extremes[id].max - extremes[id].min) * dVar;
                    }
                };

                auto UnpackPoints16 = [&](auto type, const size_t id, const size_t sid) {
                    constexpr float fractal = 1.0f / 0xffff;
                    const auto ttype = m.GetSubTrackType(static_cast<TransformType>(type));
                    if (ttype == SplineTrackType::DYNAMIC) {
                        const float dVar = static_cast<float>(buffer.read_pod<uint16>()) * fractal;
                        sTrack->tracks[id][sid] = extremes[id].min + (extremes[id].max - extremes[id].min) * dVar;
                    }
                };

                if (qtype == QuantizationType::QT_8bit) {
                    for (int t = 0; t <= numItems; t++) {
                        UnpackPoints8(stype::X, 0, t);
                        UnpackPoints8(stype::Y, 1, t);
                        UnpackPoints8(stype::Z, 2, t);
                    }
                }
                else {
                    for (int t = 0; t <= numItems; t++) {
                        UnpackPoints16(stype::X, 0, t);
                        UnpackPoints16(stype::Y, 1, t);
                        UnpackPoints16(stype::Z, 2, t);
                    }
                }

                ApplyPadding(buffer);
                return sTrack;
            }

            auto sTrack = std::make_unique<SplineStaticTrack<glm::vec3> >();
            auto MakeSubTrack = [&](auto type, const int32 id) {
                const auto ttype = m.GetSubTrackType(type);

                if (ttype == SplineTrackType::STATIC) {
                    sTrack->item[id] = buffer.read_pod<float32>();
                }
                else {
                    sTrack->item[id] = defVal;
                }
            };

            MakeSubTrack(stype::X, 0);
            MakeSubTrack(stype::Y, 1);
            MakeSubTrack(stype::Z, 2);

            return sTrack;
        };

        track.pos = MakeTrack(m.GetPosQuantizationType(), 0.f, TransformTypePos{});

        if (m.GetSubTrackType(TransformType::Rotation) == SplineTrackType::DYNAMIC) {
            const auto rTrack = new SplineDynamicTrackQuat();
            track.rotation = TransformTrack::TrackType<glm::vec4>(rTrack);
            const auto numItems = buffer.read_pod<uint16>();
            rTrack->degree = buffer.read_pod<uint8>();
            const size_t knot_count = numItems + rTrack->degree + 2;
            rTrack->knots.resize(knot_count);
            buffer.read_exact(rTrack->knots);

            const QuantizationType quantType = m.GetRotQuantizationType();

            if (quantType == QuantizationType::QT_48bit || quantType == QuantizationType::QT_16bitQuat)
                ApplyPadding(buffer, 2);
            else if (quantType == QuantizationType::QT_32bit || quantType == QuantizationType::QT_Uncompressed)
                ApplyPadding(buffer);

            rTrack->track.resize(numItems + 1);

            for (int t = 0; t <= numItems; t++) {
                rTrack->track[t] = ReadQuat(quantType, buffer);
            }
        }
        else {
            auto rTrack = std::make_unique<SplineStaticTrack<glm::vec4> >();

            if (m.GetSubTrackType(TransformType::Rotation) == SplineTrackType::STATIC) {
                rTrack->item = ReadQuat(m.GetRotQuantizationType(), buffer);
            }
            else {
                rTrack->item.w = 1.0f;
            }

            track.rotation = std::move(rTrack);
        }

        ApplyPadding(buffer);

        track.scale = MakeTrack(m.GetScaleQuantizationType(), 1.f, TransformTypeScale{});
    }
}

void hkaSplineDecompressor::Assign(
    const HavokTypes::hkaSplineCompressedAnimation *input) {
    const auto blockOffsets = input->blockOffsets;
    const auto data_buffer = IO::Buffer::wrap(input->data);
    blocks.resize(blockOffsets.size());
    int cBlock = 0;

    for (auto &b: blocks) {
        IO::MemoryViewFile block_view(data_buffer.readonly_view(blockOffsets[cBlock]));
        b.Assign(block_view, input->numberOfTransformTracks, input->numberOfFloatTracks);
        cBlock++;
    }
}
