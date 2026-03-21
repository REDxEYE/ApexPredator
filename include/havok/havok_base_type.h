// Created by RED on 01.03.2026.
#pragma once

#include <format>
#include <functional>
#include <unordered_map>
#include <memory>

#include "platform/file/file.h"


namespace Havok {
    namespace CodeGen {
        enum class MetaType:uint32;
    }

    namespace Tag {
        class TagFile;
    }

    struct BaseType {
        virtual ~BaseType() = default;

        virtual void read(IO::File &buffer, Tag::TagFile &tag_file) {
            throw std::runtime_error(std::format("read is not implemented for type {}", typeid(*this).name()));
        }

        virtual void print(std::ostream &out) const {
            throw std::runtime_error(std::format("read is not implemented for type {}", typeid(*this).name()));
        }

        virtual void to_json(std::ostream &out) const {
            throw std::runtime_error(std::format("read is not implemented for type {}", typeid(*this).name()));
        }
    };

    using NewFn = std::function<std::unique_ptr<BaseType>()>;

    struct TypeInfo final {
        NewFn new_instance;
        uint32 hash = 0;
        CodeGen::MetaType type;
        std::string_view name;
    };

    using TypeInfoMap = std::unordered_map<uint32, TypeInfo *>;
}
