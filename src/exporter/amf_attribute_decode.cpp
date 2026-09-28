#include "exporter/amf_attribute_decode.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "redscore/platform/logger.h"

namespace AMF {
    namespace {
        float packing_float(const ADFTypes::AmfStreamAttribute &attribute, size_t offset) {
            float value;
            std::memcpy(&value, attribute.PackingData.data() + offset, sizeof(value));
            return value;
        }

        [[noreturn]] void unsupported_attribute(const ADFTypes::AmfStreamAttribute &attribute) {
            std::ostringstream usage_name, format_name;
            usage_name << attribute.Usage;
            format_name << attribute.Format;
            const auto diagnostic = std::format(
                "Unsupported AMF vertex attribute: usage={} format={} stream={} offset={} "
                "(raw usage=0x{:X} format=0x{:X})",
                usage_name.str(), format_name.str(),
                attribute.StreamIndex, attribute.StreamOffset,
                static_cast<uint32_t>(attribute.Usage), static_cast<uint32_t>(attribute.Format));
            GLog_Error("{}", diagnostic);
            throw std::runtime_error(diagnostic);
        }
    }

    std::optional<VM::VertexAttribute> decode_attribute(const AttributeInput &input) {
        const auto &attribute = input.descriptor;
        using ADFTypes::AmfFormat;
        using ADFTypes::AmfUsage;
        VM::VertexAttribute decoded;
        decoded.count = input.vertex_count;
        size_t input_width = 0, output_width = 0;

        switch (attribute.Usage) {
            case AmfUsage::AmfUsage_Position:
                if (attribute.Format != AmfFormat::AmfFormat_R16G16B16_SNORM &&
                    attribute.Format != AmfFormat::AmfFormat_R32G32B32_FLOAT) unsupported_attribute(attribute);
                decoded.usage = VM::ElementUsage::Position;
                decoded.format = VM::ElementFormat::F32;
                decoded.type = VM::ElementType::Vec3;
                input_width = attribute.Format == AmfFormat::AmfFormat_R16G16B16_SNORM ? 6 : 12;
                output_width = 12;
                break;
            case AmfUsage::AmfUsage_TextureCoordinate:
                if (attribute.Format != AmfFormat::AmfFormat_R16G16_SNORM &&
                    attribute.Format != AmfFormat::AmfFormat_R32G32_FLOAT) unsupported_attribute(attribute);
                decoded.usage = VM::ElementUsage::TexCoord;
                decoded.set = input.texcoord_set;
                decoded.format = VM::ElementFormat::F32;
                decoded.type = VM::ElementType::Vec2;
                input_width = attribute.Format == AmfFormat::AmfFormat_R16G16_SNORM ? 4 : 8;
                output_width = 8;
                break;
            case AmfUsage::AmfUsage_Normal:
                if (attribute.Format != AmfFormat::AmfFormat_R32_UNIT_VEC_AS_FLOAT &&
                    attribute.Format != AmfFormat::AmfFormat_R32G32B32_FLOAT) unsupported_attribute(attribute);
                decoded.usage = VM::ElementUsage::Normal;
                decoded.format = VM::ElementFormat::F32;
                decoded.type = VM::ElementType::Vec3;
                input_width = attribute.Format == AmfFormat::AmfFormat_R32_UNIT_VEC_AS_FLOAT ? 4 : 12;
                output_width = 12;
                break;
            case AmfUsage::AmfUsage_Color:
                if (attribute.Format != AmfFormat::AmfFormat_R32_R8G8B8A8_UNORM_AS_FLOAT &&
                    attribute.Format != AmfFormat::AmfFormat_R8G8B8A8_UNORM) unsupported_attribute(attribute);
                decoded.usage = input.color_as_custom ? VM::ElementUsage::Custom : VM::ElementUsage::Color;
                if (input.color_as_custom) decoded.custom_name = "_COLOR_0";
                decoded.format = VM::ElementFormat::U8;
                decoded.type = VM::ElementType::Vec4;
                decoded.normalized = true;
                input_width = output_width = 4;
                break;
            case AmfUsage::AmfUsage_BoneIndex:
                if (attribute.Format != AmfFormat::AmfFormat_R8G8B8A8_UINT) unsupported_attribute(attribute);
                if (input.bone_lookup.empty()) return std::nullopt;
                decoded.usage = VM::ElementUsage::Joints;
                decoded.format = VM::ElementFormat::U16;
                decoded.type = VM::ElementType::Vec4;
                input_width = 4;
                output_width = 8;
                break;
            case AmfUsage::AmfUsage_BoneWeight:
                if (attribute.Format != AmfFormat::AmfFormat_R8G8B8A8_UNORM &&
                    attribute.Format != AmfFormat::AmfFormat_R32G32B32A32_FLOAT) unsupported_attribute(attribute);
                decoded.usage = VM::ElementUsage::Weights;
                decoded.format = VM::ElementFormat::F32;
                decoded.type = VM::ElementType::Vec4;
                input_width = attribute.Format == AmfFormat::AmfFormat_R8G8B8A8_UNORM ? 4 : 16;
                output_width = 16;
                break;
            default:
                return std::nullopt;
        }

        if (!input.vertex_count || input.stride < size_t(attribute.StreamOffset) + input_width ||
            input.offset > input.buffer.size() || input_width > input.buffer.size() - input.offset ||
            (input.vertex_count - 1) > (input.buffer.size() - input.offset - input_width) / input.stride ||
            input.vertex_count > std::numeric_limits<size_t>::max() / output_width)
            return std::nullopt;

        decoded.data.resize(input.vertex_count * output_width);
        const float position_scale = attribute.Format == AmfFormat::AmfFormat_R16G16B16_SNORM
                                         ? packing_float(attribute, 0) : 1.f;
        const float uv_scale_u = attribute.Format == AmfFormat::AmfFormat_R16G16_SNORM
                                     ? packing_float(attribute, 0) : 1.f;
        const float uv_scale_v = attribute.Format == AmfFormat::AmfFormat_R16G16_SNORM
                                     ? packing_float(attribute, 4) : 1.f;
        for (size_t i = 0; i < input.vertex_count; ++i) {
            const uint8_t *src = input.buffer.data() + input.offset + i * input.stride;
            uint8_t *dst = decoded.data.data() + i * output_width;
            float values[4]{};
            switch (attribute.Usage) {
                case AmfUsage::AmfUsage_Position:
                    if (attribute.Format == AmfFormat::AmfFormat_R16G16B16_SNORM) {
                        int16_t packed[3];
                        std::memcpy(packed, src, sizeof(packed));
                        for (size_t j = 0; j < 3; ++j)
                            values[j] = position_scale * std::max(-1.f, packed[j] / 32767.f);
                        std::memcpy(dst, values, 12);
                    } else std::memcpy(dst, src, 12);
                    break;
                case AmfUsage::AmfUsage_TextureCoordinate:
                    if (attribute.Format == AmfFormat::AmfFormat_R16G16_SNORM) {
                        int16_t packed[2];
                        std::memcpy(packed, src, sizeof(packed));
                        values[0] = std::max(-1.f, packed[0] / 32767.f) * uv_scale_u;
                        values[1] = std::max(-1.f, packed[1] / 32767.f) * uv_scale_v;
                        std::memcpy(dst, values, 8);
                    } else std::memcpy(dst, src, 8);
                    break;
                case AmfUsage::AmfUsage_Normal:
                    if (attribute.Format == AmfFormat::AmfFormat_R32_UNIT_VEC_AS_FLOAT) {
                        float packed;
                        std::memcpy(&packed, src, 4);
                        if (!std::isfinite(packed)) return std::nullopt;
                        for (size_t j = 0; j < 3; ++j) {
                            float whole;
                            values[j] = -1.f + 2.f * std::modf(packed, &whole);
                            packed *= 1.f / 256.f;
                        }
                        const float length = std::sqrt(values[0] * values[0] + values[1] * values[1] +
                                                       values[2] * values[2]);
                        if (length > 0.f) for (size_t j = 0; j < 3; ++j) values[j] /= length;
                        std::memcpy(dst, values, 12);
                    } else std::memcpy(dst, src, 12);
                    break;
                case AmfUsage::AmfUsage_Color:
                    if (attribute.Format == AmfFormat::AmfFormat_R8G8B8A8_UNORM) {
                        std::memcpy(dst, src, 4);
                    } else {
                        float packed;
                        std::memcpy(&packed, src, 4);
                        if (!std::isfinite(packed)) return std::nullopt;
                        for (size_t j = 0; j < 4; ++j) {
                            float whole;
                            dst[j] = static_cast<uint8_t>(std::clamp(std::modf(packed, &whole) * 255.f,
                                                                      0.f, 255.f));
                            packed *= 1.f / 256.f;
                        }
                    }
                    break;
                case AmfUsage::AmfUsage_BoneIndex:
                    for (size_t j = 0; j < 4; ++j) {
                        if (src[j] >= input.bone_lookup.size() || input.bone_lookup[src[j]] < 0)
                            return std::nullopt;
                        const uint16_t bone = static_cast<uint16_t>(input.bone_lookup[src[j]]);
                        std::memcpy(dst + j * 2, &bone, 2);
                    }
                    break;
                case AmfUsage::AmfUsage_BoneWeight:
                    if (attribute.Format == AmfFormat::AmfFormat_R8G8B8A8_UNORM) {
                        for (size_t j = 0; j < 4; ++j) values[j] = src[j] / 255.f;
                    } else std::memcpy(values, src, 16);
                    {
                        const float sum = values[0] + values[1] + values[2] + values[3];
                        if (sum > 0.f) for (float &value: values) value /= sum;
                    }
                    std::memcpy(dst, values, 16);
                    break;
                default:
                    break;
            }
        }
        return decoded;
    }
}
