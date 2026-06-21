// Created by RED on 03.10.2025.
#include "apex/rtpc.h"

#include "glm/gtc/type_ptr.hpp"
#include "glm/glm.hpp"
#include "apex/hashes.h"
#include "redscore/platform/logger.h"

using json = nlohmann::json;


#pragma pack(push, 1)
typedef struct {
    char ident[4];
    uint32 version;
} RTPCHeader;

struct RuntimeNodeHeader {
    uint32 name_hash;
    uint32 data_offset;
    uint16 prop_count;
    uint16 child_count;
};

struct RuntimePropHeader {
    uint32 name_hash;

    union {
        uint32 uint_value;
        float32 float_value;
    } data_raw;

    PropType prop_type;
};
#pragma pack(pop)

RuntimeProp::RuntimeProp(IO::File &buffer) {
    const auto header = buffer.read_pod<RuntimePropHeader>();
    m_name_hash = header.name_hash;
    auto orig_offset = buffer.get_position();
    switch (header.prop_type) {
        case PropType::NONE: {
            // Nothing to read
            break;
        }
        case PropType::U32: {
            m_value = header.data_raw.uint_value;
            break;
        }
        case PropType::F32: {
            m_value = header.data_raw.float_value;
            break;
        }
        case PropType::STR: {
            buffer.set_position(header.data_raw.uint_value, std::ios::beg);
            auto &v = m_value.emplace<std::string>();
            buffer.read_cstring(v);
            break;
        }
        case PropType::VEC2: {
            buffer.set_position(header.data_raw.uint_value, std::ios::beg);
            auto &v = m_value.emplace<glm::vec2>();
            v = buffer.read_pod<glm::vec2>();
            break;
        }
        case PropType::VEC3: {
            buffer.set_position(header.data_raw.uint_value, std::ios::beg);
            auto &v = m_value.emplace<glm::vec3>();
            v = buffer.read_pod<glm::vec3>();
            break;
        }
        case PropType::VEC4: {
            buffer.set_position(header.data_raw.uint_value, std::ios::beg);
            auto &v = m_value.emplace<glm::vec4>();
            v = buffer.read_pod<glm::vec4>();
            break;
        }
        case PropType::MAT3X3: {
            buffer.set_position(header.data_raw.uint_value, std::ios::beg);
            auto &v = m_value.emplace<glm::mat3>();
            v = buffer.read_pod<glm::mat3>();
            break;
        }
        case PropType::MAT4X4: {
            buffer.set_position(header.data_raw.uint_value, std::ios::beg);
            auto &v = m_value.emplace<glm::mat4>();
            v = buffer.read_pod<glm::mat4>();
            break;
        }
        case PropType::ARRAY_U32: {
            buffer.set_position(header.data_raw.uint_value, std::ios::beg);
            auto array_size = buffer.read_pod<uint32>();
            auto &v = m_value.emplace<std::vector<uint32> >();
            v.reserve(array_size);
            if (array_size > 0) {
                buffer.read_exact(v);
            }
            break;
        }
        case PropType::ARRAY_F32: {
            buffer.set_position(header.data_raw.uint_value, std::ios::beg);
            auto array_size = buffer.read_pod<uint32>();
            auto &v = m_value.emplace<std::vector<float32> >();
            v.reserve(array_size);
            if (array_size > 0) {
                buffer.read_exact(v);
            }
            break;
        }
        case PropType::ARRAY_U8: {
            buffer.set_position(header.data_raw.uint_value, std::ios::beg);
            auto array_size = buffer.read_pod<uint32>();
            auto &v = m_value.emplace<std::vector<uint8> >();
            if (array_size > 0) {
                buffer.read_exact(v);
            }
            break;
        }
        case PropType::OBJID: {
            buffer.set_position(header.data_raw.uint_value, std::ios::beg);
            auto &v = m_value.emplace<uint64>();
            v = buffer.read_pod<uint64>();
            break;
        }
        case PropType::EVENT: {
            buffer.set_position(header.data_raw.uint_value, std::ios::beg);
            auto array_size = buffer.read_pod<uint32>();
            auto &v = m_value.emplace<std::vector<RuntimeEvent> >();
            v.reserve(array_size);
            for (uint32 i = 0; i < array_size; ++i) {
                auto a = buffer.read_pod<uint32>();
                auto b = buffer.read_pod<uint32>();
                v.emplace_back(a, b);
            }
            break;
        }
        default: {
            printf("[ERROR]: Unknown RTPC property type in RuntimeProp constructor: %d\n", header.prop_type);
            abort();
        }
    }
    buffer.set_position(orig_offset, std::ios::beg);
}

RuntimeNode::RuntimeNode(IO::File &buffer) {
    const auto [name_hash, data_offset, prop_count, child_count] = buffer.read_pod<RuntimeNodeHeader>();

    m_name_hash = name_hash;
    m_children.reserve(child_count);
    m_props.reserve(prop_count);

    auto orig_pos = buffer.get_position();
    buffer.set_position(data_offset, std::ios::beg);

    for (int i = 0; i < prop_count; ++i) {
        RuntimeProp prop(buffer);
        uint32 hash = prop.hash();
        m_props.emplace(hash, prop);
    }
    // Align buffer position to 4
    buffer.align(4);

    for (int i = 0; i < child_count; ++i) {
        m_children.emplace_back(buffer);
    }
    buffer.set_position(orig_pos, std::ios::beg);
}

bool RuntimeNode::has(const std::string_view name) const {
    return has(hash_string(name));
}


RuntimeNode RuntimeNode::RootNode(const std::unique_ptr<IO::File> &file) {
    const auto header = file->read_pod<RTPCHeader>();
    if (std::memcmp(header.ident, "RTPC", 4) != 0) {
        throw std::runtime_error("Invalid RTPC header");
    }
    return RuntimeNode(*file);
}

json RuntimeNode::to_json() const {
    json node;
    auto& props = node["props"];
    auto& children = node["children"];
    for (const auto &[hash, prop]: m_props) {
        const auto name = find_name(hash).value_or(std::to_string(hash));
        json value;
        auto &prop_value = prop.value();
        if (const auto str = std::get_if<std::string>(&prop_value)) {
            value = *str;
        }
        else if (const auto flt = std::get_if<float32>(&prop_value)) {
            value = *flt;
        }
        else if (const auto int_ = std::get_if<uint32>(&prop_value)) {
            value = *int_;
        }
        else if (const auto int_ = std::get_if<uint64>(&prop_value)) {
            value = *int_;
        }
        else if (const auto vec = std::get_if<glm::vec2>(&prop_value)) {
            value = {vec->x, vec->y};
        }
        else if (const auto vec = std::get_if<glm::vec3>(&prop_value)) {
            value = {vec->x, vec->y, vec->z};
        }
        else if (const auto vec = std::get_if<glm::vec4>(&prop_value)) {
            value = {vec->x, vec->y, vec->z, vec->w};
        }
        else if (const auto mat = std::get_if<glm::mat3>(&prop_value)) {
            const auto value_ptr = glm::value_ptr(*mat);
            value = {
                value_ptr[0], value_ptr[1], value_ptr[2],
                value_ptr[3], value_ptr[4], value_ptr[5],
                value_ptr[6], value_ptr[7], value_ptr[8]
            };
        }
        else if (const auto mat = std::get_if<glm::mat4>(&prop_value)) {
            const auto value_ptr = glm::value_ptr(*mat);
            value = {
                value_ptr[0], value_ptr[1], value_ptr[2], value_ptr[3],
                value_ptr[4], value_ptr[5], value_ptr[6], value_ptr[7],
                value_ptr[8], value_ptr[9], value_ptr[10], value_ptr[11],
                value_ptr[12], value_ptr[13], value_ptr[14], value_ptr[15]
            };
        }
        else if (const auto array = std::get_if<std::vector<uint32> >(&prop_value)) {
            value = *array;
        }
        else if (const auto array = std::get_if<std::vector<float32> >(&prop_value)) {
            value = *array;
        }
        else if (const auto array = std::get_if<std::vector<uint8> >(&prop_value)) {
            value = *array;
        }
        else if (const auto array = std::get_if<std::vector<RuntimeEvent> >(&prop_value)) {
            std::vector<json> events;
            events.reserve(array->size());
            for (const auto &event: *array) {
                events.emplace_back(json{event.a, event.b});
            }
            value = events;
        }
        else if (const auto array = std::get_if<uint64>(&prop_value)) {
            value = *array;
        }
        else {
            throw std::runtime_error("Unknown prop type");
        }

        props[name] = value;
    }

    for (const auto &child: m_children) {
        children.push_back(child.to_json());
    }

    return node;
}

bool RuntimeNode::has(const uint32 hash) const {
    return m_props.contains(hash);
}
