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

namespace {
void require_range(IO::File &buffer, uint64 offset, uint64 size) {
    const auto length = buffer.get_size();
    if (offset > length || size > length - offset)
        throw std::runtime_error("RTPC data exceeds file bounds");
}

void validate_version(uint32 version) {
    if (version < 1 || version > 3)
        throw std::runtime_error("Unsupported RTPC version: " + std::to_string(version));
}

template<typename T>
void read_array(IO::File &buffer, std::vector<T> &value, uint32 count) {
    require_range(buffer, buffer.get_position(), uint64(count) * sizeof(T));
    value.resize(count);
    buffer.read_exact(value);
}
}

static_assert(sizeof(RTPCHeader) == 8);
static_assert(sizeof(RuntimeNodeHeader) == 12);
static_assert(sizeof(RuntimePropHeader) == 9);

RuntimeProp::RuntimeProp(IO::File &buffer) {
    require_range(buffer, buffer.get_position(), sizeof(RuntimePropHeader));
    const auto header = buffer.read_pod<RuntimePropHeader>();
    if (header.prop_type >= PropType::STR)
        require_range(buffer, header.data_raw.uint_value, 1);
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
            while (true) {
                const auto c = buffer.read_pod<char>();
                if (!c) break;
                v.push_back(c);
            }
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
            read_array(buffer, v, array_size);
            break;
        }
        case PropType::ARRAY_F32: {
            buffer.set_position(header.data_raw.uint_value, std::ios::beg);
            auto array_size = buffer.read_pod<uint32>();
            auto &v = m_value.emplace<std::vector<float32> >();
            read_array(buffer, v, array_size);
            break;
        }
        case PropType::ARRAY_U8: {
            buffer.set_position(header.data_raw.uint_value, std::ios::beg);
            auto array_size = buffer.read_pod<uint32>();
            auto &v = m_value.emplace<std::vector<uint8> >();
            read_array(buffer, v, array_size);
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
            require_range(buffer, buffer.get_position(), uint64(array_size) * 8);
            v.reserve(array_size);
            for (uint32 i = 0; i < array_size; ++i) {
                auto a = buffer.read_pod<uint32>();
                auto b = buffer.read_pod<uint32>();
                v.emplace_back(a, b);
            }
            break;
        }
        default: {
            throw std::runtime_error("Unsupported RTPC property type: " +
                                     std::to_string(static_cast<uint8>(header.prop_type)));
        }
    }
    buffer.set_position(orig_offset, std::ios::beg);
}

RuntimeNode::RuntimeNode(IO::File &buffer, uint32 version) : RuntimeNode(buffer, version, 0) {}

RuntimeNode::RuntimeNode(IO::File &buffer, uint32 version, size_t depth) : m_version(version) {
    validate_version(version);
    if (depth >= 8192) throw std::runtime_error("RTPC node nesting exceeds limit");
    require_range(buffer, buffer.get_position(), sizeof(RuntimeNodeHeader));
    const auto [name_hash, data_offset, prop_count, child_count] = buffer.read_pod<RuntimeNodeHeader>();

    const uint64 children_offset = (uint64(data_offset) + uint64(prop_count) * 9 + 3) & ~uint64(3);
    require_range(buffer, data_offset, children_offset - data_offset + uint64(child_count) * 12 + (version == 3 ? 4 : 0));
    m_name_hash = name_hash;
    m_children.reserve(child_count);
    m_props.reserve(prop_count);

    auto orig_pos = buffer.get_position();
    buffer.set_position(data_offset, std::ios::beg);

    for (int i = 0; i < prop_count; ++i) {
        RuntimeProp prop(buffer);
        uint32 hash = prop.hash();
        m_props.emplace(hash, std::move(prop));
    }
    // Align buffer position to 4
    buffer.align(4);

    for (int i = 0; i < child_count; ++i) {
        m_children.push_back(RuntimeNode(buffer, version, depth + 1));
    }
    if (version == 3) m_v3_metadata = buffer.read_pod<uint32>();
    buffer.set_position(orig_pos, std::ios::beg);
}

bool RuntimeNode::has(const std::string_view name) const {
    return has(rtpc_name_hash(name));
}


RuntimeNode RuntimeNode::RootNode(const std::unique_ptr<IO::File> &file) {
    const auto header = file->read_pod<RTPCHeader>();
    if (std::memcmp(header.ident, "RTPC", 4) != 0) {
        throw std::runtime_error("Invalid RTPC header");
    }
    validate_version(header.version);
    return RuntimeNode(*file, header.version);
}

json RuntimeNode::to_json() const {
    json node;
    if (m_v3_metadata) node["v3_metadata"] = *m_v3_metadata;
    auto& props = node["props"];
    auto& children = node["children"];
    for (const auto &[hash, prop]: m_props) {
        const auto name = find_lookup3_name(hash).value_or(std::to_string(hash));
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
