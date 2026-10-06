#include "apex/adf/sti.h"
#include "apex/hashes.h"
#include "apex/adf/adf_support_types.h"
#include <sstream>

#include <cstring>
#include <fstream>
#include <iostream>
#include <set>

// These fixtures contain no string hashes and need no asset database.
bool check_hash_presence(uint64) { return true; }
void store_hash_name(std::string_view) {}

static void check(bool ok) {
    if (!ok) throw std::runtime_error("ADF typegen regression failed");
}

static ADF::ADFFile names_fixture(const std::vector<std::string> &names) {
    ADF::Header header{};
    std::memcpy(header.ident, ADF_MAGIC, 4);
    header.version = 4;
    header.nametable_count = names.size();
    header.nametable_offset = sizeof(header) + 1;
    std::vector<uint8> bytes(header.nametable_offset + names.size());
    for (const auto &name : names) {
        bytes.insert(bytes.end(), name.begin(), name.end());
        bytes.push_back(0);
    }
    header.total_size = bytes.size();
    std::memcpy(bytes.data(), &header, sizeof(header));
    return ADF::ADFFile::from_buffer(bytes.data(), bytes.size());
}

static std::string read_file(const std::filesystem::path &path) {
    std::ifstream stream(path);
    return {std::istreambuf_iterator<char>(stream), {}};
}

int main(int argc, char **argv) {
    try {
        for (uint32 alignment : {8u, 16u, 32u}) {
            std::vector<uint8> bytes(alignment + 4, 0);
            std::memcpy(bytes.data(), ADF_SMALL_MAGIC, 4);
            const uint32 type_hash = 0x6F12D9D4;
            std::memcpy(bytes.data() + 4, &type_hash, 4);
            const uint8 payload[] = {0x12, 0x34, 0x56, 0x78};
            std::memcpy(bytes.data() + alignment, payload, sizeof(payload));
            auto small_adf = ADF::ADFFile::from_buffer(bytes.data(), bytes.size(), alignment);
            check(small_adf.instances().size() == 1);
            check(small_adf.instances()[0].type_hash == type_hash);
            auto data = small_adf.get_instance_data(0);
            check(data.size() == sizeof(payload));
            check(std::memcmp(data.data(), payload, sizeof(payload)) == 0);
        }
        check(argc == 2);
        // Scoped enums do not implicitly convert to the numeric std::to_string overloads.
        enum class SignedEnum : int32 { Negative = -1, Positive = 2 };
        Vector<SignedEnum> dynamic_enums;
        dynamic_enums.push_back(SignedEnum::Negative);
        dynamic_enums.push_back(static_cast<SignedEnum>(7));
        Array<SignedEnum, 2> fixed_enums;
        fixed_enums[0] = SignedEnum::Positive;
        fixed_enums[1] = SignedEnum::Negative;
        std::ostringstream dynamic_text, fixed_text;
        dynamic_enums.print(dynamic_text);
        fixed_enums.print(fixed_text);
        check(dynamic_text.str() == "[-1, 7]");
        check(fixed_text.str() == "[2, -1]");
        check(dynamic_enums.to_json() == nlohmann::json::array({"-1", "7"}));
        check(fixed_enums.to_json() == nlohmann::json::array({"2", "-1"}));
        enum class WideEnum : uint64 { Max = UINT64_MAX };
        Vector<WideEnum> wide_enums;
        wide_enums.push_back(WideEnum::Max);
        std::ostringstream wide_text;
        wide_enums.print(wide_text);
        check(wide_text.str() == "[18446744073709551615]");
        check(wide_enums.to_json() == nlohmann::json::array({"18446744073709551615"}));
        auto adf = names_fixture({"Item", "value", "A<Item>", "A2<Item>", "Kind", "Zero",
                                  "Holder", "first", "second", "items", "fixed", "kind",
                                  "Item_00000102", "A<Deferred>", "Key", "void*", "A<void*>", "A2<void*>",
                                  "PointerHolder", "pointer", "pointers", "fixedPointers", "voidPtr", "A<voidPtr>", "PlantAngles",
                                  "90RightPlantMinAngle", "90RightPlantMaxAngle", "90LeftPlantMinAngle", "90LeftPlantMaxAngle",
                                  "180RightPlantMinAngle", "180RightPlantMaxAngle", "180LeftPlantMinAngle", "180LeftPlantMaxAngle"});
        STI::TypeLibrary lib;
        auto add = [&](ADF::MetaType kind, uint32 hash, uint64 name, uint32 size,
                       ADF::TypeData data, uint32 element = 0, uint32 count = 0,
                       uint32 alignment = 4) -> const STI::Type & {
            ADF::TypeDef def{};
            def.type = kind; def.hash = hash; def.name_id = name;
            def.size = size; def.alignment = alignment;
            def.element_type_hash = element; def.element_len = count;
            return lib.register_type(ADF::Type(def, std::move(data)), adf);
        };
        auto member = [](uint64 name, uint32 hash, uint32 size, uint32 offset) {
            return ADF::StructMemberInfo{name, hash, size, offset, 0, 0, 0};
        };
        using M = ADF::MetaType;
        const std::vector<ADF::StructMemberInfo> small{member(1, STI_TYPE_HASH_UINT32, 4, 0)};
        const std::vector<ADF::StructMemberInfo> large{member(1, STI_TYPE_HASH_UINT64, 8, 0)};
        check(add(M::Structure, 0x100, 0, 4, small).name() == "Item");
        check(add(M::Structure, 0x101, 0, 8, large).name() == "Item_00000101");
        // A real source name can occupy the name we would otherwise synthesize.
        add(M::Structure, 0x200, 12, 4, small);
        check(add(M::Structure, 0x102, 0, 4, small).name() == "Item_00000102_00000102");
        const auto count = lib.types().size();
        check(add(M::Structure, 0x101, 0, 8, large).name() == "Item_00000101");
        check(lib.types().size() == count);
        bool rejected = false;
        try { add(M::Structure, 0x101, 0, 4, small); }
        catch (const std::runtime_error &) { rejected = true; }
        check(rejected);

        check(add(M::Array, 0x300, 2, 16, {}, 0x100).name() == "Item_Array");
        check(add(M::Array, 0x301, 2, 16, {}, 0x101).name() == "Item_00000301_Array");
        check(add(M::InlineArray, 0x400, 3, 8, {}, 0x100, 2).name() == "Item_InlineArray_2");
        check(add(M::InlineArray, 0x401, 3, 16, {}, 0x101, 2).name() == "Item_00000401_InlineArray_2");
        const std::vector<ADF::EnumMemberInfo> values{{5, 0}};
        add(M::Enumeration, 0x500, 4, 4, values);
        check(add(M::Enumeration, 0x501, 4, 4, values).name() == "Kind_00000501");
        check(add(M::Enumeration, 0x502, 0, 4, values).name() == "Item_00000502");
        add(M::Array, 0x600, 13, 16, {}, STI_TYPE_HASH_DEFERRED);
        check(add(M::Array, 0x601, 13, 16, {}, STI_TYPE_HASH_DEFERRED).name() == "Deferred_00000601_Array");
        add(M::StringHash, 0x700, 14, 4, {}, STI_TYPE_HASH_UINT32);
        check(add(M::StringHash, 0x701, 14, 8, {}, STI_TYPE_HASH_UINT64).name() == "Key_00000701");
        add(M::Structure, 0x800, 6, 48, std::vector<ADF::StructMemberInfo>{
            member(7, 0x100, 4, 0), member(8, 0x101, 8, 4), member(9, 0x301, 16, 12),
            member(10, 0x401, 16, 28), member(11, 0x501, 4, 44)});

        // Pointer names must be valid symbols; pointer storage must stay opaque even
        // inside dynamic/inline arrays (no C++ void* or ADFTypes::void* templates).
        check(add(M::Pointer, 0x900, 15, 8, {}).type_name() == "voidPtr");
        check(add(M::Array, 0x901, 16, 16, {}, 0x900).name() == "voidPtr_Array");
        check(add(M::Array, 0x902, 16, 16, {}, 0x900).name() == "voidPtr_00000902_Array");
        check(add(M::InlineArray, 0x903, 17, 16, {}, 0x900, 2).name() == "voidPtr_InlineArray_2");
        check(add(M::InlineArray, 0x904, 17, 16, {}, 0x900, 2).name() == "voidPtr_00000904_InlineArray_2");
        add(M::Structure, 0x905, 18, 40, std::vector<ADF::StructMemberInfo>{
            member(19, 0x900, 8, 0), member(20, 0x902, 16, 8), member(21, 0x904, 16, 24)});
        // Normalization must happen before duplicate-name reservation.
        check(add(M::Structure, 0x906, 22, 4, small).name() == "voidPtr_00000906");
        check(add(M::Array, 0x907, 23, 16, {}, 0x906).name() == "voidPtr_00000907_Array");

        std::vector<ADF::StructMemberInfo> angle_members;
        for (uint32 i = 0; i < 8; ++i) {
            angle_members.push_back(member(25 + i, STI_TYPE_HASH_FLOAT32, 4, i * 4));
        }
        add(M::Structure, 0xA00, 24, 32, angle_members, 0, 0, 32);

        std::set<std::string> names;
        for (const auto &[hash, type] : lib.types()) {
            if (type.type != STI::DataType::Pointer) check(names.insert(type.name()).second);
        }
        const auto root = std::filesystem::path(argv[1]);
        const auto headers = root / "apex/adf/generated";
        STI::generate_code(lib, root, headers);
        const auto header = read_file(headers / "adf_types.h");
        check(header.contains("Item_00000101 second;"));
        check(header.contains("Vector<Item_00000101> items;"));
        check(header.contains("Array<Item_00000101, 2> fixed;"));
        check(header.contains("Kind_00000501 kind;"));
        check(header.contains("extern ADF::TypeInfo voidPtr_Array_TI;"));
        check(header.contains("Vector<voidPtr> pointers;"));
        check(header.contains("Array<voidPtr, 2> fixedPointers;"));
        check(!header.contains("void*"));
        const auto implementation = read_file(root / "adf_types.cpp");
        for (uint32 i = 0; i < 8; ++i) {
            const auto name = "_" + std::string(adf.get_string(25 + i));
            check(header.contains("float32 " + name + ";"));
            check(implementation.contains(name + " = buffer.read_pod<float32>();"));
        }
        check(implementation.contains("fixedPointers[i] = buffer.read_pod<voidPtr>();"));
        check(implementation.contains("std::make_unique<Vector<voidPtr>>()"));
        check(!implementation.contains("void*"));
        check(implementation.contains("adf_type_info.emplace(0x00000101, &ADFTypes::Item_00000101_TI)"));
        check(implementation.contains(".hash = 0x00000A00,\n    .alignment = 32,\n    .name = \"PlantAngles\""));
        STI::generate_code(lib, root, headers);
        check(read_file(headers / "adf_types.h") == header);
        check(read_file(root / "adf_types.cpp") == implementation);
        std::cout << "ADF typegen regression passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
