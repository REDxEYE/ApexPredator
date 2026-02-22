// Created by RED on 30.01.2026.

#include "havok/animations/track.h"

#include "havok/animations/spline.h"
#include "utils/memory_profiling.h"
#include "cglm/cglm.h"

void Track_free(Track *t) {
    if (!t || !t->impl) return;

    switch (t->kind) {
        case TrackKind::TRACK_VEC3_STATIC:
            mp_free(t->impl);
            break;
        case TrackKind::TRACK_VEC3_SPLINE_DYNAMIC: {
            SplineDynamicVec3 *s = (SplineDynamicVec3 *) t->impl;
            mp_free(s->x);
            mp_free(s->y);
            mp_free(s->z);
            mp_free(s);
        }
        break;
        case TrackKind::TRACK_QUAT_STATIC:
            mp_free(t->impl);
            break;
        case TrackKind::TRACK_QUAT_SPLINE_DYNAMIC: {
            SplineDynamicQuat *r = (SplineDynamicQuat *) t->impl;
            mp_free(r->q);
            mp_free(r);
        }
        break;
        default:
            mp_free(t->impl);
            break;
    }

    t->impl = NULL;
    t->kind = TrackKind::None;
}

static int FindKnotSpan(int degree, float value, int cPointsSize, const u8 *knots) {
    if (value >= (float)knots[cPointsSize]) return cPointsSize - 1;

    int low = degree, high = cPointsSize;
    int mid = (low + high) / 2;

    while (value < (float)knots[mid] || value >= (float)knots[mid + 1]) {
        if (value < (float)knots[mid]) high = mid;
        else low = mid;
        mid = (low + high) / 2;
    }
    return mid;
}

static float GetSinglePoint_f32(int knotSpanIndex, int degree, float frame,
                                const u8 *knots, const float *cPoints) {
    float N[5] = { 1.0f, 0, 0, 0, 0 };

    for (int i = 1; i <= degree; i++) {
        for (int j = i - 1; j >= 0; j--) {
            float denom = (float)knots[knotSpanIndex + i - j] - (float)knots[knotSpanIndex - j];
            float A = (denom != 0.0f) ? ((frame - (float)knots[knotSpanIndex - j]) / denom) : 0.0f;
            float tmp = N[j] * A;
            N[j + 1] += N[j] - tmp;
            N[j] = tmp;
        }
    }

    float ret = 0.0f;
    for (int i = 0; i <= degree; i++)
        ret += cPoints[knotSpanIndex - i] * N[i];

    return ret;
}

static void GetSinglePoint_quat(int knotSpanIndex, int degree, float frame,
                                const u8 *knots, versor *cPoints,
                                versor out_q) {
    float N[5] = { 1.0f, 0, 0, 0, 0 };

    for (int i = 1; i <= degree; i++) {
        for (int j = i - 1; j >= 0; j--) {
            float denom = (float)knots[knotSpanIndex + i - j] - (float)knots[knotSpanIndex - j];
            float A = (denom != 0.0f) ? ((frame - (float)knots[knotSpanIndex - j]) / denom) : 0.0f;
            float tmp = N[j] * A;
            N[j + 1] += N[j] - tmp;
            N[j] = tmp;
        }
    }

    versor acc = {};
    glm_vec4_zero(acc);

    for (int i = 0; i <= degree; i++) {
        versor tmp;
        glm_vec4_scale(cPoints[knotSpanIndex - i], N[i], tmp);
        glm_quat_add(acc, tmp, acc);
    }

    glm_quat_normalize_to(acc, out_q);
}

void Track_get_value_vec3(Track *t, float time, vec3 out) {
    switch (t->kind) {
        case TrackKind::TRACK_VEC3_STATIC: {
            SplineStaticVec3 *st = (SplineStaticVec3*)t->impl;
            glm_vec3_copy(st->item, out);
        } break;

        case TrackKind::TRACK_VEC3_SPLINE_DYNAMIC: {
            const SplineDynamicVec3 *s = (const SplineDynamicVec3*)t->impl;

            out[0] =  0.0f;
            out[1] =  0.0f;
            out[2] =  0.0f;

            int knotSpan = -1;
            const int cSize = (int)(s->num_items + 1u);

            if (s->x_dynamic) {
                knotSpan = FindKnotSpan((int)s->degree, time, cSize, s->knots);
                out[0] = GetSinglePoint_f32(knotSpan, (int)s->degree, time, s->knots, s->x);
            }
            if (s->y_dynamic) {
                if (knotSpan < 0) knotSpan = FindKnotSpan((int)s->degree, time, cSize, s->knots);
                out[1] = GetSinglePoint_f32(knotSpan, (int)s->degree, time, s->knots, s->y);
            }
            if (s->z_dynamic) {
                if (knotSpan < 0) knotSpan = FindKnotSpan((int)s->degree, time, cSize, s->knots);
                out[2] = GetSinglePoint_f32(knotSpan, (int)s->degree, time, s->knots, s->z);
            }
        } break;

        default:
            glm_vec3_zero(out);
            break;
    }
}

void Track_get_value_quat(Track *t, float time, versor out) {
    switch (t->kind) {
        case TrackKind::TRACK_QUAT_STATIC: {
            SplineStaticQuat *st = (SplineStaticQuat*)t->impl;
            glm_quat_copy(st->item, out);
        } break;

        case TrackKind::TRACK_QUAT_SPLINE_DYNAMIC: {
            const SplineDynamicQuat *s = (const SplineDynamicQuat*)t->impl;
            const int cSize = (int)(s->num_items + 1u);
            const int knotSpan = FindKnotSpan((int)s->degree, time, cSize, s->knots);
            GetSinglePoint_quat(knotSpan, (int)s->degree, time, s->knots, s->q, out);
        } break;

        default:
            glm_quat_identity(out);
            break;
    }
}
