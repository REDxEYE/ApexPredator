// Created by RED on 12.10.2025.


#include "havok/tag_file/havok_tag_types.h"

#include "utils/hash_helper.h"

int64 read_compressed_int(const std::unique_ptr<IO::File> &buffer);

inline const std::string &check_id_range(const uint64 id, const std::vector<std::string> &array) {
    if (id > array.size()) {
        throw std::runtime_error("ID out of range");
    }
    return array[id];
}

bool Havok::Tag::operator&(TypeFlags lhs, TypeFlags rhs) {
    return (static_cast<uint32>(lhs) & static_cast<uint32>(rhs)) != 0;
}

Havok::Tag::SharedType Havok::Tag::TypeMember::type() const {
    if (type_.expired()) {
        return nullptr;
    }
    return type_.lock();
}

void Havok::Tag::TypeMember::type(const SharedType &type) {
    type_ = type;
}

Havok::Tag::Type::Type(std::unique_ptr<IO::File> &buffer, const std::vector<std::string> &names) {
    name = check_id_range(read_compressed_int(buffer), names);

    // size_t index = 0;
    // while (true) {
    //     index = name.find("::", index);
    //     if (index != std::string::npos) {
    //         name.replace(index, 2, "_");
    //         continue;
    //     }
    //     break;
    // }

    const auto template_args_count = read_compressed_int(buffer);
    template_args.reserve(template_args_count);
    for (uint32 i = 0; i < template_args_count; i++) {
        const auto template_name = check_id_range(read_compressed_int(buffer), names);
        const auto value = read_compressed_int(buffer);
        template_args.emplace_back(template_name, value);
    }
}

Havok::Tag::Type::Type() {
    name = "<<INVALID_TYPE>>";
}

std::string Havok::Tag::Type::unique_id() const {
    std::string u_name = name;
    if (!template_args.empty()) {
        u_name += "<";
        for (const auto &arg: template_args) {
            u_name += arg.name + ":";
            if (std::holds_alternative<int64>(arg.value)) {
                u_name += std::to_string(std::get<int64>(arg.value));
            }
            else if (std::holds_alternative<WeakType>(arg.value)) {
                u_name += std::get<WeakType>(arg.value).lock()->unique_id();
            }
            if (&arg != &template_args.back()) {
                u_name += ", ";
            }
        }
        u_name += ">";
    }
    return u_name;
}

uint32 Havok::Tag::Type::size() const {
    if (size_ == 0) {
        if (parent != nullptr) {
            return parent->size();
        }
        return 0;
    }
    return size_;
}

uint32 Havok::Tag::Type::align() const {
    if (align_ == 0) {
        if (parent != nullptr) {
            return parent->align();
        }
        return 0;
    }
    return align_;
}

void Havok::Tag::Type::size(const uint32 s) {
    size_ = s;
}

void Havok::Tag::Type::align(const uint32 a) {
    align_ = a;
}
