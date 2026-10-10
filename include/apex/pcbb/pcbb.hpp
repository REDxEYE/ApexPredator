//
// Created by red_eye on 10/10/26.
//

#pragma once
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "int_def.h"
#include "glm/glm.hpp"
#include "redscore/platform/buffer/buffer.h"
#include "redscore/platform/file/file.h"
#include "utils/hash_helper.h"
#include "utils/lookup3.h"

constexpr uint8 PCBB_MAGIC[] = {'P', 'C', 'B', 'B'};

namespace Apex::PCBB {
    inline uint32 pcbb_name_hash(std::string_view name) {
        return hashlittle(name.data(), name.size(), 0);
    }

    enum class PropertyForm:uint32 {
        Group = 1,
        Leaf = 2
    };

    enum class PropertyType:uint32 {
        I32 = 1,
        F32 = 2,
        STRING = 3,
        VEC2 = 4,
        VEC3 = 5,
        VEC4 = 6,
        MAT4x4 = 8,
        BLOB = 10,
    };

    enum class PropertyKey : uint32 {
        Class = const_hash_string("_class"),
        Name = const_hash_string("name"),
        ResourceFile = const_hash_string("resource_file"),
        Filename = const_hash_string("filename"),
    };

    static bool operator==(PropertyKey lhs, PropertyKey rhs) {
        return static_cast<uint32_t>(lhs) == static_cast<uint32_t>(rhs);
    }

    static bool operator==(const PropertyKey lhs, const uint32 rhs) {
        return std::to_underlying(lhs) == rhs;
    }

    struct PropertySectionHeader {
        uint8 ident[4];
        uint32 size;
    };


    struct PropertyHeader {
        uint32 name_hash;
        PropertyForm form;
        uint32 data_ptr;
        uint32 next_ptr;
    };

    struct Property;

    using PropertyValue = std::variant<std::monostate, uint32, float, std::string, glm::vec2, glm::vec3, glm::vec4,
        glm::mat4, IO::Buffer>;

    struct PropertySection {
        std::vector<Property> properties;

        [[nodiscard]] const Property *find(uint32 hash) const;

        [[nodiscard]] const Property *find(PropertyKey key) const { return find(static_cast<uint32>(key)); }
        [[nodiscard]] const Property *find(std::string_view name) const { return find(pcbb_name_hash(name)); }

        [[nodiscard]] bool has(uint32 hash) const { return find(hash) != nullptr; }
        [[nodiscard]] bool has(PropertyKey key) const { return has(static_cast<uint32>(key)); }
        [[nodiscard]] bool has(std::string_view name) const { return has(pcbb_name_hash(name)); }

        template<typename T>
        const T &get(uint32 hash) const;

        template<typename T>
        const T &get(PropertyKey key) const { return get<T>(static_cast<uint32>(key)); }

        template<typename T>
        const T &get(std::string_view name) const { return get<T>(pcbb_name_hash(name)); }

        template<typename T>
        bool is(uint32 hash) const;

        template<typename T>
        bool is(PropertyKey key) const { return is<T>(static_cast<uint32>(key)); }

        template<typename T>
        bool is(std::string_view name) const { return is<T>(pcbb_name_hash(name)); }

        std::optional<std::string> get_string(uint32 hash) const;

        std::optional<std::string> get_string(PropertyKey key) const {
            return get_string(static_cast<uint32>(key));
        }

        std::optional<std::string> get_string(std::string_view name) const {
            return get_string(pcbb_name_hash(name));
        }


        [[nodiscard]] nlohmann::json to_json() const;

        static PropertySection from_buffer(const IO::ConstByteBufferView &buffer);
    };

    struct Property {
        uint32 name_hash;
        PropertyValue value;
        std::vector<Property> children;

        [[nodiscard]] const Property *find(uint32 hash) const;

        [[nodiscard]] const Property *find(PropertyKey key) const { return find(static_cast<uint32>(key)); }
        [[nodiscard]] const Property *find(std::string_view name) const { return find(pcbb_name_hash(name)); }

        [[nodiscard]] bool has(uint32 hash) const { return find(hash) != nullptr; }
        [[nodiscard]] bool has(PropertyKey key) const { return has(static_cast<uint32>(key)); }
        [[nodiscard]] bool has(std::string_view name) const { return has(pcbb_name_hash(name)); }

        template<typename T>
        const T &get(uint32 hash) const;

        template<typename T>
        const T &get(PropertyKey key) const { return get<T>(static_cast<uint32>(key)); }

        template<typename T>
        const T &get(std::string_view name) const { return get<T>(pcbb_name_hash(name)); }

        template<typename T>
        bool is(uint32 hash) const;

        template<typename T>
        bool is(PropertyKey key) const { return is<T>(static_cast<uint32>(key)); }

        template<typename T>
        bool is(std::string_view name) const { return is<T>(pcbb_name_hash(name)); }

        std::optional<std::string> get_string(uint32 hash) const;

        std::optional<std::string> get_string(PropertyKey key) const {
            return get_string(static_cast<uint32>(key));
        }

        std::optional<std::string> get_string(std::string_view name) const {
            return get_string(pcbb_name_hash(name));
        }


        [[nodiscard]] nlohmann::json to_json() const;

        static Property from_buffer(const IO::ConstByteBufferView &buffer, uint32 base_offset);
    };

    namespace detail {
        template<typename T>
        const T &get_value(const Property *property, uint32 hash) {
            if (!property) throw std::runtime_error("Property not found: " + std::to_string(hash));
            if (const auto *value = std::get_if<T>(&property->value)) return *value;
            throw std::runtime_error("Property type mismatch for hash: " + std::to_string(hash));
        }

        template<typename T>
        bool is_value(const Property *property, uint32 hash) {
            if (!property) throw std::out_of_range("Property not found: " + std::to_string(hash));
            return std::holds_alternative<T>(property->value);
        }
    }

    template<typename T>
    const T &PropertySection::get(uint32 hash) const { return detail::get_value<T>(find(hash), hash); }

    template<typename T>
    bool PropertySection::is(uint32 hash) const { return detail::is_value<T>(find(hash), hash); }

    template<typename T>
    const T &Property::get(uint32 hash) const { return detail::get_value<T>(find(hash), hash); }

    template<typename T>
    bool Property::is(uint32 hash) const { return detail::is_value<T>(find(hash), hash); }


    struct PropertyFile {
        std::vector<PropertySection> sections;

        [[nodiscard]] nlohmann::json to_json() const;

        static PropertyFile from_buffer(const IO::ConstByteBufferView &buffer);

        static PropertyFile from_file(IO::File &file);
    };
}
