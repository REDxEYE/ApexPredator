// Created by RED on 30.01.2026.

#ifndef APEXPREDATOR_TRACK_H
#define APEXPREDATOR_TRACK_H

#include "havok/animations/animation.h"

typedef struct TrackBBOX {
    float min;
    float max;
} TrackBBOX;

typedef enum TrackKind {
    TRACK_VEC3_STATIC = 1,
    TRACK_VEC3_SPLINE_DYNAMIC = 2,
    TRACK_QUAT_STATIC = 3,
    TRACK_QUAT_SPLINE_DYNAMIC = 4,
} TrackKind;

typedef struct Track {
    TrackKind kind;
    void *impl;
} Track;

typedef struct TransformTrack {
    Track position;
    Track rotation;
    Track scale;
} TransformTrack;

void Track_free(Track *t);

void Track_get_value_vec3(Track *t, float time, vec3 out);

void Track_get_value_quat(Track *t, float time, versor out);

#endif //APEXPREDATOR_TRACK_H
