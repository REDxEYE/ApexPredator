// Created by RED on 30.01.2026.

#ifndef APEXPREDATOR_ANIMATION_H
#define APEXPREDATOR_ANIMATION_H
#include "int_def.h"

#include "cglm/cglm.h"

typedef vec3 Vec3;
typedef versor Quat;

typedef struct QTransform {
    vec3   translation;
    versor rotation;
    vec3   scale;
} QTransform;

typedef enum FlagOffset {
    staticX = 0,
    staticY = 1,
    staticZ = 2,
    staticW = 3,
    splineX = 4,
    splineY = 5,
    splineZ = 6,
    splineW = 7
} FlagOffset;

typedef enum SplineTrackType {
    STT_IDENTITY = 0,
    STT_STATIC = 1,
    STT_DYNAMIC = 2
} SplineTrackType;

typedef enum QuantizationType {
    QT_8bit,
    QT_16bit,
    QT_32bit,
    QT_40bit,
    QT_48bit,
    QT_24bit,
    QT_16bitQuat,
    QT_Uncompressed,
} QuantizationType;

typedef enum TransformType {
    ttPosX,
    ttPosY,
    ttPosZ,
    ttRotation,
    ttScaleX,
    ttScaleY,
    ttScaleZ
} TransformType;

typedef struct TransformMask {
    u8 quantization_types;
    u8 position_types; /* bitflags FlagOffset */
    u8 rotation_types;
    u8 scale_types; /* bitflags FlagOffset */
} TransformMask;


QuantizationType TransformMask_GetPosQuantizationType(const TransformMask *m);

QuantizationType TransformMask_GetRotQuantizationType(const TransformMask *m);

QuantizationType TransformMask_GetScaleQuantizationType(const TransformMask *m);

SplineTrackType TransformMask_GetSubTrackType(const TransformMask *m, TransformType type);

/* convenience wrappers matching my earlier assign() code shape */
SplineTrackType TransformMask_GetSubTrackType_Pos(const TransformMask *m, int axis);

SplineTrackType TransformMask_GetSubTrackType_Scale(const TransformMask *m, int axis);

SplineTrackType TransformMask_GetSubTrackType_Rotation(const TransformMask *m);


#endif //APEXPREDATOR_ANIMATION_H
