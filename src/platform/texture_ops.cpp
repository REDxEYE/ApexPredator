// Created by RED on 14.01.2026.

#include "platform/texture_ops.h"

#include <cassert>
#include <memory>

#include "platform/logger.h"

std::unique_ptr<Texture> multiply_4c_by_1c(const Texture* a, const Texture* b) {
    // Fast path
    if (a->width() == b->width() && a->height() == b->height()) {
        const uint32 pixel_count = a->width() * a->height();
        const uint32 a_pixel_size = a->channel_count() * a->bpc();
        const uint32 b_pixel_size = b->channel_count() * b->bpc();
        const uint32 result_pixel_size = 4 * a->bpc();

        std::vector<uint8> result_data(pixel_count * result_pixel_size);

        for (uint32 i = 0; i < pixel_count; ++i) {
            for (uint32 c = 0; c < 4; ++c) {
                if (a->bpc() == 1) {
                    const uint8 a_value = a->data()[i * a_pixel_size + c];
                    const uint8 b_value = b->data()[i * b_pixel_size + 0];
                    const uint8 res_value = (uint8) ((uint16) a_value * (uint16) b_value / 255);
                    result_data[i * result_pixel_size + c] = res_value;
                } else if (a->bpc() == 2) {
                    const uint16 a_value = reinterpret_cast<const uint16*>(a->data().data())[i * a->channel_count() + c];
                    const uint16 b_value = reinterpret_cast<const uint16*>(b->data().data())[i * b->channel_count() + 0];
                    const uint16 res_value = (uint16) ((uint32) a_value * (uint32) b_value / 65535);
                    reinterpret_cast<uint16*>(result_data.data())[i * 4 + c] = res_value;
                } else {
                    assert(false && "Unsupported bpc in multiply_4c_by_1c");
                }
            }
        }
        return std::make_unique<Texture>(a->width(), a->height(), a->depth(), a->bpc(), 4, a->is_float(), std::move(result_data));
    }
    // Slow path for textures where resolution differs by factor of 2, aspect ratio must match
    float aspect_a = (float) a->width() / (float) a->height();
    float aspect_b = (float) b->width() / (float) b->height();
    if (aspect_a == aspect_b) {
        const Texture* hires = a->width() > b->width() ? a : b;
        const Texture* lowres = a->width() > b->width() ? b : a;
        const uint32 scale_factor = hires->width() / lowres->width();
        if (hires->width() == lowres->width() * scale_factor &&
            hires->height() == lowres->height() * scale_factor) {
            const uint32 pixel_count = hires->width() * hires->height();
            const uint32 hires_pixel_size = hires->channel_count() * hires->bpc();
            const uint32 lowres_pixel_size = lowres->channel_count() * lowres->bpc();
            const uint32 result_pixel_size = 4 * hires->bpc();

            std::vector<uint8> result_data(pixel_count * result_pixel_size);

            for (uint32 y = 0; y < hires->height(); ++y) {
                for (uint32 x = 0; x < hires->width(); ++x) {
                    const uint32 hires_index = y * hires->width() + x;
                    const uint32 lowres_x = x / scale_factor;
                    const uint32 lowres_y = y / scale_factor;
                    const uint32 lowres_index = lowres_y * lowres->width() + lowres_x;

                    for (uint32 c = 0; c < 4; ++c) {
                        if (hires->bpc() == 1) {
                            const uint8 a_value = hires->data()[hires_index * hires_pixel_size + c];
                            const uint8 b_value = lowres->data()[lowres_index * lowres_pixel_size + 0];
                            const uint8 res_value = (uint8) ((uint16) a_value * (uint16) b_value / 255);
                            result_data[hires_index * result_pixel_size + c] = res_value;
                        } else if (hires->bpc() == 2) {
                            const uint16 a_value = reinterpret_cast<const uint16*>(hires->data().data())[hires_index * hires->channel_count() + c];
                            const uint16 b_value = reinterpret_cast<const uint16*>(lowres->data().data())[lowres_index * lowres->channel_count() + 0];
                            const uint16 res_value = (uint16) ((uint32) a_value * (uint32) b_value / 65535);
                            reinterpret_cast<uint16*>(result_data.data())[hires_index * 4 + c] = res_value;
                        } else {
                            assert(false && "Unsupported bpc in multiply_4c_by_1c");
                        }
                    }
                }
            }
            return std::make_unique<Texture>(hires->width(), hires->height(), hires->depth(), hires->bpc(), 4, hires->is_float(), std::move(result_data));
        }
    }
    GLog_Error("Unsupported texture multiplication due to mismatched resolutions and aspect ratio: %dx%d x %dx%d",
        a->width(), a->height(), b->width(), b->height());
    return nullptr;
}

std::unique_ptr<Texture> multiply_3c_by_1c(const Texture* a, const Texture* b) {
    // Fast path
    if (a->width() == b->width() && a->height() == b->height()) {
        const uint32 pixel_count = a->width() * a->height();
        const uint32 a_pixel_size = a->channel_count() * a->bpc();
        const uint32 b_pixel_size = b->channel_count() * b->bpc();
        const uint32 result_pixel_size = 3 * a->bpc();

        std::vector<uint8> result_data(pixel_count * result_pixel_size);

        for (uint32 i = 0; i < pixel_count; ++i) {
            for (uint32 c = 0; c < 3; ++c) {
                if (a->bpc() == 1) {
                    const uint8 a_value = a->data()[i * a_pixel_size + c];
                    const uint8 b_value = b->data()[i * b_pixel_size + 0];
                    const uint8 res_value = (uint8) ((uint16) a_value * (uint16) b_value / 255);
                    result_data[i * result_pixel_size + c] = res_value;
                } else if (a->bpc() == 2) {
                    const uint16 a_value = reinterpret_cast<const uint16*>(a->data().data())[i * a->channel_count() + c];
                    const uint16 b_value = reinterpret_cast<const uint16*>(b->data().data())[i * b->channel_count() + 0];
                    const uint16 res_value = (uint16) ((uint32) a_value * (uint32) b_value / 65535);
                    reinterpret_cast<uint16*>(result_data.data())[i * 3 + c] = res_value;
                } else {
                    assert(false && "Unsupported bpc in multiply_3c_by_1c");
                }
            }
        }
        return std::make_unique<Texture>(a->width(), a->height(), a->depth(), a->bpc(), 3, a->is_float(), std::move(result_data));
    }
    // Slow path for textures where resolution differs by factor of 2, aspect ratio must match
    float aspect_a = (float) a->width() / (float) a->height();
    float aspect_b = (float) b->width() / (float) b->height();
    if (aspect_a == aspect_b) {
        const Texture* hires = a->width() > b->width() ? a : b;
        const Texture* lowres = a->width() > b->width() ? b : a;
        const uint32 scale_factor = hires->width() / lowres->width();

        if (hires->width() == lowres->width() * scale_factor &&
            hires->height() == lowres->height() * scale_factor) {
            const uint32 pixel_count = hires->width() * hires->height();
            const uint32 hires_pixel_size = hires->channel_count() * hires->bpc();
            const uint32 lowres_pixel_size = lowres->channel_count() * lowres->bpc();
            const uint32 result_pixel_size = 3 * hires->bpc();

            std::vector<uint8> result_data(pixel_count * result_pixel_size);

            for (uint32 y = 0; y < hires->height(); ++y) {
                for (uint32 x = 0; x < hires->width(); ++x) {
                    const uint32 hires_index = y * hires->width() + x;
                    const uint32 lowres_x = x / scale_factor;
                    const uint32 lowres_y = y / scale_factor;
                    const uint32 lowres_index = lowres_y * lowres->width() + lowres_x;

                    for (uint32 c = 0; c < 3; ++c) {
                        if (hires->bpc() == 1) {
                            const uint8 a_value = hires->data()[hires_index * hires_pixel_size + c];
                            const uint8 b_value = lowres->data()[lowres_index * lowres_pixel_size + 0];
                            const uint8 res_value = (uint8) ((uint16) a_value * (uint16) b_value / 255);
                            result_data[hires_index * result_pixel_size + c] = res_value;
                        } else if (hires->bpc() == 2) {
                            const uint16 a_value = reinterpret_cast<const uint16*>(hires->data().data())[hires_index * hires->channel_count() + c];
                            const uint16 b_value = reinterpret_cast<const uint16*>(lowres->data().data())[lowres_index * lowres->channel_count() + 0];
                            const uint16 res_value = (uint16) ((uint32) a_value * (uint32) b_value / 65535);
                            reinterpret_cast<uint16*>(result_data.data())[hires_index * 3 + c] = res_value;
                        } else {
                            assert(false && "Unsupported bpc in multiply_3c_by_1c");
                        }
                    }
                }
            }
            return std::make_unique<Texture>(hires->width(), hires->height(), hires->depth(), hires->bpc(), 3, hires->is_float(), std::move(result_data));
        }
    }
    GLog_Error("Unsupported texture multiplication due to mismatched resolutions and aspect ratio: %dx%d x %dx%d",
        a->width(), a->height(), b->width(), b->height());
    return nullptr;
}

std::unique_ptr<Texture> TextureOps::multiply(const Texture* texture_a, const Texture* texture_b) {
    if (texture_a->bpc() != texture_b->bpc() ||
        texture_a->is_float() != texture_b->is_float() ||
        texture_a->depth() != texture_b->depth()) {
        GLog_Error("Unsupported texture multiplication due to mismatched properties");
        return nullptr;
    }
    const Texture* tex_a = texture_a->channel_count() > texture_b->channel_count() ? texture_a : texture_b;
    const Texture* tex_b = texture_a->channel_count() > texture_b->channel_count() ? texture_b : texture_a;

    if (tex_a->channel_count() == 4 && tex_b->channel_count() == 1) {
        return multiply_4c_by_1c(tex_a, tex_b);
    }
    if (tex_a->channel_count() == 3 && tex_b->channel_count() == 1) {
        return multiply_3c_by_1c(tex_a, tex_b);
    }

    GLog_Error("Unsupported texture multiplication channel counts: %d x %d",
           texture_a->channel_count(), texture_b->channel_count());
    return nullptr;
}
