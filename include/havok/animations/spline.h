// Created by RED on 30.01.2026.

#ifndef APEXPREDATOR_SPLINE_H
#define APEXPREDATOR_SPLINE_H
#include "utils/dynamic_array.h"

#include "havok/generated/havok_generated.h"
#include "havok/animations/animation.h"
#include "havok/animations/track.h"

typedef struct SplineStaticVec3{
    Vec3 item;
} SplineStaticVec3;

typedef struct SplineDynamicVec3{
    u8 degree;
    u32 num_items;

    const u8 *knots;
    u32 knots_len;

    float *x;
    float *y;
    float *z;

    u8 x_dynamic, y_dynamic, z_dynamic;
} SplineDynamicVec3;

typedef struct SplineStaticQuat{
    Quat item;
} SplineStaticQuat;

typedef struct SplineDynamicQuat{
    u8 degree;
    u32 num_items;

    const u8 *knots;
    u32 knots_len;

    Quat *q;
} SplineDynamicQuat;

typedef struct TransformSplineBlock {
    const TransformMask *masks;
    u32 mask_count;
    TransformTrack *tracks;
    u32 track_count;
} TransformSplineBlock;

DYNAMIC_ARRAY_STRUCT(TransformSplineBlock, TransformSplineBlock);

typedef struct hkaSplineDecompressor {
    DynamicArray_TransformSplineBlock blocks;
} hkaSplineDecompressor;


void TransformSplineBlock_free(TransformSplineBlock *self);

bool TransformSplineBlock_assign(TransformSplineBlock *self, const uint8 *data,
                                 uint32 track_count,
                                 uint32 float_track_count);

void TransformSplineBlock_get_value(const TransformSplineBlock *self,
                                                  u32 trackID, float time,
                                                  QTransform *out);

void hkaSplineDecompressor_assign(hkaSplineDecompressor *self, const hkaSplineCompressedAnimation *input);

void hkaSplineDecompressor_free(hkaSplineDecompressor *self);


#endif //APEXPREDATOR_SPLINE_H
