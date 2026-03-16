// Created by RED on 20.01.2026.

#ifndef APEXPREDATOR_HAVOK_TYPES_H
#define APEXPREDATOR_HAVOK_TYPES_H
#include <array>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <memory>
#include <format>
#include <utility>

#include "int_def.h"
#include "tag_file/havok_tag_file.h"

namespace Havok::CodeGen {
    class Type;

    using SharedType = std::shared_ptr<Type>;
    using WeakType = std::weak_ptr<Type>;

    SharedType unwrap_weak(const WeakType &weak);

    struct Member {
        std::string name;
        uint64 flags;
        uint64 offset;

        [[nodiscard]] SharedType type() const;

        Member(std::string name, const uint64 flags, const uint64 offset, const SharedType &type)
            : name(std::move(name)),
              flags(flags),
              offset(offset),
              type_(type) {
        }

    private:
        WeakType type_;
    };

    enum class MetaType:uint32 {
        PRIMITIVE,
        OPAQUE,
        STRING,
        BASIC,
        POINTER,
        RECORD,
        FIXED_ARRAY,
        ARRAY,
        ENUM,
        SPECIAL,
        TYPE_COUNT,
    };


    static std::array<std::string_view, std::to_underlying(MetaType::TYPE_COUNT)> HavokTypeMetaTypeNames = {
        "Primitive",
        "OPAQUE",
        "String",
        "Basic"
        "Pointer",
        "Record",
        "FixedArray",
        "Array",
        "Enum",
        "Special",
        "TYPE_COUNT",
    };

    inline std::ostream &operator<<(std::ostream &os, const MetaType &value) {
        os << HavokTypeMetaTypeNames[std::to_underlying(value)];
        return os;
    }

    struct TypeAndLen {
        int64 length;
        WeakType type;
    };

    using ValueOfType = std::variant<
        int64,
        WeakType
    >;

    struct TemplateArgument {
        std::string name;
        ValueOfType value;
    };

    using TypeData = std::variant<
        std::vector<Member>, // members
        std::monostate
    >;

    class Type {
    public:
        MetaType type;
        uint32 hash{0};
        uint32 size{0};
        uint32 align{0};
        WeakType parent_{};
        std::vector<TemplateArgument> template_args{};
        TypeData data{};

        [[nodiscard]] std::string name() const;

        [[nodiscard]] std::string type_name() const;

        [[nodiscard]] std::string full_name() const;

        [[nodiscard]] std::vector<std::string> name_parts() const;

        [[nodiscard]] int64 size_without_padding() const;

        [[nodiscard]] SharedType parent() const;


        Type(const std::string &name, MetaType type, uint32 hash,
             uint32 size, uint32 align, const SharedType &parent);

    private:
        std::string m_name;
    };

    class TypeLibrary {
    public:
        TypeLibrary() = default;

        void register_types(const Tag::TagFile &tag_file);

        SharedType register_type(const Tag::TagFile &tag_file, const Tag::SharedType &tag_type);

        [[nodiscard]] const std::unordered_map<uint32, SharedType> &types() const;

        bool is_type(std::string_view name) const;

    private:
        std::unordered_map<uint32, SharedType> m_types;
        std::set<uint32> m_exported_hashes;
    };
}

template<>
struct std::formatter<Havok::CodeGen::MetaType> : std::formatter<std::string_view> {
    auto format(const Havok::CodeGen::MetaType &value, std::format_context &ctx) const {
        return std::formatter<std::string_view>::format(
            Havok::CodeGen::HavokTypeMetaTypeNames[std::to_underlying(value)], ctx);
    }
};
#endif //APEXPREDATOR_HAVOK_TYPES_H
