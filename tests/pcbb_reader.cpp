#include "apex/pcbb/pcbb.hpp"

#include <cstddef>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
// Name resolution is supplied by the asset database in the game module.
std::optional<std::string> find_lookup3_name(uint32 hash) {
    if (hash == 0xB7B9F367) return "child_value";
    return std::nullopt;
}


namespace {
using Apex::PCBB::PropertyKey;
static_assert(static_cast<uint32>(PropertyKey::Class) == 0x1473B179);
static_assert(static_cast<uint32>(PropertyKey::ResourceFile) == 0x2ACF24F6);
static_assert(static_cast<uint32>(PropertyKey::Filename) == 0x32B409E0);
static_assert(static_cast<uint32>(PropertyKey::Name) == 0xD31AB684);

void check(bool condition) {
    if (!condition) throw std::runtime_error("PCBB section regression failed");
}

void put32(std::vector<uint8> &bytes, size_t offset, uint32 value) {
    for (size_t i = 0; i < 4; ++i) bytes.at(offset + i) = static_cast<uint8>(value >> (8 * i));
}

bool is_u32(const Apex::PCBB::PropertyValue &value, uint32 expected) {
    const auto *word = std::get_if<uint32>(&value);
    return word && *word == expected;
}

template<typename Exception, typename Function>
bool throws(Function &&function) {
    try {
        function();
    } catch (const Exception &) {
        return true;
    }
    return false;
}
}

int main() {
    try {
        // A short first object followed by a grouped second object. All node
        // and payload offsets are relative to their own chunk's body.
        std::vector<uint8> bytes(0xE0, 0xDD);
        std::memcpy(bytes.data(), "PCBB", 4);
        put32(bytes, 4, 0x30);
        put32(bytes, 0x08, 0x22222222); // first chunk's sole leaf
        put32(bytes, 0x0C, 2);
        put32(bytes, 0x10, 0x10);
        put32(bytes, 0x14, 0xFFFFFFFF);
        put32(bytes, 0x18, 1);
        put32(bytes, 0x1C, 0x12345678);

        std::memcpy(bytes.data() + 0x38, "PCBB", 4);
        put32(bytes, 0x3C, 0xA0);
        const size_t body = 0x40;
        put32(bytes, body, 0x01020304); // group with two ordered children
        put32(bytes, body + 4, 1);
        put32(bytes, body + 8, 0x10);
        put32(bytes, body + 12, 0x60);
        put32(bytes, body + 16, 0x20);

        put32(bytes, body + 0x20, 0x33333333); // child: high-bit inline word
        put32(bytes, body + 0x24, 2);
        put32(bytes, body + 0x28, 0x30);
        put32(bytes, body + 0x2C, 0x40);
        put32(bytes, body + 0x30, 1);
        put32(bytes, body + 0x34, 0xF907D551);

        put32(bytes, body + 0x40, 0xB7B9F367); // second child: "child_value"
        put32(bytes, body + 0x44, 2);
        put32(bytes, body + 0x48, 0x50);
        put32(bytes, body + 0x4C, 0xFFFFFFFF);
        put32(bytes, body + 0x50, 1);
        put32(bytes, body + 0x54, 0x10203040);

        put32(bytes, body + 0x60, 0x1473B179); // next root: _class
        put32(bytes, body + 0x64, 2);
        put32(bytes, body + 0x68, 0x70);
        put32(bytes, body + 0x6C, 0xFFFFFFFF);
        put32(bytes, body + 0x70, 3);
        put32(bytes, body + 0x74, 0x80);
        std::memcpy(bytes.data() + body + 0x80, "CGeometryObject", sizeof("CGeometryObject"));

        const auto file = Apex::PCBB::PropertyFile::from_buffer({bytes.data(), bytes.size()});
        check(file.sections.size() == 2);
        const auto &first = file.sections[0].properties;
        check(first.size() == 1 && first[0].name_hash == 0x22222222);
        check(is_u32(first[0].value, 0x12345678));
        const auto &second = file.sections[1].properties;
        check(second.size() == 2);
        const auto &group = second[0];
        check(group.name_hash == 0x01020304);
        check(std::holds_alternative<std::monostate>(group.value));
        check(group.children.size() == 2);
        check(group.children[0].name_hash == 0x33333333);
        check(is_u32(group.children[0].value, 0xF907D551));
        check(group.children[1].name_hash == 0xB7B9F367);
        check(is_u32(group.children[1].value, 0x10203040));
        check(second[1].name_hash == 0x1473B179);
        check(std::get<std::string>(second[1].value) == "CGeometryObject");
        const auto &section = file.sections[1];
        check(section.has(0x01020304) && section.has("_class"));
        check(!section.has("missing"));
        check(section.find("_class") == &second[1]);
        check(section.get<std::string>("_class") == "CGeometryObject");
        check(section.is<std::string>(0x1473B179));
        check(!section.is<uint32>("_class"));
        check(section.is<std::monostate>(0x01020304));
        check(section.get_string("_class") == std::optional<std::string>{"CGeometryObject"});
        check(section.find(PropertyKey::Class) == &second[1]);
        check(section.has(PropertyKey::Class));
        check(section.get<std::string>(PropertyKey::Class) == "CGeometryObject");
        check(section.is<std::string>(PropertyKey::Class));
        check(section.get_string(PropertyKey::Class) == std::optional<std::string>{"CGeometryObject"});
        check(!section.get_string(0x01020304));

        check(group.has(0x33333333) && !group.has("_class"));
        check(!group.has(PropertyKey::Class));
        check(group.find(PropertyKey::Class) == nullptr);
        check(group.find(0x33333333) == &group.children[0]);
        check(group.get<uint32>(0x33333333) == 0xF907D551);
        check(group.is<uint32>(0x33333333));
        check(group.has("child_value"));
        check(group.find("child_value") == &group.children[1]);
        check(group.is<uint32>("child_value"));
        check(group.get<uint32>("child_value") == 0x10203040);
        check(!group.get_string(0x33333333));
        check(throws<std::runtime_error>([&] { (void)section.get<uint32>("_class"); }));
        check(throws<std::runtime_error>([&] { (void)section.get<uint32>("missing"); }));
        check(throws<std::out_of_range>([&] { (void)section.is<uint32>("missing"); }));
        check(throws<std::out_of_range>([&] { (void)section.get_string("missing"); }));
        const auto output = file.to_json();
        check(output.is_array() && output.size() == 2);
        check(output.at(0).is_object() && output.at(0).size() == 1);
        check(output.at(0).at(std::to_string(0x22222222)) == 0x12345678);
        const auto &second_json = output.at(1);
        check(second_json.is_object() && second_json.size() == 2);
        check(second_json.at("_class") == "CGeometryObject");
        const auto &group_json = second_json.at(std::to_string(0x01020304));
        check(group_json.is_object() && group_json.size() == 2);
        check(group_json.at(std::to_string(0x33333333)) == 0xF907D551u);
        check(group_json.at("child_value") == 0x10203040);

        glm::mat4 matrix(1.0f);
        matrix[0][1] = 2.5f;
        matrix[3][2] = 9.5f;
        const Apex::PCBB::PropertySection values{{
            {static_cast<uint32>(PropertyKey::Name), std::string("object"), {}},
            {static_cast<uint32>(PropertyKey::ResourceFile), std::string("asset.lod"), {}},
            {static_cast<uint32>(PropertyKey::Filename), std::string("mesh.lod"), {}},
            {0x100, 1.5f, {}},
            {0x101, glm::vec3{1.0f, 2.0f, 3.0f}, {}},
            {0x102, glm::vec4{4.0f, 5.0f, 6.0f, 7.0f}, {}},
            {0x103, matrix, {}},
            {0x104, std::monostate{}, {}},
        }};
        const auto value_json = values.to_json();
        check(value_json.at("name") == "object");
        check(value_json.at("resource_file") == "asset.lod");
        check(value_json.at("filename") == "mesh.lod");
        check(value_json.at("256") == 1.5f);
        check(value_json.at("257") == nlohmann::json::array({1.0f, 2.0f, 3.0f}));
        check(value_json.at("258") == nlohmann::json::array({4.0f, 5.0f, 6.0f, 7.0f}));
        check(value_json.at("259") == nlohmann::json::array({
            1.0f, 2.5f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 9.5f, 1.0f,
        }));
        check(value_json.at("260") == nlohmann::json::object());
        std::cout << "PCBB two-section regression passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
