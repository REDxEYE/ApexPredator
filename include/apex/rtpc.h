// Created by RED on 03.10.2025.

#ifndef APEXPREDATOR_RTPC_H
#define APEXPREDATOR_RTPC_H
#include <vector>
#include <array>
#include <unordered_map>
#include <variant>

#include <glm/glm.hpp>

#include "int_def.h"
#include "utils/lookup3.h"
#include <optional>
#include <string_view>
#include "redscore/platform/file/file.h"

#define RTPC_MAGIC "RTPC"

#include "hashes.h"
#include "nlohmann/json.hpp"


enum class PropType:uint8 {
    NONE = 0,
    U32 = 1,
    F32 = 2,
    STR = 3,
    VEC2 = 4,
    VEC3 = 5,
    VEC4 = 6,
    MAT3X3 = 7,
    MAT4X4 = 8,
    ARRAY_U32 = 9,
    ARRAY_F32 = 10,
    ARRAY_U8 = 11,
    DEPRECIATED_12 = 12,
    OBJID = 13,
    EVENT = 14,
    UNK_15 = 15,
    UNK_16 = 16,
};

typedef struct RuntimeEvent {
    uint32 a;
    uint32 b;
} RuntimeEvent;


using PropValue = std::variant<
    uint32,
    float32,
    std::string,
    glm::vec2,
    glm::vec3,
    glm::vec4,
    glm::mat3,
    glm::mat4,
    std::vector<uint32>,
    std::vector<float32>,
    std::vector<std::uint8_t>,
    uint64,
    std::vector<RuntimeEvent>
>;

inline uint32 rtpc_name_hash(std::string_view name) {
    return hashlittle(name.data(), name.size(), 0);
}

class RuntimeProp {
public:
    explicit RuntimeProp(IO::File &buffer);

    [[nodiscard]] uint32 hash() const { return m_name_hash; }
    [[nodiscard]] const PropValue &value() const { return m_value; }

private:
    uint32 m_name_hash;
    PropValue m_value;
};

class RuntimeNode {
public:
    explicit RuntimeNode(IO::File &buffer, uint32 version = 1);

    template<typename T>
    const T &get(const uint32 hash) const {
        const auto it = m_props.find(hash);
        if (it == m_props.end()) {
            throw std::runtime_error("Property not found: " + std::to_string(hash));
        }
        const RuntimeProp &prop = it->second;
        if (!std::holds_alternative<T>(prop.value())) {
            throw std::runtime_error("Property type mismatch for hash: " + std::to_string(hash));
        }
        return std::get<T>(prop.value());
    }

    template<typename T>
    const T &get(const std::string_view name) const {
        return get<T>(rtpc_name_hash(name));
    }

    std::optional<std::string> get_string(const std::string_view name) const {
        return get_string(rtpc_name_hash(name));
    }

    std::optional<std::string> get_string(const uint32 hash) const {
        if (is<std::string>(hash)) {
            return std::get<std::string>(m_props.at(hash).value());
        }
        if (is<uint32>(hash)) {
            return find_lookup3_name(std::get<uint32>(m_props.at(hash).value()));
        }
        return std::nullopt;
    }

    template<typename T>
    bool is(const std::string_view name) const {
        return is<T>(rtpc_name_hash(name));
    }

    template<typename T>
    bool is(const uint32 hash) const {
        const auto& prop = m_props.at(hash);
        return std::holds_alternative<T>(prop.value());
    }

    [[nodiscard]] bool has(uint32 hash) const;

    [[nodiscard]] bool has(std::string_view name) const;

    [[nodiscard]] const std::vector<RuntimeNode> &children() const { return m_children; }
    [[nodiscard]] const std::unordered_map<uint32, RuntimeProp> &props() const { return m_props; }

    [[nodiscard]] uint32 name_hash() const { return m_name_hash; }

    [[nodiscard]] uint32 version() const { return m_version; }
    // Meaning is unknown; preserve the per-node word present in version 3.
    [[nodiscard]] std::optional<uint32> v3_metadata() const { return m_v3_metadata; }

    static RuntimeNode RootNode(const std::unique_ptr<IO::File> &file);

    nlohmann::json to_json() const;

private:
    RuntimeNode(IO::File &buffer, uint32 version, size_t depth);
    uint32 m_version;
    std::optional<uint32> m_v3_metadata;
    uint32 m_name_hash;

    std::unordered_map<uint32, RuntimeProp> m_props;
    std::vector<RuntimeNode> m_children;
};

#endif //APEXPREDATOR_RTPC_H
