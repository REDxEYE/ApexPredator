// Created by RED on 12.10.2025.

#ifndef APEXPREDATOR_HAVOK_TAG_TYPES_H
#define APEXPREDATOR_HAVOK_TAG_TYPES_H
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <variant>

#include "int_def.h"
#include "utils/file/file.h"

namespace Havok::Tag {
    enum class TypeFlags {
        Format = 0x01,
        SubType = 0x02,
        Version = 0x04,
        SizeAlign = 0x08,
        Flags = 0x10,
        Fields = 0x20,
        Interfaces = 0x40,
        Attribute = 0x80
    };

    bool operator&(TypeFlags lhs, TypeFlags rhs);

    struct Type;

    using SharedType = std::shared_ptr<Type>;
    using WeakType = std::weak_ptr<Type>;


    struct TypeMember {
        std::string name;
        uint64 flags;
        uint64 offset;

        SharedType type() const;
        void type(const SharedType &type);

        TypeMember(const std::string &name_, const uint64 flags, const uint64 offset,
                   const WeakType &type) : flags(flags), offset(offset), type_(type) {
            if (name_[0] >= '0' && name_[0] <= '9')
                name = "_" + name_;
            else
                name = name_;
        }

    private:
        WeakType type_;
    };

    using ValueOrType = std::variant<
        int64,
        WeakType
    >;

    struct TemplateArgument {
        std::string name;
        ValueOrType value;
    };

    struct Interface {
        uint64 type_id;
        uint64 offset;
    };

    enum class DataType:uint32 {
        PRIMITIVE = 0,
        OPAQUE = 1,
        BOOL = 2,
        STRING = 3,
        BASIC = 4,
        FLOAT = 5,
        POINTER = 6,
        RECORD = 7,
        ARRAY = 8,
    };

    static std::array<std::string_view, 9> DataType_Names = {
        "PRIMITIVE", "OPAQUE", "BOOL", "STRING", "BASIC", "FLOAT", "POINTER", "RECORD", "ARRAY"
    };


    struct Type {
        std::string name;
        std::vector<TemplateArgument> template_args{};
        std::vector<TypeMember> members{};
        std::vector<Interface> interfaces{};
        uint32 format{};
        uint32 sub_type{};
        uint32 version{};
        SharedType parent{};

        uint32 flags{};
        uint32 hash{};
        DataType data_type{};

        Type(std::unique_ptr<IO::File> &buffer, const std::vector<std::string> &names);

        Type();

        [[nodiscard]] std::string unique_id() const;

        [[nodiscard]] uint32 size() const;

        [[nodiscard]] uint32 align() const;

        void size(uint32 s);

        void align(uint32 a);

    private:
        uint32 size_{};
        uint32 align_{};
    };
}
#endif //APEXPREDATOR_HAVOK_TAG_TYPES_H
