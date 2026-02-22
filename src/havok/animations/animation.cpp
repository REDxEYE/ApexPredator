// Created by RED on 30.01.2026.

#include "havok/animations/animation.h"

#include <stdbool.h>


static bool flags_test(const u8 flags, const FlagOffset bit) {
    return (flags >> (u8)bit) & 1u;
}

QuantizationType TransformMask_GetPosQuantizationType(const TransformMask *m) {
    return (QuantizationType) (m->quantization_types & 3u);
}

QuantizationType TransformMask_GetRotQuantizationType(const TransformMask *m) {
    return (QuantizationType) ((((u8) (m->quantization_types >> 2)) & 0x0Fu) + 2u);
}

QuantizationType TransformMask_GetScaleQuantizationType(const TransformMask *m) {
    return (QuantizationType) ((u8) (m->quantization_types >> 6) & 3u);
}

SplineTrackType TransformMask_GetSubTrackType(const TransformMask *m, TransformType type) {
    switch (type) {
        case ttPosX:
            return flags_test(m->position_types, staticX)
                       ? STT_STATIC
                       : flags_test(m->position_types, splineX)
                             ? STT_DYNAMIC
                             : STT_IDENTITY;
        case ttPosY:
            return flags_test(m->position_types, staticY)
                       ? STT_STATIC
                       : flags_test(m->position_types, splineY)
                             ? STT_DYNAMIC
                             : STT_IDENTITY;
        case ttPosZ:
            return flags_test(m->position_types, staticZ)
                       ? STT_STATIC
                       : flags_test(m->position_types, splineZ)
                             ? STT_DYNAMIC
                             : STT_IDENTITY;

        case ttRotation:
            if (m->rotation_types & 0xF0u)
                return STT_DYNAMIC;
            if (m->rotation_types & 0x0Fu)
                return STT_STATIC;
            return STT_IDENTITY;

        case ttScaleX:
            return flags_test(m->scale_types, staticX)
                       ? STT_STATIC
                       : flags_test(m->scale_types, splineX)
                             ? STT_DYNAMIC
                             : STT_IDENTITY;
        case ttScaleY:
            return flags_test(m->scale_types, staticY)
                       ? STT_STATIC
                       : flags_test(m->scale_types, splineY)
                             ? STT_DYNAMIC
                             : STT_IDENTITY;
        case ttScaleZ:
            return flags_test(m->scale_types, staticZ)
                       ? STT_STATIC
                       : flags_test(m->scale_types, splineZ)
                             ? STT_DYNAMIC
                             : STT_IDENTITY;
        default:
            return STT_IDENTITY;
    }
}

SplineTrackType TransformMask_GetSubTrackType_Pos(const TransformMask *m, int axis) {
    return TransformMask_GetSubTrackType(m, (TransformType) (ttPosX + axis));
}

SplineTrackType TransformMask_GetSubTrackType_Scale(const TransformMask *m, int axis) {
    return TransformMask_GetSubTrackType(m, (TransformType) (ttScaleX + axis));
}

SplineTrackType TransformMask_GetSubTrackType_Rotation(const TransformMask *m) {
    return TransformMask_GetSubTrackType(m, ttRotation);
}
