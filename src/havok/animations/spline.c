// Created by RED on 30.01.2026.

#include "havok/animations/spline.h"


u8 rd_u8(const u8 **p) {
    const u8 v = **p;
    *p += 1;
    return v;
}

u16 rd_u16_le(const u8 **p) {
    u16 v;
    memcpy(&v, *p, 2);
    *p += 2;
    return v;
}

float rd_f32_le(const u8 **p) {
    float v;
    memcpy(&v, *p, 4);
    *p += 4;
    return v;
}

size_t compute_padding_ptr(const void *ptr, const int alignment) {
    const uintptr_t cur = (uintptr_t) ptr;
    const size_t r = (size_t) (cur & (uintptr_t) (alignment - 1));
    return r ? (size_t) alignment - r : 0;
}


void apply_padding(const u8 **p, int alignment) {
    *p += compute_padding_ptr(*p, alignment);
}

void apply_padding4(const u8 **p) { apply_padding(p, 4); }
void apply_padding2(const u8 **p) { apply_padding(p, 2); }

static u32 rd_u32_le(const u8 **p) { u32 v; memcpy(&v, *p, 4); *p += 4; return v; }

static u64 rd_u40_le(const u8 **p) {
    const u8 *b = *p;
    u64 v = (u64)b[0] | ((u64)b[1] << 8) | ((u64)b[2] << 16) | ((u64)b[3] << 24) | ((u64)b[4] << 32);
    *p += 5;
    return v;
}
static int16_t rd_i16_le_ptr(const u8 *p) { int16_t v; memcpy(&v, p, 2); return v; }

static void Read32Quat_C(const u8 **p, float out[4]) {
    const u32 cVal = rd_u32_le(p);

    const u32 rMask = (1u << 10) - 1u;
    const float rFrac = 1.0f / 1023.0f;

    const float fPI  = 3.14159265f;
    const float fPI2 = 0.5f * fPI;
    const float fPI4 = 0.5f * fPI2;        /* pi/4 */
    const float phiFrac = fPI2 / 511.0f;   /* (pi/2)/511 */

    float R = (float)((cVal >> 18) & rMask) * rFrac;
    R = 1.0f - (R * R);

    const float phiTheta = (float)(cVal & 0x3FFFFu);

    float phi = floorf(sqrtf(phiTheta));
    float theta = 0.0f;

    if (phi > 0.0f) {
        theta = fPI4 * (phiTheta - (phi * phi)) / phi;
        phi = phiFrac * phi;
    }

    const float magnitude = sqrtf(fmaxf(0.0f, 1.0f - R * R));
    const float sPhi = sinf(phi);
    const float cPhi = cosf(phi);
    const float sTheta = sinf(theta);
    const float cTheta = cosf(theta);

    float x = sPhi * cTheta * magnitude;
    float y = sPhi * sTheta * magnitude;
    float z = cPhi * magnitude;
    float w = R;

    /* sign bits 28..31 */
    if (cVal & 0x10000000u) x = -x;
    if (cVal & 0x20000000u) y = -y;
    if (cVal & 0x40000000u) z = -z;
    if (cVal & 0x80000000u) w = -w;

    out[0] = x; out[1] = y; out[2] = z; out[3] = w;
}

static void Read40Quat_C(const u8 **p, float out[4]) {
    const u64 cVal = rd_u40_le(p);

    const float fractal = 0.000345436f; /* matches your C++ constant */

    const u32 a = (u32)((cVal >> 0)  & 0xFFFu);
    const u32 b = (u32)((cVal >> 12) & 0xFFFu);
    const u32 c = (u32)((cVal >> 24) & 0xFFFu);

    /* Center to roughly [-2048, 2047] (your SIMD did "- (1<<11) - 1"; close enough).
       If you want bit-exact behavior, change bias to 2049. */
    const int ia = (int)a - 2048;
    const int ib = (int)b - 2048;
    const int ic = (int)c - 2048;

    float v0 = (float)ia * fractal;
    float v1 = (float)ib * fractal;
    float v2 = (float)ic * fractal;

    const u32 resultShift = (u32)((cVal >> 36) & 3u);
    const float wsign = ((cVal >> 38) & 1u) ? -1.0f : 1.0f;

    const float sumsq = v0*v0 + v1*v1 + v2*v2;
    const float missing = sqrtf(fmaxf(0.0f, 1.0f - sumsq)) * wsign;

    /* Build like your SSE: tmp is [v0,v1,v2,missing], then shuffle based on resultShift */
    switch (resultShift) {
        case 0: out[0] = missing; out[1] = v0;      out[2] = v1;      out[3] = v2;      break; /* [w,x,y,z] */
        case 1: out[0] = v0;      out[1] = missing; out[2] = v1;      out[3] = v2;      break; /* [x,w,y,z] */
        case 2: out[0] = v0;      out[1] = v1;      out[2] = missing; out[3] = v2;      break; /* [x,y,w,z] */
        default:out[0] = v0;      out[1] = v1;      out[2] = v2;      out[3] = missing; break; /* [x,y,z,w] */
    }
}

static void Read48Quat_C(const u8 **p, float out[4]) {
    const u8 *b = *p;

    /* 3x little-endian int16 */
    const int16_t X = rd_i16_le_ptr(b + 0);
    const int16_t Y = rd_i16_le_ptr(b + 2);
    const int16_t Z = rd_i16_le_ptr(b + 4);
    *p += 6;

    const float fractal = 0.000043161f; /* matches your C++ constant */
    const u32 mask = (1u << 15) - 1u;

    const u32 ux = (u16)X;
    const u32 uy = (u16)Y;
    const u32 uz = (u16)Z;

    const u32 resultShift = ((uy >> 14) & 2u) | ((ux >> 15) & 1u);
    const float wsign = ((uz >> 15) != 0) ? -1.0f : 1.0f;

    const int ix = (int)(ux & mask) - (int)(mask >> 1) - 1; /* center ~[-16384,16383] */
    const int iy = (int)(uy & mask) - (int)(mask >> 1) - 1;
    const int iz = (int)(uz & mask) - (int)(mask >> 1) - 1;

    float v0 = (float)ix * fractal;
    float v1 = (float)iy * fractal;
    float v2 = (float)iz * fractal;

    const float sumsq = v0*v0 + v1*v1 + v2*v2;
    const float missing = sqrtf(fmaxf(0.0f, 1.0f - sumsq)) * wsign;

    switch (resultShift) {
        case 0: out[0] = missing; out[1] = v0;      out[2] = v1;      out[3] = v2;      break;
        case 1: out[0] = v0;      out[1] = missing; out[2] = v1;      out[3] = v2;      break;
        case 2: out[0] = v0;      out[1] = v1;      out[2] = missing; out[3] = v2;      break;
        default:out[0] = v0;      out[1] = v1;      out[2] = v2;      out[3] = missing; break;
    }
}

void ReadQuat_C(const QuantizationType qt, const u8 **p, float out[4]) {
    switch (qt) {
        case QT_32bit:
            Read32Quat_C(p, out);
            break;
        case QT_40bit:
            Read40Quat_C(p, out);
            break;
        case QT_48bit:
            Read48Quat_C(p, out);
            break;
        case QT_Uncompressed:
            out[0] = rd_f32_le(p);
            out[1] = rd_f32_le(p);
            out[2] = rd_f32_le(p);
            out[3] = rd_f32_le(p);
            break;
        default:
            out[0] = 0.0f;
            out[1] = 0.0f;
            out[2] = 0.0f;
            out[3] = 1.0f;
            break;
    }
}


static Track make_vec3_track(
    const TransformMask *m,
    QuantizationType qtype,
    float defVal,
    int is_scale, /* 0=pos, 1=scale */
    const u8 **p
) {
    const SplineTrackType tx = is_scale ? TransformMask_GetSubTrackType_Scale(m, 0) : TransformMask_GetSubTrackType_Pos(m, 0);
    const SplineTrackType ty = is_scale ? TransformMask_GetSubTrackType_Scale(m, 1) : TransformMask_GetSubTrackType_Pos(m, 1);
    const SplineTrackType tz = is_scale ? TransformMask_GetSubTrackType_Scale(m, 2) : TransformMask_GetSubTrackType_Pos(m, 2);

    const int useSpline = (tx == STT_DYNAMIC) || (ty == STT_DYNAMIC) || (tz == STT_DYNAMIC);

    if (useSpline) {
        SplineDynamicVec3 *s = (SplineDynamicVec3 *) mp_calloc(1, sizeof(*s));
        const u16 numItems = rd_u16_le(p);
        s->num_items = (u32) numItems;

        s->degree = rd_u8(p);

        const u32 knots_len = (u32) numItems + (u32) s->degree + 2u;
        s->knots = *p;
        s->knots_len = knots_len;
        *p += knots_len;
        apply_padding4(p);

        TrackBBOX extremes[3] = {0};

        s->x_dynamic = (tx == STT_DYNAMIC);
        s->y_dynamic = (ty == STT_DYNAMIC);
        s->z_dynamic = (tz == STT_DYNAMIC);

        if (tx == STT_DYNAMIC) {
            extremes[0].min = rd_f32_le(p);
            extremes[0].max = rd_f32_le(p);
            s->x = (float *) mp_malloc(sizeof(float) * (s->num_items + 1u));
        }
        else {
            s->x = NULL;
        }

        if (ty == STT_DYNAMIC) {
            extremes[1].min = rd_f32_le(p);
            extremes[1].max = rd_f32_le(p);
            s->y = (float *) mp_malloc(sizeof(float) * (s->num_items + 1u));
        }
        else {
            s->y = NULL;
        }

        if (tz == STT_DYNAMIC) {
            extremes[2].min = rd_f32_le(p);
            extremes[2].max = rd_f32_le(p);
            s->z = (float *) mp_malloc(sizeof(float) * (s->num_items + 1u));
        }
        else {
            s->z = NULL;
        }

        float x_static = defVal, y_static = defVal, z_static = defVal;
        if (tx == STT_STATIC) x_static = rd_f32_le(p);
        if (ty == STT_STATIC) y_static = rd_f32_le(p);
        if (tz == STT_STATIC) z_static = rd_f32_le(p);

        if (tx == STT_DYNAMIC) {
            for (u32 i = 0; i <= s->num_items; i++) s->x[i] = x_static;
        }
        if (ty == STT_DYNAMIC) {
            for (u32 i = 0; i <= s->num_items; i++) s->y[i] = y_static;
        }
        if (tz == STT_DYNAMIC) {
            for (u32 i = 0; i <= s->num_items; i++) s->z[i] = z_static;
        }

        if (qtype == QT_8bit) {
            const float fract = 1.0f / 255.0f;
            for (u32 t = 0; t <= s->num_items; t++) {
                if (tx == STT_DYNAMIC) {
                    const float d = (float) rd_u8(p) * fract;
                    s->x[t] = extremes[0].min + (extremes[0].max - extremes[0].min) * d;
                }
                if (ty == STT_DYNAMIC) {
                    const float d = (float) rd_u8(p) * fract;
                    s->y[t] = extremes[1].min + (extremes[1].max - extremes[1].min) * d;
                }
                if (tz == STT_DYNAMIC) {
                    const float d = (float) rd_u8(p) * fract;
                    s->z[t] = extremes[2].min + (extremes[2].max - extremes[2].min) * d;
                }
            }
        }
        else {
            const float fract = 1.0f / 65535.0f;
            for (u32 t = 0; t <= s->num_items; t++) {
                if (tx == STT_DYNAMIC) {
                    const float d = (float) rd_u16_le(p) * fract;
                    s->x[t] = extremes[0].min + (extremes[0].max - extremes[0].min) * d;
                }
                if (ty == STT_DYNAMIC) {
                    const float d = (float) rd_u16_le(p) * fract;
                    s->y[t] = extremes[1].min + (extremes[1].max - extremes[1].min) * d;
                }
                if (tz == STT_DYNAMIC) {
                    const float d = (float) rd_u16_le(p) * fract;
                    s->z[t] = extremes[2].min + (extremes[2].max - extremes[2].min) * d;
                }
            }
        }

        apply_padding4(p);

        Track out = {0};
        out.kind = TRACK_VEC3_SPLINE_DYNAMIC;
        out.impl = s;
        return out;
    }

    SplineStaticVec3 *st = (SplineStaticVec3 *) mp_calloc(1, sizeof(*st));
    st->item[0] = (tx == STT_STATIC) ? rd_f32_le(p) : defVal;
    st->item[1] = (ty == STT_STATIC) ? rd_f32_le(p) : defVal;
    st->item[2] = (tz == STT_STATIC) ? rd_f32_le(p) : defVal;

    Track out = {0};
    out.kind = TRACK_VEC3_STATIC;
    out.impl = st;
    return out;
}

static Track make_quat_track(const TransformMask *m, const u8 **p) {
    const SplineTrackType rt = TransformMask_GetSubTrackType_Rotation(m);

    if (rt == STT_DYNAMIC) {
        SplineDynamicQuat *r = (SplineDynamicQuat *) mp_calloc(1, sizeof(*r));

        const u16 numItems = rd_u16_le(p);
        r->num_items = (u32) numItems;

        r->degree = rd_u8(p);

        const u32 knots_len = (u32) numItems + (u32) r->degree + 2u;
        r->knots = *p;
        r->knots_len = knots_len;
        *p += knots_len;

        const QuantizationType qt = TransformMask_GetRotQuantizationType(m);
        if (qt == QT_48bit || qt == QT_16bitQuat) apply_padding2(p);
        else if (qt == QT_32bit || qt == QT_Uncompressed) apply_padding4(p);

        r->q = (Quat *) mp_malloc(sizeof(Quat) * (r->num_items + 1u));
        for (u32 t = 0; t <= r->num_items; t++) {
            ReadQuat_C(qt, p, r->q[t]);
        }

        Track out = {0};
        out.kind = TRACK_QUAT_SPLINE_DYNAMIC;
        out.impl = r;
        return out;
    }

    SplineStaticQuat *st = (SplineStaticQuat *) mp_calloc(1, sizeof(*st));

    if (rt == STT_STATIC) {
        ReadQuat_C(TransformMask_GetRotQuantizationType(m), p, st->item);
    }
    else {
        st->item[0] = 0;
        st->item[1] = 0;
        st->item[2] = 0;
        st->item[3] = 1;
    }

    Track out = {0};
    out.kind = TRACK_QUAT_STATIC;
    out.impl = st;
    return out;
}

void TransformSplineBlock_free(TransformSplineBlock *self) {
    if (!self) return;
    if (self->tracks) {
        for (u32 i = 0; i < self->track_count; i++) {
            Track_free(&self->tracks[i].position);
            Track_free(&self->tracks[i].rotation);
            Track_free(&self->tracks[i].scale);
        }
        mp_free(self->tracks);
    }
    self->masks = NULL;
    self->mask_count = 0;
    self->tracks = NULL;
    self->track_count = 0;
}

bool TransformSplineBlock_assign(TransformSplineBlock *self, const uint8 *data,
                                 const uint32 track_count,
                                 const uint32 float_track_count) {
    if (!self || !data) return false;

    TransformSplineBlock_free(self);

    const TransformMask *track_start = (const TransformMask *) data;
    const u8 *p = data + sizeof(TransformMask) * (size_t) track_count + (size_t) float_track_count;
    apply_padding4(&p);

    self->masks = track_start;
    self->mask_count = track_count;

    self->tracks = (TransformTrack *) mp_calloc(track_count, sizeof(TransformTrack));
    self->track_count = track_count;

    for (u32 i = 0; i < track_count; i++) {
        const TransformMask *m = &track_start[i];

        self->tracks[i].position = make_vec3_track(
            m,
            TransformMask_GetPosQuantizationType(m),
            0.0f,
            0,
            &p
        );

        self->tracks[i].rotation = make_quat_track(m, &p);

        apply_padding4(&p);

        self->tracks[i].scale = make_vec3_track(
            m,
            TransformMask_GetScaleQuantizationType(m),
            1.0f,
            1,
            &p
        );
    }
    return true;
}

void TransformSplineBlock_get_value(const TransformSplineBlock *self, u32 trackID, float time, QTransform *out) {
    Track_get_value_quat(&self->tracks[trackID].rotation, time, out->rotation);
    Track_get_value_vec3(&self->tracks[trackID].position, time, out->translation);
    Track_get_value_vec3(&self->tracks[trackID].scale,    time, out->scale);
}


void hkaSplineDecompressor_assign(hkaSplineDecompressor *self, const hkaSplineCompressedAnimation *input) {
    DA_init(&self->blocks, TransformSplineBlock, input->numBlocks);
    const uint8 *raw_data = input->data.m_data;
    for (int i = 0; i < input->numBlocks; ++i) {
        const uint32 block_offset = input->blockOffsets.m_data[i];
        TransformSplineBlock *block = DA_append_get(&self->blocks);
        TransformSplineBlock_assign(block, raw_data + block_offset, input->numberOfTransformTracks,
                                    input->numberOfFloatTracks);
    }
}

void hkaSplineDecompressor_free(hkaSplineDecompressor *self) {
    for (uint32 i = 0; i < self->blocks.count; ++i) {
        TransformSplineBlock_free(&self->blocks.items[i]);
    }
    DA_free(&self->blocks);
}
