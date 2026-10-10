//
// Created by red_eye on 10/10/26.
//

#include "apex/pcbb/pcbb.hpp"


#include "apex/hashes.h"
#include <utility>

namespace {
const Apex::PCBB::Property *find_property(const std::vector<Apex::PCBB::Property> &properties, uint32 hash) {
    for (const auto &property: properties) {
        if (property.name_hash == hash) return &property;
    }
    return nullptr;
}

std::optional<std::string> property_string(const Apex::PCBB::Property *property, uint32 hash) {
    if (!property) throw std::out_of_range("Property not found: " + std::to_string(hash));
    if (const auto *value = std::get_if<std::string>(&property->value)) return *value;
    if (const auto *value = std::get_if<uint32>(&property->value)) return find_lookup3_name(*value);
    return std::nullopt;
}

std::string property_name(uint32 hash) {
    switch (static_cast<Apex::PCBB::PropertyKey>(hash)) {
        case Apex::PCBB::PropertyKey::Class: return "_class";
        case Apex::PCBB::PropertyKey::Name: return "name";
        case Apex::PCBB::PropertyKey::ResourceFile: return "resource_file";
        case Apex::PCBB::PropertyKey::Filename: return "filename";
    }
    if (auto name = find_lookup3_name(hash)) return std::move(*name);
    return std::to_string(hash);
}

nlohmann::json properties_to_json(const std::vector<Apex::PCBB::Property> &properties) {
    auto object = nlohmann::json::object();
    for (const auto &property: properties) {
        object[property_name(property.name_hash)] = property.to_json();
    }
    return object;
}
}

const Apex::PCBB::Property *Apex::PCBB::PropertySection::find(uint32 hash) const {
    return find_property(properties, hash);
}

const Apex::PCBB::Property *Apex::PCBB::Property::find(uint32 hash) const {
    return find_property(children, hash);
}

std::optional<std::string> Apex::PCBB::PropertySection::get_string(uint32 hash) const {
    return property_string(find(hash), hash);
}

std::optional<std::string> Apex::PCBB::Property::get_string(uint32 hash) const {
    return property_string(find(hash), hash);
}

nlohmann::json Apex::PCBB::PropertySection::to_json() const {
    return properties_to_json(properties);
}

nlohmann::json Apex::PCBB::Property::to_json() const {
    if (std::holds_alternative<std::monostate>(value)) return properties_to_json(children);
    if (const auto *word = std::get_if<uint32>(&value)) return *word;
    if (const auto *number = std::get_if<float>(&value)) return *number;
    if (const auto *text = std::get_if<std::string>(&value)) return *text;
    if (const auto *vec = std::get_if<glm::vec2>(&value)) return nlohmann::json::array({vec->x, vec->y});
    if (const auto *vec = std::get_if<glm::vec3>(&value)) return nlohmann::json::array({vec->x, vec->y, vec->z});
    if (const auto *vec = std::get_if<glm::vec4>(&value)) return nlohmann::json::array({vec->x, vec->y, vec->z, vec->w});
    if (const auto *buffer = std::get_if<IO::Buffer>(&value)) return {std::vector(buffer->data(), buffer->data() + buffer->size())};

    const auto &matrix = std::get<glm::mat4>(value);
    auto array = nlohmann::json::array();
    auto &elements = array.get_ref<nlohmann::json::array_t &>();
    elements.reserve(16);
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) elements.emplace_back(matrix[column][row]);
    }
    return array;
}

nlohmann::json Apex::PCBB::PropertyFile::to_json() const {
    auto array = nlohmann::json::array();
    auto &elements = array.get_ref<nlohmann::json::array_t &>();
    elements.reserve(sections.size());
    for (const auto &section: sections) elements.emplace_back(section.to_json());
    return array;
}

Apex::PCBB::PropertySection Apex::PCBB::PropertySection::from_buffer(const IO::ConstByteBufferView &buffer) {
    uint32 offset = 0;
    const auto buffer_size = buffer.size();
    std::vector<Property> properties;
    while (offset < buffer_size) {
        while (true) {
            const PropertyHeader &child_header = buffer.readonly_view_as<PropertyHeader>(offset).at(0);
            properties.emplace_back(Property::from_buffer(buffer, offset));
            offset = child_header.next_ptr;
            if (offset == 0xFFFFFFFF) {
                break;
            }
        }
    }
    return {std::move(properties)};
}

Apex::PCBB::Property Apex::PCBB::Property::from_buffer(const IO::ConstByteBufferView &buffer, uint32 base_offset) {
    const auto &header = buffer.readonly_view_as<PropertyHeader>(base_offset).at(0);

    switch (header.form) {
        case PropertyForm::Group: {
            const auto &group_view = buffer.subview(header.data_ptr);
            const auto first_child_offset = group_view.readonly_view_as<uint32>().at(0);
            std::vector<Property> children;
            auto offset = first_child_offset;
            while (true) {
                const PropertyHeader &child_header = buffer.readonly_view_as<PropertyHeader>(offset).at(0);
                children.emplace_back(from_buffer(buffer, offset));
                offset = child_header.next_ptr;
                if (offset == 0xFFFFFFFF) {
                    break;
                }
            }
            return {header.name_hash, PropertyValue{}, std::move(children)};
        }
        case PropertyForm::Leaf: {
            const auto &leaf_view = buffer.subview(header.data_ptr);
            const auto prop_type = leaf_view.readonly_view_as<PropertyType>().at(0);
            switch (prop_type) {
                case PropertyType::I32:
                    return {header.name_hash, PropertyValue{leaf_view.readonly_view_as<uint32>().at(1)}};
                case PropertyType::F32:
                    return {header.name_hash, PropertyValue{leaf_view.readonly_view_as<float>().at(1)}};
                case PropertyType::STRING: {
                    const auto string_offset = leaf_view.readonly_view_as<uint32>().at(1);
                    const auto string_view = buffer.readonly_view_as<char>(string_offset);
                    return {header.name_hash, PropertyValue{std::string(string_view.data())}};
                }
                case PropertyType::VEC2: {
                    const auto value_offset = leaf_view.readonly_view_as<uint32>().at(1);
                    const auto value_view = buffer.readonly_view_as<float>(value_offset, 2);
                    return {header.name_hash, PropertyValue{glm::vec2(value_view[0], value_view[1])}};
                }
                case PropertyType::VEC3: {
                    const auto value_offset = leaf_view.readonly_view_as<uint32>().at(1);
                    const auto value_view = buffer.readonly_view_as<float>(value_offset, 3);
                    return {header.name_hash, PropertyValue{glm::vec3(value_view[0], value_view[1], value_view[2])}};
                }
                case PropertyType::VEC4: {
                    const auto value_offset = leaf_view.readonly_view_as<uint32>().at(1);
                    const auto value_view = buffer.readonly_view_as<float>(value_offset, 4);
                    return {
                        header.name_hash,
                        PropertyValue{glm::vec4(value_view[0], value_view[1], value_view[2], value_view[3])}
                    };
                }
                case PropertyType::MAT4x4: {
                    const auto value_offset = leaf_view.readonly_view_as<uint32>().at(1);
                    const auto value_view = buffer.readonly_view_as<float>(value_offset, 16);
                    return {
                        header.name_hash, PropertyValue{
                            glm::mat4x4(value_view[0], value_view[1], value_view[2], value_view[3],
                                        value_view[4], value_view[5], value_view[6], value_view[7],
                                        value_view[8], value_view[9], value_view[10], value_view[11],
                                        value_view[12], value_view[13], value_view[14], value_view[15])
                        }
                    };
                }
                case PropertyType::BLOB: {
                    const auto value_offset = leaf_view.readonly_view_as<uint32>().at(1);
                    const auto value_size = buffer.readonly_view_as<uint32>(value_offset, 1).at(0);
                    const auto value_view = buffer.readonly_view_as<uint8>(value_offset + 4, value_size);
                    return {
                        header.name_hash,
                        PropertyValue{IO::Buffer::copy_of(value_view)}
                    };
                }
            }
            throw std::runtime_error("Invalid property type");
        }
    }
    throw std::runtime_error("Invalid property form");
}

Apex::PCBB::PropertyFile Apex::PCBB::PropertyFile::from_buffer(const IO::ConstByteBufferView &buffer) {

    uint64 offset = 0;
    const uint64 buffer_size = buffer.size();

    std::vector<PropertySection> sections;
    while (offset < buffer_size) {
        const auto &[ident, size] = buffer.readonly_view_as<PropertySectionHeader>(offset).at(0);
        if (!std::ranges::equal(ident, PCBB_MAGIC)) {
            throw std::runtime_error("Invalid PCBB file");
        }

        offset += sizeof(PropertySectionHeader);
        const auto &slice = buffer.subview(offset, size);
        offset += size;
        auto section = PropertySection::from_buffer(slice);
        sections.emplace_back(std::move(section));
    }
    return {std::move(sections)};
}

Apex::PCBB::PropertyFile Apex::PCBB::PropertyFile::from_file(IO::File &file) {
    IO::Buffer buffer = IO::Buffer::of_fixed_size(file.get_size());
    file.read(buffer.data(), buffer.size());
    return from_buffer(buffer.readonly_view());
}

