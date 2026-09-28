// Created by RED on 12.10.2025.
#include "havok/havok_codegen.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <map>
#include <ranges>
#include <set>
#include <unordered_set>

using namespace Havok::CodeGen;

// These definitions live in the support headers, not in the tag-file schema.
static bool supplied_type(const SharedType &type) {
    static const std::set<std::string> names{
        "hkBool", "hkBaseObject", "hkVector4f", "hkRotationImpl", "hkMatrix3Impl",
        "hkReflect_Type", "hkReflect_Detail_Opaque", "hkReflect_QualifiedType",
        "hkString", "hkFixedArray", "hkArray", "hkRelArray", "hkFreeListArray",
        "hkEnum", "hkFlags", "hkPtrAndInt", "hkHashMap", "hkRefPtr", "hkRefVariant",
        "hkPtr", "hkHandle", "hkaiIndex", "hkaiPackedKey_", "hkFreeListArrayElement",
        "hkcdStaticTree_Tree", "hkcdDynamicTree_Tree", "hkcdStaticTree",
        "hkcdStaticTree_DynamicStorage", "hkcdDynamicTree_DefaultDynamicStorage",
        "hknpSparseCompactMap", "hkcdStaticMeshTreeBase_PrimitiveDataRunBase",
        "hkBitFieldBase", "hkBitFieldStorage", "hkcdStaticMeshTreeCommonConfig"
    };
    return names.contains(type->name());
}

static bool native_type(const SharedType &type) {
    static const std::set<std::string> names{
        "void", "bool", "char", "signed char", "unsigned char", "short", "unsigned short",
        "int", "unsigned int", "long", "unsigned long", "long long", "unsigned long long",
        "float", "double", "int8", "uint8", "int16", "uint16", "int32", "uint32", "int64", "uint64",
        "float32", "float64"
    };
    return names.contains(type->name());
}

bool is_basic_type(const SharedType &type) {
    if (!type) throw std::runtime_error("Missing Havok member type");
    if (supplied_type(type)) return type->name() == "hkBool";
    if (!type->scalar_type.empty()) return true;
    if (native_type(type)) return type->name() != "void";
    return type->type == MetaType::PRIMITIVE && type->parent() && is_basic_type(type->parent());
}

struct Context {
    const TypeLibrary &lib;
    std::ofstream &fwd_header_stream;
    std::ofstream &header_stream;
    std::ofstream &impl_stream;
    std::ofstream &formatting_impl_stream;
    std::vector<SharedType> ordered;
    std::map<const Type *, SharedType> enum_storage;
    std::map<const Type *, std::vector<SharedType>> dependencies;
};

static SharedType type_arg(const SharedType &type, size_t index) {
    if (index >= type->template_args.size() || !std::holds_alternative<WeakType>(type->template_args[index].value))
        throw std::runtime_error("Missing type argument in " + type->name());
    auto result = unwrap_weak(std::get<WeakType>(type->template_args[index].value));
    if (!result) throw std::runtime_error("Expired type argument in " + type->name());
    return result;
}

static bool indirect_type(const SharedType &type) {
    return type->type == MetaType::POINTER || type->type == MetaType::ARRAY ||
           type->name() == "hkRefPtr" || type->name() == "hkRelArray";
}

static void plan_types(Context &ctx) {
    std::vector<SharedType> roots;
    for (const auto &[hash, type] : ctx.lib.types()) roots.push_back(type);
    std::ranges::sort(roots, [](const auto &a, const auto &b) {
        return std::pair(a->type_name(), a->hash) < std::pair(b->type_name(), b->hash);
    });
    // Enum wrappers refer to an enum identity that may otherwise appear as BASIC/OPAQUE.
    // Its storage belongs to the wrapper; emit the identity once, not once per wrapper.
    for (const auto &type : roots) {
        if (type->type == MetaType::ENUM) {
            auto identity = type_arg(type, 0), storage = type_arg(type, 1);
            ctx.enum_storage.try_emplace(identity.get(), storage);
        }
    }
    for (const auto &type : roots) {
        auto &deps = ctx.dependencies[type.get()];
        auto add = [&](const SharedType &dep) {
            if (!dep) throw std::runtime_error("Missing dependency of " + type->type_name());
            if (std::ranges::find(deps, dep) == deps.end()) deps.push_back(dep);
        };
        if (auto it = ctx.enum_storage.find(type.get()); it != ctx.enum_storage.end()) {
            add(it->second);
            continue;
        }
        if (type->parent() && !supplied_type(type)) add(type->parent());
        if (!supplied_type(type) && type->type == MetaType::RECORD) {
            if (const auto *members = std::get_if<std::vector<Member>>(&type->data))
                for (const auto &member : *members) add(member.type());
        }
        for (const auto &arg : type->template_args) {
            if (const auto *weak = std::get_if<WeakType>(&arg.value)) {
                auto inner = unwrap_weak(*weak);
                if (!inner) throw std::runtime_error("Expired template argument in " + type->name());
                // Pointers/vectors need a declared class, not its completed definition.
                // Aliases and enums still must be defined before they can be named.
                if (indirect_type(type) && inner->type == MetaType::RECORD && !supplied_type(inner)) continue;
                add(inner);
            }
        }
    }
    enum class State { Visiting, Done };
    std::map<const Type *, State> state;
    std::vector<SharedType> path;
    std::function<void(const SharedType &)> visit = [&](const SharedType &type) {
        if (const auto it = state.find(type.get()); it != state.end()) {
            if (it->second == State::Done) return;
            std::string message = "Havok by-value/inheritance dependency cycle: ";
            for (const auto &entry : path) message += entry->type_name() + " -> ";
            throw std::runtime_error(message + type->type_name());
        }
        state[type.get()] = State::Visiting;
        path.push_back(type);
        for (const auto &dep : ctx.dependencies.at(type.get())) visit(dep);
        path.pop_back();
        state[type.get()] = State::Done;
        ctx.ordered.push_back(type);
    };
    for (const auto &type : roots) visit(type);
}

static bool generated_record(const SharedType &type) {
    return type->type == MetaType::RECORD && !supplied_type(type);
}

static void emit_template_declaration(const SharedType &type, std::ostream &out) {
    out << "template<";
    for (size_t i = 0; i < type->template_args.size(); ++i) {
        if (i) out << ", ";
        out << (std::holds_alternative<WeakType>(type->template_args[i].value) ? "typename" : "int64");
    }
    out << "> struct " << type->name() << ";\n";
}

static void emit_struct(const SharedType &type, std::ostream &out) {
    if (!type->template_args.empty()) out << "template<>\n";
    out << std::format("struct {} : {} {{ // size: {}, alignment: {}\n", type->type_name(),
        type->parent() ? type->parent()->type_name() : "Havok::BaseType", type->size, type->align);
    if (const auto *members = std::get_if<std::vector<Member>>(&type->data)) {
        for (const auto &member : *members) {
            out << std::format("    {} {}; // offset: {}, size: {}\n", member.type()->type_name(), member.name,
                               member.offset, member.type()->size);
        }
    }
    out << "    void read(IO::File& buffer, Havok::Tag::TagFile& tag_file) override;\n"
           "    void print(std::ostream& os) const override;\n"
           "    nlohmann::json to_json() const override;\n};\n\n";
}

static void emit_definition(Context &ctx, const SharedType &type, std::ostream &out) {
    if (type->name() == "<<INVALID_TYPE>>") return; // Tag index zero is a sentinel.
    if (auto it = ctx.enum_storage.find(type.get()); it != ctx.enum_storage.end()) {
        // Prefer the identity's scalar width/sign when present; wrappers can use different storage.
        const auto underlying = type->scalar_type.empty() ? it->second->type_name() : type->scalar_type;
        out << std::format("enum class {} : {} {{}};\n\n", type->name(), underlying);
    } else if (native_type(type) || supplied_type(type)) {
        out << "// Provided type " << type->type_name() << "\n";
    } else if (generated_record(type) && !ctx.enum_storage.contains(type.get())) {
        emit_struct(type, out);
    } else if (type->type == MetaType::PRIMITIVE && type->parent()) {
        out << std::format("using {} = {};\n\n", type->name(), type->parent()->type_name());
    } else if (!type->scalar_type.empty()) {
        out << std::format("using {} = {};\n\n", type->name(), type->scalar_type);
    } else if (type->type == MetaType::BASIC || type->type == MetaType::OPAQUE || type->type == MetaType::SPECIAL ||
               type->type == MetaType::PRIMITIVE) {
        if (!type->template_args.empty())
            throw std::runtime_error("No Havok support definition for " + type->type_name());
        // Unknown opaque data has no scalar semantics. Preserve its bytes, never guess an integer.
        out << std::format("struct {} : Havok::BaseType {{\n    std::array<uint8, {}> bytes{{}};\n", type->name(), type->size);
        out << "    void read(IO::File& buffer, Havok::Tag::TagFile&) override { buffer.read_exact<uint8>(bytes); }\n"
               "    void print(std::ostream& os) const override { os << to_json(); }\n"
               "    nlohmann::json to_json() const override { return bytes; }\n};\n\n";
    }
}

static void emit_types(Context &ctx) {
    ctx.fwd_header_stream << "namespace HavokTypes {\n\n";
    std::set<std::string> templates;
    for (const auto &type : ctx.ordered) {
        if (!generated_record(type) || ctx.enum_storage.contains(type.get())) continue;
        if (type->template_args.empty()) ctx.fwd_header_stream << "struct " << type->name() << ";\n";
        else if (templates.insert(type->name()).second) emit_template_declaration(type, ctx.fwd_header_stream);
    }
    // extra_support_types.h embeds these generated types. Put their complete dependency
    // closure in the forward header, in the same topological order as the main header.
    std::unordered_set<const Type *> early;
    std::function<void(const SharedType &)> include = [&](const SharedType &type) {
        if (!early.insert(type.get()).second) return;
        for (const auto &dep : ctx.dependencies.at(type.get())) include(dep);
    };
    for (const auto &type : ctx.ordered) {
        if (type->name() == "hkAabb" || type->name() == "hkUint32" || type->name() == "hkStringPtr") include(type);
    }
    ctx.header_stream << "namespace HavokTypes {\n\n";
    for (const auto &type : ctx.ordered)
        emit_definition(ctx, type, early.contains(type.get()) ? ctx.fwd_header_stream : ctx.header_stream);
    ctx.fwd_header_stream << "}\n";
    ctx.header_stream << "}\n";
}

void emit_struct_to_json_function_members(Context &ctx, const SharedType &type, std::ofstream &impl_stream) {
    const auto &type_members = std::get<std::vector<Member> >(type->data);
    if (type->parent()!=nullptr) {
        if (std::holds_alternative<std::vector<Member> >(type->parent()->data)) {
            const auto &parent_members = std::get<std::vector<Member> >(type->parent()->data);
            if (!parent_members.empty()) {
                emit_struct_to_json_function_members(ctx, type->parent(), impl_stream);
            }
        }else {
            throw std::runtime_error(std::format("Parent type {} of struct {} has non-member data",
                                             type->parent()->name(), type->name()));
        }
    }
    for (const auto &member: type_members) {
        if (is_basic_type(member.type()) || ctx.enum_storage.contains(member.type().get())) {
            impl_stream << std::format("    obj_[\"{}\"] = {};\n", member.name, member.name);
        }
        else{
            impl_stream << std::format("    obj_[\"{}\"] = {}.to_json();\n", member.name, member.name);
        }
    }
}

void emit_struct_to_json_function(Context &ctx, const SharedType &type, std::ofstream &impl_stream) {
    impl_stream << std::format("nlohmann::json {}::to_json() const {{\n", type->type_name());
    impl_stream << "    nlohmann::json obj_;\n";
    emit_struct_to_json_function_members(ctx, type, impl_stream);
    impl_stream << "    return obj_;\n";
    impl_stream << "}\n\n";
}

void emit_struct_read_function_members(Context &ctx, const SharedType &type,
                                       std::ofstream &impl_stream, int64 &offset) {
    const auto &type_members = std::get<std::vector<Member> >(type->data);
    if (type->parent()!=nullptr) {
        if (std::holds_alternative<std::vector<Member> >(type->parent()->data)) {
            const auto &parent_members = std::get<std::vector<Member> >(type->parent()->data);
            if (!parent_members.empty()) {
                emit_struct_read_function_members(ctx, type->parent(), impl_stream, offset);
            }
        }else {
            throw std::runtime_error(std::format("Parent type {} of struct {} has non-member data",
                                             type->parent()->name(), type->name()));
        }
    }
    if (type_members.empty() && type->parent()==nullptr) {
        impl_stream << std::format("    buffer.skip({});\n", type->size);
        offset+=type->size;
        return;
    }
    for (const auto &member: type_members) {
        impl_stream << std::format("    buffer.set_position(_obj_start + {}, std::ios::beg);\n", member.offset);
        if (is_basic_type(member.type()) || ctx.enum_storage.contains(member.type().get())) {
            impl_stream << std::format("    {} = buffer.read_pod<{}>();\n", member.name, member.type()->type_name());
        }
        else if (member.type()->type == MetaType::FIXED_ARRAY) {
            impl_stream << std::format("    {}.read(buffer, tag_file);\n", member.name);
            // auto inner_type = unwrap_weak(std::get<WeakType>(member.type()->template_args[0].value));
            // auto count = std::get<int64>(member.type()->template_args[1].value);
            // impl_stream << std::format("    for (size_t i = 0; i < {}; ++i) {{\n", count);
            // if (is_basic_type(inner_type)) {
            //     impl_stream << std::format("        {}[i] = buffer.read_pod<{}>();\n", member.name, inner_type->name());
            // }
            // else {
            //     impl_stream << std::format("        {}[i].read(buffer, tag_file);\n", member.name);
            // }
            // impl_stream << "    }\n";
        }
        else {
            impl_stream << std::format("    {}.read(buffer, tag_file);\n", member.name);
        }
        offset += member.type()->size_without_padding();
    }
}

void emit_struct_read_function(Context &ctx, const SharedType &type,
                               const std::vector<Member> &members,
                               std::ofstream &impl_stream) {
    impl_stream << std::format("void {}::read(IO::File& buffer, Tag::TagFile& tag_file) {{\n",
                               type->type_name());
    impl_stream<< "    const u64 _obj_start = buffer.get_position();\n";

    int64 offset = 0;
    emit_struct_read_function_members(ctx, type, impl_stream, offset);
    impl_stream << std::format("    buffer.set_position(_obj_start+ {}, std::ios::beg);\n", type->size);
    impl_stream << "}\n\n";
}

void emit_struct_print_function(Context &ctx, const SharedType &type,
                                const std::vector<Member> &members,
                                std::ofstream &impl_stream) {
    impl_stream << std::format("void {}::print(std::ostream &os) const {{\n", type->type_name());
    if (type->parent() != nullptr) {
        impl_stream << std::format("    {}::print(os);\n", type->parent()->type_name());
    }
    impl_stream << "    throw std::runtime_error(\"Not implemented\");\n";
    impl_stream << "}\n\n";
}

void emit_functions(Context &ctx) {
    for (const auto &type: ctx.ordered) {
        if (generated_record(type) && !ctx.enum_storage.contains(type.get())) {
            if (std::holds_alternative<std::vector<Member> >(type->data)) {
                const auto &members = std::get<std::vector<Member> >(type->data);

                // Generate read, print, and to_json functions
                emit_struct_read_function(ctx, type, members, ctx.impl_stream);
                emit_struct_print_function(ctx, type, members, ctx.impl_stream);
                emit_struct_to_json_function(ctx, type, ctx.impl_stream);
            }
        }

    }
}


void emit_type_infos(const Context &ctx) {
    for (const auto &type: ctx.ordered) {
        if (type->hash == 0) {
            continue;
        }
        auto full_name = type->full_name();
        if (type->type == MetaType::POINTER) {
            full_name += "_Ptr";
        }
        auto &stream = ctx.impl_stream;
        stream << std::format("TypeInfo TI_{:08X} = {{\n", type->hash);
        if (type->type == MetaType::RECORD || type->type == MetaType::ARRAY) {
            stream << std::format("    .new_instance = new_instance<{}>,\n", type->type_name());
        }
        else {
            stream << "    .new_instance = nullptr,\n";
        }
        stream << std::format("    .hash = 0x{:08X},\n", type->hash);
        stream << std::format("    .type = CodeGen::MetaType({}),\n", std::to_underlying(type->type));
        stream << std::format("    .name = \"{}\",\n", type->type_name());
        stream << "};\n\n";
    }
}

void emit_type_info_table(Context &ctx) {
    ctx.header_stream << "extern Havok::TypeInfoMap havok_type_info;\n\n";
    ctx.header_stream << "void init_havok_type_info();\n\n";

    ctx.impl_stream << "TypeInfoMap havok_type_info;\n\n";
    ctx.impl_stream << "void init_havok_type_info() {\n";
    ctx.impl_stream << std::format("    havok_type_info.reserve({});\n", ctx.lib.types().size());

    for (const auto &type: ctx.ordered) {
        if (type->hash != 0) {
            ctx.impl_stream << std::format("    havok_type_info.emplace(0x{:08X}, &TI_{:08X});\n", type->hash,
                                           type->hash);
        }
    }

    ctx.impl_stream << "}\n";
}


void Havok::CodeGen::generate_code(const TypeLibrary &lib,
                                   const std::filesystem::path &sources_path,
                                   const std::filesystem::path &headers_path) {
    std::filesystem::create_directories(sources_path);
    std::filesystem::create_directories(headers_path);

    auto header_output = headers_path / "havok_types.h";
    auto fwd_decl_output = headers_path / "havok_types_fwd.h";
    auto impl_output = sources_path / "havok_types.cpp";
    auto formatters_output = sources_path / "havok_types_formatters.cpp";

    std::ofstream header_stream, fwd_decl_stream, impl_stream, formatter_stream;
    Context ctx{lib, fwd_decl_stream, header_stream, impl_stream, formatter_stream};
    plan_types(ctx); // Validate cycles before truncating an existing generated file.
    header_stream.open(header_output);
    fwd_decl_stream.open(fwd_decl_output);
    impl_stream.open(impl_output);
    formatter_stream.open(formatters_output);
    if (!header_stream || !fwd_decl_stream || !impl_stream || !formatter_stream)
        throw std::runtime_error("Failed to open Havok code generation outputs");

    header_stream << "// This file is autogenerated\n";
    header_stream << "#pragma once\n";
    header_stream << "#include <array>\n";
    header_stream << "#include \"havok/havok_support_types.h\"\n";
    if (lib.is_type("hkAabb") && lib.is_type("hkStringPtr") && lib.is_type("hkUint32"))
        header_stream << "#include \"havok/extra_support_types.h\"\n\n";
    header_stream << "#include \"havok/generated/havok_types_fwd.h\"\n\n";
    header_stream << "#include \"havok/havok_base_type.h\"\n";
    header_stream << "#include \"nlohmann/json.hpp\"\n";

    impl_stream << "// This file is autogenerated\n";
    impl_stream << "#include \"havok/generated/havok_types.h\"\n\n";
    impl_stream << "#include <stdexcept>\n\n";
    impl_stream << "#include \"havok/havok_support_types.h\"\n";
    impl_stream << "#include \"havok/tag_file/havok_tag_file.h\"\n";
    impl_stream << "using namespace Havok;\n";
    impl_stream << "using namespace HavokTypes;\n\n";

    formatter_stream << "// This file is autogenerated\n";
    formatter_stream << "#include <iostream>\n";
    formatter_stream << "#include <format>\n";
    formatter_stream << "#include <string>\n";
    formatter_stream << "#include <string_view>\n\n";
    formatter_stream << "#include \"havok/generated/havok_types.h\"\n\n";

    fwd_decl_stream << "// This file is autogenerated\n";
    fwd_decl_stream << "#pragma once\n";
    fwd_decl_stream << "#include \"havok/havok_base_type.h\"\n";
    fwd_decl_stream << "#include \"havok/havok_support_types.h\"\n\n";
    fwd_decl_stream << "#include <iostream>\n";
    fwd_decl_stream << "#include <format>\n";
    fwd_decl_stream << "#include <string_view>\n\n";

    emit_types(ctx);
    emit_functions(ctx);
    emit_type_infos(ctx);
    emit_type_info_table(ctx);
}
