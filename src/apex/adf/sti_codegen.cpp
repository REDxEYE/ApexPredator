// Created by RED on 07.10.2025.
#include <vector>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <set>

#include "apex/adf/sti.h"
#include "redscore/platform/logger.h"

using namespace STI;

static std::set<uint32> g_processed_hashes{};

// Forward declarations
void emit_type(const TypeLibrary &lib, const Type &type, std::ofstream &header_stream);

// Helper functions
static const Type &get_type_or_throw(const TypeLibrary &lib, uint32 type_hash) {
    const auto type_opt = lib.get_type(type_hash);
    if (!type_opt) {
        GLog_Error("Failed to find type with hash 0x{:08X}", type_hash);
        throw std::runtime_error("Failed to find type");
    }
    return type_opt->get();
}

static bool should_skip_type_info(DataType type) {
    return type == DataType::Bitfield ||
           type == DataType::Primitive ||
           type == DataType::Pointer ||
           type == DataType::DeferredType ||
           type == DataType::StringType;
}

static bool is_simple_type(DataType type) {
    return type == DataType::Primitive ||
           type == DataType::Bitfield ||
           type == DataType::StringType ||
           type == DataType::StringHash ||
           type == DataType::DeferredType;
}

void emit_type_forward_declaration(const Type &type, std::ofstream &header_stream) {
    if (type.type == DataType::Structure) {
        header_stream << std::format("struct {}; // size: {}\n", type.type_name(), type.size);
    } else if (type.type == DataType::Enumeration) {
        header_stream << std::format("enum class {}: uint{};\n", type.type_name(), type.size * 8);
    }
}

void emit_struct(const TypeLibrary &lib, const Type &type, std::ofstream &header_stream) {
    const auto &members = std::get<std::vector<StructMember> >(type.data);

    // Emit dependencies first
    for (const auto &member: members) {
        const Type &member_type = get_type_or_throw(lib, member.type_hash);
        emit_type(lib, member_type, header_stream);
    }

    header_stream << "extern ADF::TypeInfo " << type.type_name() << "_TI;\n";
    header_stream << std::format("struct {}: ADF::BaseType {{ // size: {}, alignment: {}\n", type.type_name(),type.size, type.alignment);

    // Emit member declarations
    for (const auto &member: members) {
        const Type &member_type = get_type_or_throw(lib, member.type_hash);

        switch (member_type.type) {
            case DataType::Primitive:
            case DataType::Bitfield:
            case DataType::StringType:
            case DataType::StringHash:
            case DataType::Enumeration:
            case DataType::Structure:
                header_stream << std::format("    {} {}; // offset: {}, size: {}\n",
                                             member_type.type_name(), member.name, member.offset, member.size);
                break;

            case DataType::Pointer:
                header_stream << std::format("    u64 {}; // offset: {}, size: {}\n", member.name, member.offset, member.size);
                break;

            case DataType::Array: {
                const auto &type_info = std::get<TypeAndSize>(member_type.data);
                const Type &inner_type = get_type_or_throw(lib, type_info.type_hash);
                header_stream << std::format("    Vector<{}> {}; // offset: {}, size: {}\n",
                                             inner_type.type_name(), member.name, member.offset, member.size);
                break;
            }

            case DataType::InlineArray: {
                const auto &type_info = std::get<TypeAndSize>(member_type.data);
                const Type &inner_type = get_type_or_throw(lib, type_info.type_hash);
                header_stream << std::format("    Array<{}, {}> {}; // offset: {}, size: {}\n",
                                             inner_type.type_name(), type_info.count, member.name, member.offset,
                                             member.size);
                break;
            }

            case DataType::DeferredType:
                header_stream << std::format("    std::unique_ptr<ADF::BaseType> {}; // offset: {}, size: {}\n",
                                             member.name, member.offset, member.size);
                break;

            case DataType::Recursive:
                GLog_Error("Type {} is not supported yet", member_type.type_name());
                break;
        }
    }

    header_stream << "    void read(IO::File& buffer) override;\n";
    header_stream << "    void print(std::ostream &out) const override;\n";
    header_stream << "    nlohmann::json to_json() const override;\n";

    header_stream << "}; // size: " << type.size << "\n\n";
}

void emit_array_type(const TypeLibrary &lib, const Type &type, std::ofstream &header_stream,
                     const std::string &array_kind) {
    const auto &type_info = std::get<TypeAndSize>(type.data);
    const Type &inner_type = get_type_or_throw(lib, type_info.type_hash);
    emit_type(lib, inner_type, header_stream);

    header_stream << "// " << array_kind << " of " << inner_type.type_name() << "\n";
    header_stream << "extern ADF::TypeInfo " << type.name() << "_TI;\n\n";
}

void emit_array(const TypeLibrary &lib, const Type &type, std::ofstream &header_stream) {
    emit_array_type(lib, type, header_stream, "Array");
}

void emit_inline_array(const TypeLibrary &lib, const Type &type, std::ofstream &header_stream) {
    emit_array_type(lib, type, header_stream, "Inline array");
}

void emit_enum(const Type &type, std::ofstream &header_stream) {
    const auto &members = std::get<std::vector<EnumMember> >(type.data);
    header_stream << std::format("enum class {}: uint{} {{\n", type.type_name(), type.size * 8);
    for (const auto &member: members) {
        header_stream << std::format("    {} = {},\n", member.name, member.value);
    }
    header_stream << "};\n\n";
    header_stream << "extern ADF::TypeInfo " << type.type_name() << "_TI;\n\n";
}

void emit_string_hash(const Type &type, std::ofstream &header_stream) {
    header_stream << "extern ADF::TypeInfo " << type.name() << "_TI;\n\n";
}

void emit_type(const TypeLibrary &lib, const Type &type, std::ofstream &header_stream) {
    if (g_processed_hashes.contains(type.hash)) {
        return;
    }
    g_processed_hashes.emplace(type.hash);

    switch (type.type) {
        case DataType::Structure:
            emit_struct(lib, type, header_stream);
            break;
        case DataType::Primitive:
        case DataType::StringType:
        case DataType::Pointer:
        case DataType::Bitfield:
        case DataType::DeferredType:
            // Do nothing
            break;
        case DataType::InlineArray:
            emit_inline_array(lib, type, header_stream);
            break;
        case DataType::Array:
            emit_array(lib, type, header_stream);
            break;
        case DataType::Enumeration:
            emit_enum(type, header_stream);
            break;
        case DataType::StringHash:
            emit_string_hash(type, header_stream);
            break;
        default:
            throw std::runtime_error("Unsupported type");
    }
}

void emit_types(const TypeLibrary &lib, std::ofstream &fwd_decl_stream, std::ofstream &header_stream) {
    fwd_decl_stream << "namespace ADFTypes {\n\n";
    for (const auto &type: lib.types() | std::views::values) {
        emit_type_forward_declaration(type, fwd_decl_stream);
    }
    fwd_decl_stream << "};\n";

    header_stream << "namespace ADFTypes {\n\n";
    header_stream << "\n";

    for (const auto &type: lib.types() | std::views::values) {
        emit_type(lib, type, header_stream);
    }

    header_stream << "enum class ADFHashes:uint32 {\n";
    for (const auto &type: lib.types() | std::views::values) {
        if (type.type == DataType::Array ||
            type.type == DataType::InlineArray ||
            type.type == DataType::Primitive ||
            type.type == DataType::Bitfield ||
            type.type == DataType::Pointer) {
            continue;
        }
        header_stream << std::format("    {} = 0x{:08X},\n", type.name(), type.hash);
    }
    header_stream << "};\n";
    header_stream << "};\n\n";
}

void emit_type_infos(const std::unordered_map<uint32, Type> &types, std::ostream &stream) {
    for (const auto &type: types | std::views::values) {
        if (should_skip_type_info(type.type)) {
            continue;
        }

        stream << std::format("ADF::TypeInfo ADFTypes::{}_TI = {{\n", type.name());
        if (type.type == DataType::Structure || type.type == DataType::Array) {
            stream << std::format("    .new_instance = {}_new_instance,\n", type.name());
        } else {
            stream << "    .new_instance = nullptr,\n";
        }
        stream << std::format("    .hash = 0x{:08X},\n", type.hash);
        stream << std::format("    .name = \"{}\"\n", type.name());
        stream << "};\n\n";
    }
}

void emit_type_info_table(const std::unordered_map<uint32, Type> &types,
                          std::ofstream &header_stream,
                          std::ofstream &impl_stream) {
    header_stream << "extern ADF::TypeInfoMap adf_type_info;\n\n";
    header_stream << "void init_adf_type_info();\n\n";

    impl_stream << "ADF::TypeInfoMap adf_type_info;\n\n";
    impl_stream << "void init_adf_type_info() {\n";
    impl_stream << std::format("    adf_type_info.reserve({});\n", types.size());

    for (const auto &type: types | std::views::values) {
        if (should_skip_type_info(type.type)) {
            continue;
        }
        impl_stream << std::format("    adf_type_info.emplace(0x{:08X}, &ADFTypes::{}_TI);\n", type.hash, type.name());
    }

    impl_stream << "}\n";
}

// Function generation helpers
void emit_array_print_helper(const TypeLibrary &lib, const Type &member_type,
                             const std::string &member_name, std::ofstream &impl_stream) {
    impl_stream << std::format("    out << \"{}: \\n\";\n", member_name);
    impl_stream << std::format("    {}.print(out);\n", member_name);
    impl_stream << "    out << \"\\n\";\n";
}

void emit_struct_print_function(const TypeLibrary &lib, const Type &type,
                                const std::vector<StructMember> &members,
                                const std::string &type_name,
                                std::ofstream &impl_stream) {
    impl_stream << std::format("void {}::print(std::ostream &out) const {{\n", type_name);

    for (const auto &member: members) {
        const Type &member_type = get_type_or_throw(lib, member.type_hash);

        switch (member_type.type) {
            case DataType::Primitive:
            case DataType::Bitfield:
            case DataType::StringType:
                impl_stream << std::format("    out << \"{}: \" << {} << \"\\n\";\n", member.name, member.name);
                break;

            case DataType::Structure:
                impl_stream << std::format("    {}.print(out);\n", member.name);
                break;

            case DataType::Pointer:
                impl_stream << std::format("    out << \"{}: \" << {} << \"\\n\";\n", member.name, member.name);
                // impl_stream << "    throw std::runtime_error(\"Pointer types are not supported yet\");\n";
                break;

            case DataType::Array:
            case DataType::InlineArray:
                emit_array_print_helper(lib, member_type, member.name, impl_stream);
                break;

            case DataType::Enumeration:
                impl_stream << std::format("    out << \"{}: \" << {} << \"\\n\";\n", member.name, member.name);
                break;

            case DataType::StringHash:
                impl_stream << std::format("    out << \"{}: \";\n", member.name);
                impl_stream << std::format("    {}.print(out);\n", member.name);
                impl_stream << "    out << \"\\n\";\n";
                break;

            case DataType::DeferredType:
                impl_stream << std::format("    out << \"{}: \" << \"\\n\";\n", member.name);
                impl_stream << std::format("    if ({}) {{\n", member.name);
                impl_stream << std::format("        {}->print(out);\n", member.name);
                impl_stream << "    }\n";
                break;

            case DataType::Recursive:
                throw std::runtime_error("Recursive types are not supported yet");
        }
    }

    impl_stream << "}\n\n";
}

void emit_struct_to_json_function(const TypeLibrary &lib, const Type &type,
                                  const std::vector<StructMember> &members,
                                  const std::string &type_name,
                                  std::ofstream &impl_stream) {
    impl_stream << std::format("nlohmann::json {}::to_json() const {{\n", type_name);
    impl_stream << "    nlohmann::json _res;\n";
    for (const auto &member: members) {
        const Type &member_type = get_type_or_throw(lib, member.type_hash);
        switch (member_type.type) {
            case DataType::Primitive:
            case DataType::Bitfield:
            case DataType::StringType:
                impl_stream << std::format("    _res[\"{}\"] = {};\n", member.name, member.name);
                break;

            case DataType::Structure:
            case DataType::Array:
            case DataType::StringHash:
            case DataType::InlineArray:
                impl_stream << std::format("    _res[\"{}\"] = {}.to_json();\n", member.name, member.name);
                break;

            case DataType::Pointer:
                impl_stream << std::format("    _res[\"{}\"] = {};\n", member.name, member.name);
                // impl_stream << std::format("    _res[\"{}\"] = {}.to_json();\n", member.name, member.name);
                // impl_stream << "    throw std::runtime_error(\"Pointer types are not supported yet\");\n";
                break;

            case DataType::Enumeration:
                impl_stream << std::format("    _res[\"{}\"] = {};\n", member.name, member.name);
                // impl_stream << std::format("    out << \"{}: \" << {} << \"\\n\";\n",
                //                            member.name, member.name);
                break;

            case DataType::DeferredType:
                impl_stream << std::format("    if ({}) {{\n", member.name);
                impl_stream << std::format("        _res[\"{}\"] = {}->to_json();\n", member.name, member.name);
                impl_stream << "    }\n";
                break;

            case DataType::Recursive:
                throw std::runtime_error("Recursive types are not supported yet");
        }
    }
    // impl_stream << "    throw std::runtime_error(\"Not implemented\");\n";
    impl_stream << "    return _res;\n";
    impl_stream << "}\n\n";
}

void emit_enum_formatter(const Type &type, const std::string &type_name, std::ofstream &fwd_decl_stream,
                         std::ofstream &impl_stream) {
    const auto &members = std::get<std::vector<EnumMember> >(type.data);

    fwd_decl_stream << std::format("constexpr std::string_view to_string({} v) noexcept;\n", type_name);
    fwd_decl_stream << std::format("std::ostream& operator<<(std::ostream &os, {} value);\n", type_name);

    impl_stream << std::format("constexpr std::string_view to_string({} v) noexcept{{\n", type_name);
    impl_stream << "    using namespace std::literals;\n";
    impl_stream << "    switch (v) {\n";
    for (const auto &member: members) {
        impl_stream << std::format("        case {}::{}: return \"{}\"sv;\n", type_name, member.name, member.name);
    }
    impl_stream << "        default: return \"Unknown\";\n";
    impl_stream << "    }\n";
    impl_stream << "}\n\n";

    impl_stream << std::format("std::ostream& operator<<(std::ostream &os, const {} value) {{\n", type_name);
    impl_stream << "    return os << to_string(value);\n";
    impl_stream << "}\n\n";


    // fwd_decl_stream << "template<>\n";
    // fwd_decl_stream << std::format("struct std::formatter<{}>: std::formatter<std::string_view> {{\n", type_name);
    // fwd_decl_stream << std::format("    auto format(const ADFTypes::{} value, std::format_context &ctx) {{\n",
    //                            type.type_name());
    // fwd_decl_stream << "        std::string_view str_value = to_string(value);\n";
    // fwd_decl_stream << "        return std::formatter<std::string_view>::format(str_value, ctx);\n";
    // fwd_decl_stream << "    }\n";
    // fwd_decl_stream << "};\n\n";
}

void emit_new_instance_function(const std::string &type_name, const std::string &name, std::ofstream &impl_stream) {
    impl_stream << std::format("static std::unique_ptr<{}> {}_new_instance() {{\n", type_name, name);
    impl_stream << std::format("    return std::make_unique<{}>();\n", type_name);
    impl_stream << "}\n\n";
}

std::string get_qualified_type_name(const Type &type, const Type &inner_type) {
    if (is_simple_type(inner_type.type)) {
        return inner_type.type_name();
    }
    return "ADFTypes::" + inner_type.type_name();
}

void emit_struct_read_function(const TypeLibrary &lib, const Type &type,
                               const std::vector<StructMember> &members,
                               const std::string &type_name,
                               std::ofstream &impl_stream) {
    impl_stream << std::format("void {}::read(IO::File& buffer) {{\n", type_name);
    uint32 offset = 0;

    for (uint32 i = 0; i < members.size(); ++i) {
        const auto &member = members[i];
        const Type &member_type = get_type_or_throw(lib, member.type_hash);

        // Handle padding
        if (member.offset != offset) {
            if (offset > member.offset) {
                throw std::runtime_error(std::format(
                    "Overlapping members in struct {}, member {} has offset {}, expected {}",
                    type.type_name(), member.name, member.offset, offset));
            }
            uint32 pad_size = member.offset - offset;
            impl_stream << std::format("    buffer.skip({});\n", pad_size);
            offset += pad_size;
        }

        // Read member based on type
        switch (member_type.type) {
            case DataType::Primitive:
                impl_stream << std::format("    {} = buffer.read_pod<{}>();\n",
                                           member.name, member_type.type_name());
                break;

            case DataType::Array:
            case DataType::Structure:
            case DataType::StringHash:
                impl_stream << std::format("    {}.read(buffer);\n", member.name);
                break;

            case DataType::Pointer:
                impl_stream << std::format("    {} = buffer.read_pod<u64>();\n",member.name);
                // impl_stream << "    throw std::runtime_error(\"Pointer types are not supported yet\");\n";
                break;

            case DataType::InlineArray: {
                const Type &inner_type = get_type_or_throw(lib, std::get<TypeAndSize>(member_type.data).type_hash);
                impl_stream << std::format("    for(int i = 0; i < {}; ++i) {{\n",
                                           std::get<TypeAndSize>(member_type.data).count);
                if (inner_type.type == DataType::Structure || inner_type.type == DataType::Array || inner_type.type == DataType::StringHash) {
                    impl_stream << std::format("        {}[i].read(buffer);\n", member.name);
                } else if (inner_type.type == DataType::Primitive) {
                    impl_stream << std::format("        {}[i] = buffer.read_pod<{}>();\n",
                                               member.name, inner_type.type_name());
                } else if (inner_type.type == DataType::StringType) {
                    impl_stream << std::format("        {}[i] = buffer.read_cstring();\n", member.name);
                } else {
                    impl_stream << "        throw std::runtime_error(\"Unsupported inline array inner type\");\n";
                }
                impl_stream << "    }\n";
                break;
            }

            case DataType::StringType:
                impl_stream << "    {\n";
                impl_stream << "        uint32 string_offset = buffer.read_pod<uint32>();\n";
                impl_stream << "        uint32 unk = buffer.read_pod<uint32>();\n";
                impl_stream << "        std::streamoff original_offset = buffer.get_position();\n";
                impl_stream << "        buffer.set_position(string_offset, std::ios::beg);\n";
                impl_stream << std::format("        {} = buffer.read_cstring();\n", member.name);
                impl_stream << "        buffer.set_position(original_offset, std::ios::beg);\n";
                impl_stream << "    }\n";
                break;

            case DataType::DeferredType:
                impl_stream << std::format("    {} = std::move(Deferred::read(buffer));\n", member.name);
                break;

            case DataType::Bitfield: {
                const Type &total_member = member_type;
                uint32 j = i;
                for (; j < members.size(); ++j) {
                    const auto &bitfield_member = members[j];
                    const Type &bitfield_member_type = get_type_or_throw(lib, bitfield_member.type_hash);
                    if (bitfield_member_type.type != DataType::Bitfield ||
                        bitfield_member_type.size != total_member.size) {
                        break;
                    }
                }

                impl_stream << "    {\n";
                impl_stream << std::format("        const uint{} bitfield_value = buffer.read_pod<uint{}>();\n",
                                           total_member.size * 8, total_member.size * 8);
                uint32 bit_offset = 0;
                for (uint32 k = i; k < j; ++k) {
                    const auto &bitfield_member = members[k];
                    const Type &bitfield_member_type = get_type_or_throw(lib, bitfield_member.type_hash);
                    if (bitfield_member_type.type != DataType::Bitfield) {
                        throw std::runtime_error("Expected bitfield type");
                    }
                    uint32 bit_width = std::get<uint32>(bitfield_member_type.data);
                    impl_stream << std::format("        {} = (bitfield_value >> {}) & ((1ULL << {}) - 1);\n",
                                               bitfield_member.name, bit_offset, bit_width);
                    bit_offset += bit_width;
                }
                impl_stream << "    }\n";
                i = j - 1;
                break;
            }

            case DataType::Enumeration: {
                const char *uint_types[] = {"uint8", "uint16", "uint32", "uint64"};
                uint32 size_index = 0;
                if (member_type.size == 2) size_index = 1;
                else if (member_type.size == 4) size_index = 2;
                else if (member_type.size == 8) size_index = 3;
                else if (member_type.size != 1) throw std::runtime_error("Unsupported enum size");

                impl_stream << std::format("    {} = static_cast<ADFTypes::{}>(buffer.read_pod<{}>());\n",
                                           member.name, member_type.type_name(), uint_types[size_index]);
                break;
            }

            case DataType::Recursive:
                break;
        }
        offset += member_type.size;
    }

    // Handle final padding
    if (offset != type.size) {
        if (offset > type.size) {
            throw std::runtime_error(std::format(
                "Struct {} is larger than expected, expected size {}, actual size {}",
                type.type_name(), type.size, offset));
        }
        uint32 pad_size = type.size - offset;
        impl_stream << std::format("    buffer.skip({});\n", pad_size);
    }

    impl_stream << "}\n\n";
}

void emit_functions(const TypeLibrary &lib, std::ofstream &fwd_decl_stream, std::ofstream &impl_stream,
                    std::ofstream &formatter_out) {
    for (const auto &type: lib.types() | std::views::values) {
        // Skip types that don't need function generation
        if (type.type == DataType::Bitfield ||
            type.type == DataType::Pointer ||
            type.type == DataType::Primitive) {
            continue;
        }

        std::string type_name;

        switch (type.type) {
            case DataType::Structure: {
                type_name = "ADFTypes::" + type.type_name();
                const auto &members = std::get<std::vector<StructMember> >(type.data);

                // Generate read, print, and to_json functions
                emit_struct_read_function(lib, type, members, type_name, impl_stream);
                emit_struct_print_function(lib, type, members, type_name, impl_stream);
                emit_struct_to_json_function(lib, type, members, type_name, impl_stream);
                break;
            }

            case DataType::Array: {
                const auto &type_and_size = std::get<TypeAndSize>(type.data);
                const Type &inner_type = get_type_or_throw(lib, type_and_size.type_hash);
                type_name = std::format("Vector<{}>", get_qualified_type_name(type, inner_type));
                break;
            }

            // case DataType::InlineArray: {
            //     const auto &type_and_size = std::get<TypeAndSize>(type.data);
            //     const Type &inner_type = get_type_or_throw(lib, type_and_size.type_hash);
            //     type_name = std::format("std::array<{}, {}>",
            //                            get_qualified_type_name(type, inner_type),
            //                            type_and_size.count);
            //     // InlineArray doesn't need new_instance
            //     continue;
            // }

            case DataType::Enumeration: {
                type_name = "ADFTypes::" + type.type_name();
                emit_enum_formatter(type, type_name, fwd_decl_stream, formatter_out);
                // Enumeration doesn't need new_instance
                continue;
            }

            default:
                continue;
        }

        // Generate new_instance function for types that need it
        emit_new_instance_function(type_name, type.name(), impl_stream);
    }
}

void STI::generate_code(const TypeLibrary &lib,
                        const std::filesystem::path &sources_path,
                        const std::filesystem::path &headers_path) {
    std::filesystem::create_directories(sources_path);
    std::filesystem::create_directories(headers_path);

    auto header_output = headers_path / "adf_types.h";
    auto fwd_decl_output = headers_path / "adf_types_fwd.h";
    auto impl_output = sources_path / "adf_types.cpp";
    auto formatters_output = sources_path / "adf_types_formatters.cpp";

    std::ofstream header_stream(header_output);
    std::ofstream fwd_decl_stream(fwd_decl_output);
    std::ofstream impl_stream(impl_output);
    std::ofstream formatter_stream(formatters_output);

    header_stream << "// This file is autogenerated\n";
    header_stream << "#pragma once\n";
    header_stream << "#include <array>\n";
    header_stream << "#include \"apex/adf/adf_base_type.h\"\n";
    header_stream << "#include \"apex/adf/adf_support_types.h\"\n\n";
    header_stream << "#include \"json.hpp\"\n";
    header_stream << "#include \"apex/adf/generated/adf_types_fwd.h\"\n\n";

    impl_stream << "// This file is autogenerated\n";
    impl_stream << "#include \"apex/adf/generated/adf_types.h\"\n\n";
    impl_stream << "#include <stdexcept>\n\n";
    impl_stream << "#include \"apex/adf/adf_support_types.h\"\n";
    impl_stream << "using namespace ADF;\n\n";

    formatter_stream << "// This file is autogenerated\n";
    formatter_stream << "#include <iostream>\n";
    formatter_stream << "#include <format>\n";
    formatter_stream << "#include <string>\n";
    formatter_stream << "#include <string_view>\n\n";
    formatter_stream << "#include \"apex/adf/generated/adf_types.h\"\n\n";

    fwd_decl_stream << "// This file is autogenerated\n";
    fwd_decl_stream << "#include <iostream>\n";
    fwd_decl_stream << "#include <format>\n";
    fwd_decl_stream << "#include <string_view>\n\n";
    fwd_decl_stream << "#include \"apex/adf/generated/adf_types.h\"\n\n";

    emit_types(lib, fwd_decl_stream, header_stream);
    emit_functions(lib, fwd_decl_stream, impl_stream, formatter_stream);
    emit_type_infos(lib.types(), impl_stream);
    emit_type_info_table(lib.types(), header_stream, impl_stream);
}
