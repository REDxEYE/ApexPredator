// Created by RED on 20.01.2026.

#include "havok/havok_types.h"

#include "havok/tag_file/havok_tag_file.h"
#include "utils/hash_helper.h"

#include <ranges>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace Havok::CodeGen {
    static inline SharedType lock_or_null(const WeakType &w) noexcept {
        return w.expired() ? SharedType{} : w.lock();
    }

    SharedType unwrap_weak(const WeakType &weak) {
        return lock_or_null(weak);
    }

    [[nodiscard]] SharedType Member::type() const {
        return lock_or_null(type_);
    }
} // namespace Havok::CodeGen

static Havok::CodeGen::MetaType convert_meta_type(const Havok::Tag::DataType value) {
    using DT = Havok::Tag::DataType;
    using MT = Havok::CodeGen::MetaType;

    switch (value) {
        case DT::PRIMITIVE: return MT::PRIMITIVE;
        case DT::OPAQUE: return MT::OPAQUE;
        case DT::BOOL: return MT::BASIC;
        case DT::STRING: return MT::STRING;
        case DT::BASIC: return MT::BASIC;
        case DT::FLOAT: return MT::BASIC;
        case DT::POINTER: return MT::POINTER;
        case DT::RECORD: return MT::RECORD;
        case DT::ARRAY: return MT::ARRAY;
    }

    throw std::runtime_error(std::format(
        "Unknown Havok data type: {}",
        Havok::Tag::DataType_Names[std::to_underlying(value)]));
}

namespace Havok::CodeGen {
    static inline const WeakType &require_type_arg(std::string_view owner, const TemplateArgument &arg) {
        if (!std::holds_alternative<WeakType>(arg.value)) {
            throw std::runtime_error(std::format("{} has non-type template argument", owner));
        }
        return std::get<WeakType>(arg.value);
    }

    static inline int64 require_i64_arg(std::string_view owner, const TemplateArgument &arg) {
        if (!std::holds_alternative<int64>(arg.value)) {
            throw std::runtime_error(std::format("{} has non-int template argument", owner));
        }
        return std::get<int64>(arg.value);
    }

    std::string Type::name() const {
        if (type == MetaType::PRIMITIVE || type == MetaType::BASIC) {
            return m_name;
        }
        // if (type==MetaType::POINTER) {
        //     if (template_args.size() == 1) {
        //         const auto &inner = lock_or_null(require_type_arg(m_name, template_args[0]));
        //         if (!inner) return std::format("{}_Ptr", m_name);
        //         if (inner->name() == "void") return std::format("{}_Ptr", m_name);
        //         return std::format("{}_Ptr", inner->name());
        //     }
        //      return std::format("{}_Ptr", m_name);
        // }

        if (type == MetaType::ARRAY) {
            if (template_args.size() == 2) {
                const auto &inner = lock_or_null(require_type_arg(m_name, template_args[0]));
                if (!inner) return std::format("{}_Array", m_name);
                return std::format("{}_Array", inner->name());
            }
            return m_name;
        }

        if (type == MetaType::FIXED_ARRAY) {
            if (template_args.size() != 2) {
                throw std::runtime_error(std::format("Fixed Array type {} has invalid number of template arguments",
                                                     m_name));
            }
            const auto &inner = lock_or_null(require_type_arg(m_name, template_args[0]));
            const int64 len = require_i64_arg(m_name, template_args[1]);
            if (!inner) return std::format("{}_{}_FixedArray", m_name, len);
            return std::format("{}_{}_FixedArray", inner->name(), len);
        }

        return m_name;
    }

    std::string Type::type_name() const {
        if (type == MetaType::POINTER) {
            if (template_args.size() == 1) {
                const auto &inner = lock_or_null(require_type_arg(m_name, template_args[0]));
                if (!inner) return "hkPtr<BaseType>";
                if (inner->type_name() == "void") return "hkPtr<BaseType>";
                return std::format("hkPtr<{}>", inner->type_name());
            }
            return std::format("hkPtr<{}>", name());
        }

        if (type == MetaType::FIXED_ARRAY) {
            if (template_args.size() == 2) {
                const auto &inner = lock_or_null(require_type_arg(m_name, template_args[0]));
                const int64 len = require_i64_arg(m_name, template_args[1]);
                const auto inner_name = inner ? inner->type_name() : std::string{"void"};
                return std::format("std::array<{}, {}>", inner_name, len);
            }
        }

        if (type == MetaType::ARRAY) {
            if (template_args.size() == 2) {
                const auto &inner = lock_or_null(require_type_arg(m_name, template_args[0]));
                const auto &alloc = lock_or_null(require_type_arg(m_name, template_args[1]));
                const auto inner_name = inner ? inner->type_name() : std::string{"void"};
                const auto alloc_name = alloc ? alloc->type_name() : std::string{"void"};
                return std::format("hkArray<{}, {}>", inner_name, alloc_name);
            }
            return m_name;
        }

        if (type == MetaType::STRING) {
            return "hkString";
        }

        if (!template_args.empty()) {
            std::string out;
            out.reserve(m_name.size() + 2 + template_args.size() * 8);
            out.append(m_name).append("<");
            for (size_t i = 0; i < template_args.size(); ++i) {
                const auto &arg = template_args[i];
                std::visit([&](const auto &v) {
                    using V = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<V, int64>) {
                        out += std::to_string(v);
                    }
                    else if constexpr (std::is_same_v<V, WeakType>) {
                        const auto t = lock_or_null(v);
                        out += t ? t->type_name() : "void";
                    }
                }, arg.value);

                if (i + 1 != template_args.size()) out.append(", ");
            }
            out.append(">");
            return out;
        }

        return m_name;
    }

    std::string Type::full_name() const {
        std::string out = type_name();
        size_t index;
        while ((index = out.find(",")) != std::string::npos) {
            out.replace(index, 1, "_");
        }
        while ((index = out.find("<")) != std::string::npos) {
            out.replace(index, 1, "_");
        }
        while ((index = out.find(">")) != std::string::npos) {
            out.replace(index, 1, "_");
        }
        // erase spaces
        std::erase_if(out, ::isspace);


        return out;
    }

    std::vector<std::string> Type::name_parts() const {
        std::vector<std::string> parts;
        size_t start = 0;
        const auto &n = name();
        for (size_t i = 0; i < n.size(); ++i) {
            if (n[i] == ':') {
                if (i + 1 < n.size() && n[i + 1] == ':') {
                    parts.emplace_back(n.data() + start, i - start);
                    start = i + 2;
                    ++i;
                }
            }
        }
        parts.emplace_back(n.data() + start, n.size() - start);
        return parts;
    }

    int64 Type::size_without_padding() const {
        switch (type) {
            case MetaType::RECORD: {
                const auto &members = std::get<std::vector<Member> >(data);
                if (members.empty()) return size;
                const auto &last = members.back();
                const auto last_t = last.type();
                return static_cast<int64>(last.offset) + (last_t ? last_t->size : 0);
            }

            case MetaType::BASIC:
            case MetaType::ENUM:
            case MetaType::POINTER:
            case MetaType::STRING:
            case MetaType::OPAQUE:
            case MetaType::ARRAY:
            case MetaType::SPECIAL:
                return size;

            case MetaType::PRIMITIVE: {
                const auto p = parent();
                return p ? p->size_without_padding() : size;
            }

            case MetaType::FIXED_ARRAY: {
                if (template_args.size() != 2) {
                    throw std::runtime_error(std::format(
                        "Fixed Array type {} has invalid number of template arguments", m_name));
                }
                const auto &inner = lock_or_null(require_type_arg(m_name, template_args[0]));
                const int64 len = require_i64_arg(m_name, template_args[1]);
                if (!inner) return 0;
                return inner->size_without_padding() * len;
            }

            default:
                throw std::runtime_error(std::format(
                    "size_without_padding is only valid for record/array-ish types, but {} is of type {}",
                    name(), HavokTypeMetaTypeNames[std::to_underlying(type)]));
        }
    }

    SharedType Type::parent() const {
        return lock_or_null(parent_);
    }

    Type::Type(const std::string &name, const MetaType type_, const uint32 hash_, const uint32 size_,
               const uint32 align_, const SharedType &parent)
        : type(type_)
          , hash(hash_)
          , size(size_)
          , align(align_)
          , parent_(parent)
          , m_name(name) {
        if (name == "T[N]") {
            type = MetaType::FIXED_ARRAY;
        }

        auto index = 0;
        while (true) {
            index = m_name.find("::", index);
            if (index == std::string::npos) break;
            m_name.replace(index, 2, "_");
            index += 1;
        }

    }

    void TypeLibrary::register_types(const Tag::TagFile &tag_file) {
        for (auto &t: tag_file.types()) {
            (void) register_type(tag_file, t);
        }
    }

    std::shared_ptr<Type> TypeLibrary::register_type(const Tag::TagFile &tag_file, const Tag::SharedType &tag_type) {
        (void) tag_file;

        const auto unique_id = tag_type->unique_id();
        const uint32 key = hash_string(unique_id);

        if (auto it = m_types.find(key); it != m_types.end()) {
            if (it->second->hash == 0) {
                it->second->hash = tag_type->hash;
            }
            return it->second;
        }

        const SharedType parent_type = tag_type->parent ? register_type(tag_file, tag_type->parent) : nullptr;

        auto new_type = std::make_shared<Type>(
            tag_type->name,
            convert_meta_type(tag_type->data_type),
            tag_type->hash,
            tag_type->size(),
            tag_type->align(),
            parent_type
        );

        m_types.emplace(key, new_type);

        // Members -> make it a RECORD unless it's a pointer wrapper.
        if (!tag_type->members.empty() && tag_type->data_type != Tag::DataType::POINTER) {
            std::vector<Member> members;
            members.reserve(tag_type->members.size());
            for (auto &m: tag_type->members) {
                members.emplace_back(m.name, m.flags, m.offset, register_type(tag_file, m.type()));
            }
            new_type->data = std::move(members);
            new_type->type = MetaType::RECORD;
        }

        // Template args
        if (!tag_type->template_args.empty()) {
            new_type->template_args.reserve(tag_type->template_args.size());
            for (const auto &[t_name, v_value]: tag_type->template_args) {
                if (const auto *pi = std::get_if<int64>(&v_value)) {
                    new_type->template_args.push_back({t_name, *pi});
                    continue;
                }
                if (const auto *pw = std::get_if<Tag::WeakType>(&v_value)) {
                    auto inner_tag = pw->lock();
                    auto inner_type = inner_tag ? register_type(tag_file, inner_tag) : SharedType{};
                    new_type->template_args.push_back({t_name, inner_type});
                    continue;
                }
            }
        }

        // Normalization rules
        if (new_type->type_name().starts_with("hkArray<")) {
            new_type->type = MetaType::ARRAY;
        }
        // if (new_type->name().starts_with("hkRefVariant")) {
        //     new_type->type = MetaType::BASIC;
        // }
        if (new_type->type_name().starts_with("hkFreeListArrayElement")) {
            new_type->type = MetaType::BASIC;
        }
        if (new_type->name() == "hkaiIndex") {
            new_type->type = MetaType::SPECIAL;
        }
        if (new_type->name() == "hkRefVariant") {
            new_type->type = MetaType::SPECIAL;
        }
        if (new_type->name() == "hkaiPackedKey_") {
            new_type->type = MetaType::SPECIAL;
        }
        if (new_type->name() == "hkBool" || new_type->name() == "hkBaseObject") {
            new_type->type = MetaType::BASIC;
        }
        if (new_type->name() == "hkString") {
            new_type->type = MetaType::STRING;
        }
        if (new_type->type == MetaType::PRIMITIVE) {
            const auto tn = new_type->type_name();
            if (tn.starts_with("hkFlags<") || tn.starts_with("hkEnum<")) {
                new_type->type = MetaType::ENUM;
            }
        }

        return new_type;
    }

    const std::unordered_map<uint32, SharedType> &TypeLibrary::types() const {
        return m_types;
    }

    bool TypeLibrary::is_type(const std::string_view name) const{
        for (const auto &type: m_types | std::views::values) {
            auto name_parts = type->name_parts();
            if (name_parts.at(name_parts.size() - 1) == name) {
                return true;
            }
        }
        return false;
    }
} // namespace Havok::CodeGen
