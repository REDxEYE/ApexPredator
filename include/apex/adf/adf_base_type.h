// Created by RED on 01.03.2026.
#pragma once

#include <memory>
#include <functional>
#include <unordered_map>

#include "redscore/platform/file/file.h"
#include "nlohmann/json.hpp"

namespace ADF {
    struct BaseType {
        virtual ~BaseType() = default;

        virtual void read(IO::File &buffer) = 0;

        virtual void print(std::ostream &out) const = 0;

        [[nodiscard]] virtual nlohmann::json to_json() const = 0;
    };

    using NewFn = std::function<std::unique_ptr<BaseType>()>;

    struct TypeInfo final {
        NewFn new_instance;
        uint32 hash = 0;
        std::string_view name;
    };

    using TypeInfoMap = std::unordered_map<uint32, TypeInfo*>;
}