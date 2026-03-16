// Created by RED on 04.02.2026.

#ifndef APEXPREDATOR_ADF_SUPPORT_TYPES_H
#define APEXPREDATOR_ADF_SUPPORT_TYPES_H
#include <format>
#include <string>
#include <optional>
#include <functional>
#include <unordered_map>

#include "adf_base_type.h"
#include "apex/hashes.h"
#include "utils/file/file.h"

using String = std::string;


template<class E>
requires std::is_enum_v<E>
std::string to_string(E e) {
    return std::format("{}", e); // uses std::formatter<E>
}

template<uint32 hash, uint32 width>
struct StringHash : ADF::BaseType {
    uint64 storage{0};


    void read(IO::File &buffer) override {
        if constexpr (width == 4) {
            storage = buffer.read_pod<uint32>();
            return;
        }
        else if constexpr (width == 6 || width == 8) {
            storage = buffer.read_pod<uint64>();
            return;
        }

        static_assert(width == 4 || width == 6 || width == 8, "Unsupported StringHash width");
        throw std::runtime_error("Unsupported StringHash width");
    };

    void print(std::ostream &out) const override {
        if (const auto str = find_name64_sv(storage)) {
            out << *str;
        }
        else {
            out << std::format("0x{:0{}X}", storage, width * 2);
        }
    };

    void to_json(std::ostream &out) const override {
        if (const auto str = find_name64_sv(storage)) {
            out << std::format("\"{}\"", *str);
        }
        else {
            out << std::format("\"0x{:0{}X}\"", storage, width * 2);
        }
    }


    operator int64() const {
        return static_cast<int64>(storage);
    }
};

struct Deferred {
    uint32 offset;
    uint32 size;
    uint32 type_hash;
    uint32 pad;

    static std::unique_ptr<ADF::BaseType> read(IO::File &buffer);
};


template<class T, class = void>
inline constexpr bool is_dataset_v = false;

template<class T>
inline constexpr bool is_dataset_v<T, std::void_t<decltype(sizeof(T))> > = std::is_base_of_v<ADF::BaseType, T>;

template<typename T>
class Vector : public std::vector<T>, public ADF::BaseType {
public:
    void read(IO::File &buffer) override {
        const auto offset = buffer.read_pod<uint32>();
        const auto unk0 = buffer.read_pod<uint32>();
        const auto count = buffer.read_pod<uint32>();
        const auto unk1 = buffer.read_pod<uint32>();

        (void) unk0;
        (void) unk1;

        const std::streamoff original_offset = buffer.get_position();
        buffer.set_position(offset,std::ios::beg);
        this->resize(count);
        if constexpr (std::is_same_v<T, std::string>) {
            for (uint32 i = 0; i < count; ++i) {
                std::string &str = this->operator[](i);
                buffer.read_cstring(str);
            }
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<BaseType>>) {
            for (uint32 i = 0; i < count; ++i) {
                std::unique_ptr<BaseType> &ptr = this->operator[](i);
                ptr = Deferred::read(buffer);
            }
        }
        else if constexpr(is_dataset_v<T>) {
            for (uint32 i = 0; i < count; ++i) {
                this->operator[](i).read(buffer);
            }
        }
        else {
            buffer.read_exact<T>(*this);
        }
        buffer.set_position(original_offset, std::ios::beg);
    }

    void print(std::ostream &out) const override {
        out << "[";
        for (size_t i = 0; i < this->size(); ++i) {
            const auto& ptr = this->operator[](i);
            if constexpr (std::is_same_v<T, std::string>) {
                out << std::quoted(ptr);
            }
            else if constexpr(is_dataset_v<T>) {
                ptr.print(out);
            }
            else if constexpr(std::is_same_v<T, std::unique_ptr<BaseType>>) {
                if (ptr) {
                    ptr->print(out);
                }
                else {
                    out << "null";
                }
            }
            else {
                out << std::to_string(ptr);
            }
            if (i < this->size() - 1) {
                out << ", ";
            }
        }
        out << "]";
    }

    void to_json(std::ostream &out) const override {
        out << "[";
        for (size_t i = 0; i < this->size(); ++i) {
            const auto& ptr = this->operator[](i);
            if constexpr (std::is_same_v<T, std::string>) {
                out << std::quoted(ptr);
            }
            else if constexpr(is_dataset_v<T>) {
                ptr.to_json(out);
            }
            else if constexpr (std::is_same_v<T, std::unique_ptr<BaseType>>) {
                if (ptr) {
                    ptr->to_json(out);
                }
                else {
                    out << "null";
                }
            }
            else {
                out << std::to_string(ptr);
            }
            if (i < this->size() - 1) {
                out << ", ";
            }
        }
        out << "]";
    }
};

#endif //APEXPREDATOR_ADF_SUPPORT_TYPES_H
