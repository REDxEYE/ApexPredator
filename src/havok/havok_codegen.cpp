// Created by RED on 12.10.2025.

#include "havok/havok_codegen.h"

#include <ranges>

#include "platform/logger.h"
#include "utils/hash_helper.h"

using namespace Havok::CodeGen;

struct Context {
    const TypeLibrary &lib;
    std::ofstream &fwd_header_stream;
    std::ofstream &header_stream;
    std::ofstream &impl_stream;
    std::ofstream &formatting_impl_stream;

    bool fwd_mode;
};

bool is_trivial_type(const SharedType &type) {
    if (!type->template_args.empty() && type->type!=MetaType::ENUM) {
        return false;
    }
    bool is_trivial = true;
    if (type->parent() != nullptr) {
        is_trivial &= is_trivial_type(type->parent());
    }

    if (type->type == MetaType::BASIC ||
        type->type == MetaType::PRIMITIVE ||
        type->type == MetaType::ENUM ||
        type->type == MetaType::STRING
    ) {
        return is_trivial;
    }
    if (type->type == MetaType::POINTER) {
        return true;
    }

    if (type->type == MetaType::RECORD) {
        if (std::holds_alternative<std::vector<Member> >(type->data)) {
            const auto &members = std::get<std::vector<Member> >(type->data);
            for (const auto &member: members) {
                is_trivial &= is_trivial_type(member.type());
            }
        }
        return is_trivial;
    }

    return is_trivial;
}

bool is_basic_type(const SharedType &type) {
    return type->type == MetaType::BASIC ||
           (type->type == MetaType::PRIMITIVE && type->parent() != nullptr && is_basic_type(type->parent()));
}

static std::set<SharedType> g_processed_hashes{};
// static std::set<SharedType> g_handled_by_fwd{};

void emit_type(Context &ctx, const SharedType &type, std::ofstream &header_stream);

// void emit_type_forward_declaration(Context &ctx, const SharedType &type, std::ofstream &header_stream) {
//     if (g_processed_hashes.contains(type)) {
//         return;
//     }
//
//     if (is_trivial_type(type)) {
//         // Emit full declaration if it's simple type
//         emit_type(lib, type, header_stream);
//         g_handled_by_fwd.emplace(type);
//         return;
//     }
//
//     g_processed_hashes.emplace(type);
//
//     if (type->type == MetaType::RECORD) {
//         if (type->template_args.size() == 0) {
//             header_stream << std::format("    struct {}; // size: {}\n\n", type->name(), type->size);
//         }
//     }
//     else if (type->type == MetaType::PRIMITIVE) {
//         if (type->parent() != nullptr) {
//             emit_type_forward_declaration(lib, type->parent(), header_stream);
//             if (type->parent()->template_args.empty()) {
//                 header_stream << std::format("    typedef {} {}; // size: {}\n\n", type->parent()->name(),
//                                              type->name(),
//                                              type->size);
//             }
//             else {
//                 header_stream << std::format("    typedef {}<", type->parent()->name());
//                 for (size_t i = 0; i < type->parent()->template_args.size(); i++) {
//                     const auto &arg = type->parent()->template_args[i];
//                     if (std::holds_alternative<int64>(arg.value)) {
//                         header_stream << std::get<int64>(arg.value);
//                     }
//                     else if (std::holds_alternative<WeakType>(arg.value)) {
//                         const auto &inner_type = unwrap_weak(std::get<WeakType>(arg.value));
//                         emit_type_forward_declaration(lib, inner_type, header_stream);
//                         header_stream << inner_type->name();
//                     }
//                     if (i != type->parent()->template_args.size() - 1) {
//                         header_stream << ", ";
//                     }
//                 }
//                 header_stream << std::format("> {}; // size: {}\n\n", type->name(),
//                                              type->size);
//             }
//         }
//     }
//     else if (type->type == MetaType::ENUM) {
//         if (type->template_args.size() == 2) {
//             const auto enum_templ = type->template_args[0];
//             const auto underlying_templ = type->template_args[1];
//             if (std::holds_alternative<WeakType>(enum_templ.value) && std::holds_alternative<WeakType>(
//                     underlying_templ.value)) {
//                 const auto &enum_type = unwrap_weak(std::get<WeakType>(enum_templ.value));
//                 const auto &underlying_type = unwrap_weak(std::get<WeakType>(underlying_templ.value));
//                 emit_type_forward_declaration(lib, underlying_type, header_stream);
//                 header_stream << std::format("    enum class {} : {}{{}};\n\n", enum_type->name(),
//                                              underlying_type->name());
//             }
//         }
//     }
// }

void emit_fwd_type_decl(Context &ctx, const SharedType &type, std::ofstream &stream) {
    switch (type->type) {
        case MetaType::RECORD: {
            if (type->template_args.size() == 0) {
                stream << std::format("struct {}; // size: {}\n\n", type->name(), type->size);
            }
            else {
                stream << std::format("/*\nstruct {}; // size: {}\n*/\n\n", type->name(), type->size);
            }
            break;
        }
        case MetaType::PRIMITIVE:
        case MetaType::OPAQUE:
        case MetaType::STRING:
        case MetaType::BASIC:
        case MetaType::POINTER:
        case MetaType::FIXED_ARRAY:
        case MetaType::ARRAY:
        case MetaType::ENUM:
        case MetaType::SPECIAL:
        case MetaType::TYPE_COUNT: {
            throw std::runtime_error(std::format("emit_fwd_type_decl is not implemented for type {} of meta type {}",
                                                 type->name(), type->type));
            break;
        }
    }
}

bool should_skip_type_info(const MetaType type) {
    return type == MetaType::BASIC || type == MetaType::OPAQUE;
}


std::set<std::string> g_generated_templates;

void emit_struct(Context &ctx, const SharedType &type, std::ofstream &header_stream) {
    if (std::holds_alternative<std::vector<Member> >(type->data)) {
        const auto &members = std::get<std::vector<Member> >(type->data);

        for (const auto &member: members) {
            emit_type(ctx, member.type(), header_stream);
        }

        if (type->parent() != nullptr) {
            emit_type(ctx, type->parent(), header_stream);
        }

        if (!type->template_args.empty()) {
            if (g_generated_templates.contains(type->name())) {
                return;
            }
            g_generated_templates.insert(type->name());
            header_stream << "/*\ntemplate<";
            for (size_t i = 0; i < type->template_args.size(); i++) {
                const auto &arg = type->template_args[i];
                if (std::holds_alternative<WeakType>(arg.value)) {
                    header_stream << "typename " << arg.name;
                }
                else if (std::holds_alternative<int64>(arg.value)) {
                    header_stream << "int64 " << arg.name;
                }

                if (i != type->template_args.size() - 1) {
                    header_stream << ", ";
                }
            }
            header_stream << ">\n";
        }

        if (type->parent() == nullptr) {
            header_stream << std::format("struct {}: Havok::BaseType {{\n", type->type_name());
        }
        else {
            header_stream << std::format("struct {}: {} {{\n", type->name(), type->parent()->type_name());
        }
        for (const auto &member: members) {
            header_stream << std::format("    {} {}; // offset: {}, size: {}\n",
                                         member.type()->type_name(), member.name, member.offset,
                                         member.type()->size);
        }
        header_stream << "\n";
        header_stream << "    void read(IO::File& buffer, Havok::Tag::TagFile& tag_file) override;\n";
        header_stream << "    void print(std::ostream &os) const override;\n";
        header_stream << "    void to_json(std::ostream &os) const override;\n";

        header_stream << "};\n\n";
        if (type->template_args.size() != 0) {
            header_stream << "*/\n";
        }
    }
    else {
        if (type->template_args.size() == 0) {
            header_stream << std::format("struct {} {{}};\n\n", type->name());
        }
        else {
            header_stream << std::format("/*\nstruct {} {{}};\n*/\n\n", type->name());
        }
    }
}

void emit_array(Context &ctx, const SharedType &type, std::ofstream &ofstream) {
    if (std::holds_alternative<std::vector<Member> >(type->data)) {
        const auto &members = std::get<std::vector<Member> >(type->data);
        for (const auto &member: members) {
            emit_type(ctx, member.type(), ofstream);
        }
    }
}

void emit_primitive(Context &ctx, const SharedType &type, std::ofstream &ofstream) {
    if (type->parent() != nullptr) {
        emit_type(ctx, type->parent(), ofstream);
    }

    if (type->parent() != nullptr) {
        emit_type(ctx, type->parent(), ofstream);
        if (type->parent()->template_args.empty()) {
            ofstream << std::format("typedef {} {}; // size: {}\n\n", type->parent()->name(),
                                    type->name(),
                                    type->size);
        }
        else {
            ofstream << std::format("typedef {}<", type->parent()->name());
            for (size_t i = 0; i < type->parent()->template_args.size(); i++) {
                const auto &arg = type->parent()->template_args[i];
                if (std::holds_alternative<int64>(arg.value)) {
                    ofstream << std::get<int64>(arg.value);
                }
                else if (std::holds_alternative<WeakType>(arg.value)) {
                    const auto &inner_type = unwrap_weak(std::get<WeakType>(arg.value));
                    emit_type(ctx, inner_type, ofstream);
                    ofstream << inner_type->name();
                }
                if (i != type->parent()->template_args.size() - 1) {
                    ofstream << ", ";
                }
            }
            ofstream << std::format("> {}; // size: {}\n\n", type->name(),
                                    type->size);
        }
    }
}

void emit_pointer(Context &ctx, const SharedType &type, std::ofstream &ofstream) {
    if (!type->template_args.empty()) {
        const auto &inner_arg = type->template_args[0];
        if (!std::holds_alternative<WeakType>(inner_arg.value)) {
            throw std::runtime_error(std::format("Pointer type {} has non-type template argument", type->name()));
        }
        const auto &inner_type = unwrap_weak(std::get<WeakType>(inner_arg.value));
        if (is_trivial_type(inner_type) && ctx.fwd_mode) {
            emit_type(ctx, inner_type, ctx.fwd_header_stream);
        }
        else if (is_trivial_type(inner_type) && !ctx.fwd_mode) {
            emit_type(ctx, inner_type, ctx.header_stream);
        }
        else if (!is_trivial_type(inner_type) && ctx.fwd_mode) {
            ctx.fwd_mode = false;
            emit_fwd_type_decl(ctx, inner_type, ctx.fwd_header_stream);
            emit_type(ctx, inner_type, ctx.header_stream);
            ctx.fwd_mode = true;
        }
        else {
            emit_type(ctx, inner_type, ctx.header_stream);
        }
    }
}

void emit_basic(Context &ctx, const SharedType &type, std::ofstream &ofstream) {
    if (!type->template_args.empty()) {
        for (const auto & template_arg : type->template_args) {
            if (std::holds_alternative<WeakType>(template_arg.value)) {
                const auto &inner_type = unwrap_weak(std::get<WeakType>(template_arg.value));
                emit_type(ctx, inner_type, ofstream);
            }
        }
    }
    ofstream << std::format("// Basic type {}, size: {}\n\n", type->type_name(), type->size);
}

void emit_fixed_array(Context &ctx, const SharedType &type, std::ofstream &ofstream) {
    if (type->template_args.empty()) {
        throw std::runtime_error(std::format("Fixed array type {} has no template arguments", type->name()));
    }
    const auto &inner_arg = type->template_args[0];
    if (!std::holds_alternative<WeakType>(inner_arg.value)) {
        throw std::runtime_error(std::format("Fixed array type {} has non-type inner template argument", type->name()));
    }
    const auto &inner_type = unwrap_weak(std::get<WeakType>(inner_arg.value));
    emit_type(ctx, inner_type, ofstream);
}

void emit_enum(Context &ctx, const SharedType &type, std::ofstream &ofstream) {
    if (type->template_args.size() == 2) {
        const auto enum_templ = type->template_args[0];
        const auto underlying_templ = type->template_args[1];
        if (std::holds_alternative<WeakType>(enum_templ.value) &&
            std::holds_alternative<WeakType>(underlying_templ.value)
        ) {
            const auto &enum_type = unwrap_weak(std::get<WeakType>(enum_templ.value));
            g_processed_hashes.emplace(enum_type);
            const auto &underlying_type = unwrap_weak(std::get<WeakType>(underlying_templ.value));
            emit_type(ctx, underlying_type, ofstream);
            ofstream << std::format("enum class {} : {}{{}};\n\n", enum_type->name(),
                                    underlying_type->name());
        }
    }
}

void emit_type(Context &ctx, const SharedType &type, std::ofstream &header_stream) {
    if (g_processed_hashes.contains(type)) {
        return;
    }
    g_processed_hashes.emplace(type);

    auto& stream = is_trivial_type(type)? ctx.fwd_header_stream : ctx.header_stream;

    switch (type->type) {
        case MetaType::RECORD: {
            emit_struct(ctx, type, stream);
            break;
        }
        case MetaType::ARRAY: {
            emit_array(ctx, type, stream);
            break;
        }
        case MetaType::PRIMITIVE: {
            emit_primitive(ctx, type, stream);
            break;
        }
        case MetaType::POINTER: {
            emit_pointer(ctx, type, stream);
            break;
        }
        case MetaType::OPAQUE:
        case MetaType::SPECIAL:
        case MetaType::BASIC: {
            emit_basic(ctx, type, stream);
            break;
        }
        case MetaType::FIXED_ARRAY: {
            emit_fixed_array(ctx, type, stream);
            break;
        }
        case MetaType::STRING: {
            break;
        }
        case MetaType::ENUM: {
            emit_enum(ctx, type, stream);
            break;
        }
        case MetaType::TYPE_COUNT: {
            GLog_Warning("Unhandled {}", type->name());
            break;
        }
    }
}

void emit_types(Context &ctx) {
    ctx.fwd_header_stream << "namespace HavokTypes {\n\n";
    ctx.header_stream << "namespace HavokTypes {\n\n";

    g_processed_hashes.clear();
    for (const auto &type: ctx.lib.types() | std::views::values) {
        if (is_trivial_type(type)) {
            ctx.fwd_mode = true;
            emit_type(ctx, type, ctx.fwd_header_stream);
        }
        else {
            ctx.fwd_mode = false;
            emit_type(ctx, type, ctx.header_stream);
        }
    }
    ctx.fwd_header_stream << "};\n";
    ctx.header_stream << "};\n";
}

void emit_struct_to_json_function(const SharedType &type, std::ofstream &impl_stream) {
    impl_stream << std::format("void {}::to_json(std::ostream &out) const {{\n", type->name());
    impl_stream << "    throw std::runtime_error(\"Not implemented\");\n";
    impl_stream << "}\n\n";
}

void emit_struct_read_function_members(const SharedType &type,
                                       std::ofstream &impl_stream, int64 &offset) {
    const auto &type_members = std::get<std::vector<Member> >(type->data);
    if (type->parent()!=nullptr) {
        if (std::holds_alternative<std::vector<Member> >(type->parent()->data)) {
            const auto &parent_members = std::get<std::vector<Member> >(type->parent()->data);
            if (!parent_members.empty()) {
                emit_struct_read_function_members(type->parent(), impl_stream, offset);
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
        if (member.offset != offset) {
            if (member.offset < offset) {
                throw std::runtime_error(std::format(
                    "Member {} of struct {} has offset {}, which is less than current offset {}",
                    member.name, type->name(), member.offset, offset));
            }
            uint32 pad_size = member.offset - offset;
            impl_stream << std::format("    buffer.skip({});\n", pad_size);
            offset += pad_size;
        }

        if (is_basic_type(member.type())) {
            impl_stream << std::format("    {} = buffer.read_pod<{}>();\n", member.name, member.type()->name());
        }
        else if (member.type()->type == MetaType::FIXED_ARRAY) {
            auto inner_type = unwrap_weak(std::get<WeakType>(member.type()->template_args[0].value));
            auto count = std::get<int64>(member.type()->template_args[1].value);
            impl_stream << std::format("    for (size_t i = 0; i < {}; ++i) {{\n", count);
            if (is_basic_type(inner_type)) {
                impl_stream << std::format("        {}[i] = buffer.read_pod<{}>();\n", member.name, inner_type->name());
            }
            else {
                impl_stream << std::format("        {}[i].read(buffer, tag_file);\n", member.name);
            }
            impl_stream << "    }\n";
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
                               type->name());

    int64 offset = 0;
    emit_struct_read_function_members(type, impl_stream, offset);
    if (offset != type->size) {
        if (offset > type->size) {
            throw std::runtime_error(std::format(
                "Struct {} is larger than expected, expected size {}, actual size {}",
                type->name(), type->size, offset));
        }
        uint32 pad_size = type->size - offset;
        impl_stream << std::format("    buffer.skip({});\n", pad_size);
    }
    impl_stream << "}\n\n";
}

void emit_struct_print_function(Context &ctx, const SharedType &type,
                                const std::vector<Member> &members,
                                std::ofstream &impl_stream) {
    impl_stream << std::format("void {}::print(std::ostream &os) const {{\n", type->name());
    if (type->parent() != nullptr) {
        impl_stream << std::format("    {}::print(os);\n", type->parent()->name());
    }
    impl_stream << "    throw std::runtime_error(\"Not implemented\");\n";
    impl_stream << "}\n\n";
}

void emit_enum_formatter(const SharedType &type, std::ofstream &fwd_decl_stream, std::ofstream &impl_stream) {
    const auto &enum_type = type->template_args[0];
    if (!std::holds_alternative<WeakType>(enum_type.value)) {
        return;
    }
    const auto &enum_shared_type = unwrap_weak(std::get<WeakType>(enum_type.value));
    // Havok enums don't have members, so just get underlying int type and print it
    fwd_decl_stream << std::format("std::ostream& operator<<(std::ostream &os, const HavokTypes::{} &value);\n",
                                   enum_shared_type->name());

    impl_stream << std::format("std::ostream& operator<<(std::ostream &os, const HavokTypes::{} &value) {{\n",
                               enum_shared_type->name());
    impl_stream << std::format("    return os << std::to_underlying(value);\n");
    impl_stream << "}\n\n";
}

// void emit_new_instance_function(const SharedType &type, std::ofstream &impl_stream) {
//     std::string full_type_name = type->full_name();
//     if (type->type == MetaType::POINTER) {
//         full_type_name += "_Ptr";
//         impl_stream << std::format("static std::unique_ptr<{}> {}_new_instance() {{\n", type->type_name(),
//                                    full_type_name);
//         impl_stream << std::format("    return std::make_unique<{}>();\n", type->type_name());
//         impl_stream << "}\n\n";
//     }
//     else {
//         impl_stream << std::format("static std::unique_ptr<{}> {}_new_instance() {{\n", type->type_name(),
//                                    full_type_name);
//         impl_stream << std::format("    return std::make_unique<{}>();\n", type->type_name());
//         impl_stream << "}\n\n";
//     }
// }

void emit_functions(Context &ctx) {
    for (const auto &type: ctx.lib.types() | std::views::values) {
        if (type->type == MetaType::RECORD && type->template_args.size() == 0) {
            if (std::holds_alternative<std::vector<Member> >(type->data)) {
                const auto &members = std::get<std::vector<Member> >(type->data);

                // Generate read, print, and to_json functions
                emit_struct_read_function(ctx, type, members, ctx.impl_stream);
                emit_struct_print_function(ctx, type, members, ctx.impl_stream);
                emit_struct_to_json_function(type, ctx.impl_stream);
            }
        }
        else if (type->type == MetaType::ENUM) {
            emit_enum_formatter(type, ctx.fwd_header_stream, ctx.formatting_impl_stream);
        }
        // if (type->hash != 0) {
        //     emit_new_instance_function(type, ctx.impl_stream);
        // }
    }
}


void emit_type_infos(const Context &ctx) {
    for (const auto &type: ctx.lib.types() | std::views::values) {
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

    for (const auto &type: ctx.lib.types() | std::views::values) {
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

    std::ofstream header_stream(header_output);
    std::ofstream fwd_decl_stream(fwd_decl_output);
    std::ofstream impl_stream(impl_output);
    std::ofstream formatter_stream(formatters_output);

    header_stream << "// This file is autogenerated\n";
    header_stream << "#pragma once\n";
    header_stream << "#include <array>\n";
    header_stream << "#include \"havok/havok_support_types.h\"\n";
    header_stream << "#include \"havok/extra_support_types.h\"\n\n";
    header_stream << "#include \"havok/generated/havok_types_fwd.h\"\n\n";
    header_stream << "#include \"havok/havok_base_type.h\"\n";

    impl_stream << "// This file is autogenerated\n";
    impl_stream << "#include \"havok/generated/havok_types.h\"\n\n";
    impl_stream << "#include <stdexcept>\n\n";
    impl_stream << "#include \"havok/havok_support_types.h\"\n";
    impl_stream << "#include \"havok/tag_file/havok_tag_file.h\"\n";
    impl_stream << "#include \"platform/buffer/buffer.h\"\n\n";
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

    Context ctx(lib, fwd_decl_stream, header_stream, impl_stream, formatter_stream, false);

    emit_types(ctx);
    emit_functions(ctx);
    emit_type_infos(ctx);
    emit_type_info_table(ctx);
}
