// Created by RED on 14.01.2026.

#include "platform/texture_ops.h"

#include <assert.h>

#include "platform/logger.h"
#include "utils/memory_profiling.h"

Texture* multiply_4c_by_1c(const Texture* a, const Texture* b) {
    // Fast path
    if (a->width == b->width && a->height == b->height) {
        Texture *result = Texture_new();
        Texture_init(result, a->width, a->height, a->depth, a->bpc, 4);
        const uint32 pixel_count = a->width * a->height;
        const uint32 a_pixel_size = a->channel_count * a->bpc;
        const uint32 b_pixel_size = b->channel_count * b->bpc;
        const uint32 result_pixel_size = result->channel_count * result->bpc;
        result->data = mp_malloc(pixel_count * result_pixel_size);
        result->data_size = pixel_count * result_pixel_size;

        for (uint32 i = 0; i < pixel_count; ++i) {
            for (uint32 c = 0; c < 4; ++c) {
                if (a->bpc == 1) {
                    const uint8 a_value = a->data[i * a_pixel_size + c];
                    const uint8 b_value = b->data[i * b_pixel_size + 0];
                    const uint8 res_value = (uint8) ((uint16) a_value * (uint16) b_value / 255);
                    result->data[i * result_pixel_size + c] = res_value;
                } else if (a->bpc == 2) {
                    const uint16 a_value = ((uint16 *) a->data)[i * a->channel_count + c];
                    const uint16 b_value = ((uint16 *) b->data)[i * b->channel_count + 0];
                    const uint16 res_value = (uint16) ((uint32) a_value * (uint32) b_value / 65535);
                    ((uint16 *) result->data)[i * result->channel_count + c] = res_value;
                } else {
                    assert(false && "Unsupported bpc in multiply_4c_by_1c");
                }
            }
        }
        return result;
    }
    // Slow path for textures where resolution differs by factor of 2, aspect ratio must match
    float aspect_a = (float) a->width / (float) a->height;
    float aspect_b = (float) b->width / (float) b->height;
    if (aspect_a == aspect_b) {
        const Texture *hires = a->width > b->width ? a : b;
        const Texture *lowres = a->width > b->width ? b : a;
        const uint32 scale_factor = hires->width / lowres->width;
        if (hires->width == lowres->width * scale_factor &&
            hires->height == lowres->height * scale_factor) {
            Texture *result = Texture_new();
            Texture_init(result, hires->width, hires->height, hires->depth, hires->bpc, 4);
            const uint32 pixel_count = hires->width * hires->height;
            const uint32 hires_pixel_size = hires->channel_count * hires->bpc;
            const uint32 lowres_pixel_size = lowres->channel_count * lowres->bpc;
            const uint32 result_pixel_size = result->channel_count * result->bpc;
            result->data = mp_malloc(pixel_count * result_pixel_size);
            result->data_size = pixel_count * result_pixel_size;

            for (uint32 y = 0; y < hires->height; ++y) {
                for (uint32 x = 0; x < hires->width; ++x) {
                    const uint32 hires_index = y * hires->width + x;
                    const uint32 lowres_x = x / scale_factor;
                    const uint32 lowres_y = y / scale_factor;
                    const uint32 lowres_index = lowres_y * lowres->width + lowres_x;

                    for (uint32 c = 0; c < 4; ++c) {
                        if (hires->bpc == 1) {
                            const uint8 a_value = hires->data[hires_index * hires_pixel_size + c];
                            const uint8 b_value = lowres->data[lowres_index * lowres_pixel_size + 0];
                            const uint8 res_value = (uint8) ((uint16) a_value * (uint16) b_value / 255);
                            result->data[hires_index * result_pixel_size + c] = res_value;
                        } else if (hires->bpc == 2) {
                            const uint16 a_value = ((uint16 *) hires->data)[hires_index * hires->channel_count + c];
                            const uint16 b_value = ((uint16 *) lowres->data)[lowres_index * lowres->channel_count + 0];
                            const uint16 res_value = (uint16) ((uint32) a_value * (uint32) b_value / 65535);
                            ((uint16 *) result->data)[hires_index * result->channel_count + c] = res_value;
                        } else {
                            assert(false && "Unsupported bpc in multiply_4c_by_1c");
                        }
                    }
                }
            }
            return result;
        }
    }
    GLog_Error("Unsupported texture multiplication due to mismatched resolutions and aspect ratio: %dx%d x %dx%d",
        a->width, a->height, b->width, b->height);
    return NULL;
}

Texture *multiply_3c_by_1c(const Texture *a, const Texture *b) {
    // Fast path
    if (a->width == b->width && a->height == b->height) {
        Texture *result = Texture_new();
        Texture_init(result, a->width, a->height, a->depth, a->bpc, 3);
        const uint32 pixel_count = a->width * a->height;
        const uint32 a_pixel_size = a->channel_count * a->bpc;
        const uint32 b_pixel_size = b->channel_count * b->bpc;
        const uint32 result_pixel_size = result->channel_count * result->bpc;
        result->data = mp_malloc(pixel_count * result_pixel_size);
        result->data_size = pixel_count * result_pixel_size;

        for (uint32 i = 0; i < pixel_count; ++i) {
            for (uint32 c = 0; c < 3; ++c) {
                if (a->bpc == 1) {
                    const uint8 a_value = a->data[i * a_pixel_size + c];
                    const uint8 b_value = b->data[i * b_pixel_size + 0];
                    const uint8 res_value = (uint8) ((uint16) a_value * (uint16) b_value / 255);
                    result->data[i * result_pixel_size + c] = res_value;
                } else if (a->bpc == 2) {
                    const uint16 a_value = ((uint16 *) a->data)[i * a->channel_count + c];
                    const uint16 b_value = ((uint16 *) b->data)[i * b->channel_count + 0];
                    const uint16 res_value = (uint16) ((uint32) a_value * (uint32) b_value / 65535);
                    ((uint16 *) result->data)[i * result->channel_count + c] = res_value;
                } else {
                    assert(false && "Unsupported bpc in multiply_3c_by_1c");
                }
            }
        }
        return result;
    }
    // Slow path for textures where resolution differs by factor of 2, aspect ration must match
    float aspect_a = (float) a->width / (float) a->height;
    float aspect_b = (float) b->width / (float) b->height;
    if (aspect_a == aspect_b) {
        const Texture *hires = a->width > b->width ? a : b;
        const Texture *lowres = a->width > b->width ? b : a;
        const uint32 scale_factor = hires->width / lowres->width;

        if (hires->width == lowres->width * scale_factor &&
            hires->height == lowres->height * scale_factor) {
            Texture *result = Texture_new();
            Texture_init(result, hires->width, hires->height, hires->depth, hires->bpc, 3);
            const uint32 pixel_count = hires->width * hires->height;
            const uint32 hires_pixel_size = hires->channel_count * hires->bpc;
            const uint32 lowres_pixel_size = lowres->channel_count * lowres->bpc;
            const uint32 result_pixel_size = result->channel_count * result->bpc;
            result->data = mp_malloc(pixel_count * result_pixel_size);
            result->data_size = pixel_count * result_pixel_size;

            for (uint32 y = 0; y < hires->height; ++y) {
                for (uint32 x = 0; x < hires->width; ++x) {
                    const uint32 hires_index = y * hires->width + x;
                    const uint32 lowres_x = x / scale_factor;
                    const uint32 lowres_y = y / scale_factor;
                    const uint32 lowres_index = lowres_y * lowres->width + lowres_x;

                    for (uint32 c = 0; c < 3; ++c) {
                        if (hires->bpc == 1) {
                            const uint8 a_value = hires->data[hires_index * hires_pixel_size + c];
                            const uint8 b_value = lowres->data[lowres_index * lowres_pixel_size + 0];
                            const uint8 res_value = (uint8) ((uint16) a_value * (uint16) b_value / 255);
                            result->data[hires_index * result_pixel_size + c] = res_value;
                        } else if (hires->bpc == 2) {
                            const uint16 a_value = ((uint16 *) hires->data)[hires_index * hires->channel_count + c];
                            const uint16 b_value = ((uint16 *) lowres->data)[lowres_index * lowres->channel_count + 0];
                            const uint16 res_value = (uint16) ((uint32) a_value * (uint32) b_value / 65535);
                            ((uint16 *) result->data)[hires_index * result->channel_count + c] = res_value;
                        } else {
                            assert(false && "Unsupported bpc in multiply_3c_by_1c");
                        }
                    }
                }
            }
            return result;
        }
    }
    GLog_Error("Unsupported texture multiplication due to mismatched resolutions and aspect ratio: %dx%d x %dx%d",
        a->width, a->height, b->width, b->height);
    return NULL;
}

Texture *TextureOps_multiply(const Texture *texture_a, const Texture *texture_b) {
    if (texture_a->bpc != texture_b->bpc ||
        texture_a->is_float != texture_b->is_float ||
        texture_a->depth != texture_b->depth) {
        GLog_Error("Unsupported texture multiplication due to mismatched properties");
        return NULL;
    }
    const Texture *tex_a;
    const Texture *tex_b;
    if (texture_a->channel_count > texture_b->channel_count) {
        tex_a = texture_a;
        tex_b = texture_b;
    } else {
        tex_a = texture_b;
        tex_b = texture_a;
    }

    if (tex_a->channel_count == 4 && tex_b->channel_count == 1) {
        return multiply_4c_by_1c(tex_a, tex_b);
    }else if (tex_a->channel_count == 3 && tex_b->channel_count == 1) {
        return multiply_3c_by_1c(tex_a, tex_b);
    }

    GLog_Error("Unsupported texture multiplication channel counts: %d x %d",
           texture_a->channel_count, texture_b->channel_count);
    return NULL;
}
