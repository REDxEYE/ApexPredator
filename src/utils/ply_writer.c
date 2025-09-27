// Created by RED on 23.09.2025.

#include "utils/ply_writer.h"

#include <string.h>
#include <errno.h>

static BufferError write_ply_header(Buffer *b, uint32_t vertex_count, uint32_t face_count, int have_n, int have_uv,
                                    int have_c, int have_a) {
    String h = {0};
    String_init(&h, 0);
    String_append_cstr(&h, "ply\n");
    String_append_cstr(&h, "format binary_little_endian 1.0\n");
    String_append_format(&h, "element vertex %u\n", vertex_count);
    String_append_cstr(&h, "property float x\n");
    String_append_cstr(&h, "property float y\n");
    String_append_cstr(&h, "property float z\n");
    if (have_n) {
        String_append_cstr(&h, "property float nx\n");
        String_append_cstr(&h, "property float ny\n");
        String_append_cstr(&h, "property float nz\n");
    }
    if (have_uv) {
        String_append_cstr(&h, "property float s\n");
        String_append_cstr(&h, "property float t\n");
    }
    if (have_c) {
        String_append_cstr(&h, "property uchar red\n");
        String_append_cstr(&h, "property uchar green\n");
        String_append_cstr(&h, "property uchar blue\n");
        if (have_a) String_append_cstr(&h, "property uchar alpha\n");
    }
    String_append_format(&h, "element face %u\n", face_count);
    String_append_cstr(&h, "property list uchar int vertex_indices\n");
    String_append_cstr(&h, "end_header\n");
    if (IS_FAILED(b->write(b, String_data(&h), h.size, NULL))) { return BUFFER_FAILED; }
    String_free(&h);
    return BUFFER_SUCCESS;
}

BufferError ply_write_tri_mesh_binary_buf(
    Buffer *buffer,
    const float *positions,
    const float *normals,
    const float *uvs,
    const uint8_t *colors,
    uint32_t color_stride,
    uint32_t vertex_count,
    const uint32 *indices,
    uint32_t index_count
) {
    if (!buffer || !positions || !indices || vertex_count == 0 || index_count == 0 || (index_count % 3) != 0) {
        errno = EINVAL;
        return -1;
    }
    if (colors && !(color_stride == 3 || color_stride == 4)) {
        errno = EINVAL;
        return -1;
    }

    int have_n = normals != NULL;
    int have_uv = uvs != NULL;
    int have_c = colors != NULL;
    int have_a = have_c && (color_stride == 4);
    uint32_t face_count = index_count / 3;

    if (write_ply_header(buffer, vertex_count, face_count, have_n, have_uv, have_c, have_a) != 0) return -1;

    for (uint32_t i = 0; i < vertex_count; ++i) {
        if (buffer->write_float(buffer, positions[3 * i + 0]) ||
            buffer->write_float(buffer, positions[3 * i + 1]) ||
            buffer->write_float(buffer, positions[3 * i + 2]))
            return -1;

        if (have_n) {
            if (buffer->write_float(buffer, normals[3 * i + 0]) ||
                buffer->write_float(buffer, normals[3 * i + 1]) ||
                buffer->write_float(buffer, normals[3 * i + 2]))
                return -1;
        }

        if (have_uv) {
            if (buffer->write_float(buffer, uvs[2 * i + 0]) ||
                buffer->write_float(buffer, uvs[2 * i + 1]))
                return -1;
        }

        if (have_c) {
            if (buffer->write_uint8(buffer, colors[color_stride * i + 0]) ||
                buffer->write_uint8(buffer, colors[color_stride * i + 1]) ||
                buffer->write_uint8(buffer, colors[color_stride * i + 2]))
                return -1;
            if (have_a) {
                if (buffer->write_uint8(buffer, colors[color_stride * i + 3])) return -1;
            }
        }
    }


    for (uint32_t f = 0; f < face_count; ++f) {
        if (buffer->write_uint8(buffer, 3)) return -1;
        if (buffer->write_uint32(buffer, (int32_t) indices[3 * f + 0]) ||
            buffer->write_uint32(buffer, (int32_t) indices[3 * f + 1]) ||
            buffer->write_uint32(buffer, (int32_t) indices[3 * f + 2]))
            return -1;
    }

    return 0;
}
