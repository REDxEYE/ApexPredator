// Created by RED on 06.02.2026.

#ifndef APEXPREDATOR_HAVOK_SUPPORT_TYPES_H
#define APEXPREDATOR_HAVOK_SUPPORT_TYPES_H

#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

#include "havok/havok_base_type.h"
#include "havok/tag_file/havok_tag_file.h"
#include "tag_file/havok_tag_file_get_item.h"

template<typename T>
std::unique_ptr<Havok::BaseType> new_instance() {
    return std::make_unique<T>();
}

struct hkVector4f : Havok::BaseType {
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        value.x = buffer.read_pod<float32>();
        value.y = buffer.read_pod<float32>();
        value.z = buffer.read_pod<float32>();
        value.w = buffer.read_pod<float32>();
    }

    void print(std::ostream &out) const override {
        out << "{" << value.x << ", " << value.y << ", " << value.z << ", " << value.w << "}";
    }

    void to_json(std::ostream &out) const override {
        out << "{" << value.x << ", " << value.y << ", " << value.z << ", " << value.w << "}";
    }

    operator glm::vec3() const { return value; }
    operator glm::vec4() const { return value; }
    operator glm::quat() const { return {value.w, value.x, value.y, value.z}; }

    glm::vec4 value;
};


typedef bool hkBool;

template<typename F>
struct hkRotationImpl : Havok::BaseType, glm::mat<4, 3, F> {
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 4; j++) {
                (*this)[i][j] = buffer.read_pod<F>();
            }
        }
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkRotationImpl is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkRotationImpl is not supported yet");
    }
};

template<typename F>
struct hkMatrix3Impl : Havok::BaseType, glm::mat<3, 3, F> {
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                (*this)[j][i] = buffer.read_pod<F>();
            }
        }
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkMatrix3Impl is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkMatrix3Impl is not supported yet");
    }
};

struct hkReflect_Type : Havok::BaseType {
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkReflect__Type is not supported yet");
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkReflect__Type is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkReflect__Type is not supported yet");
    }

    uint64 value{};
};

struct hkReflect_Detail_Opaque : Havok::BaseType {
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkReflect_Detail_Opaque is not supported yet");
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkReflect_Detail_Opaque is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkReflect_Detail_Opaque is not supported yet");
    }

    uint64 value{};
};

template<typename T>
struct hkReflect_QualifiedType : Havok::BaseType {
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkReflect__QualifiedType is not supported yet");
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkReflect__QualifiedType is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkReflect__QualifiedType is not supported yet");
    }

    uint64 value{};
};

struct hkBaseObject : Havok::BaseType {
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        buffer.skip(8);
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkBaseObject is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkBaseObject is not supported yet");
    }

    uint64 value;
};

class hkString : public Havok::BaseType {
public:
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        const auto index = buffer.read_pod<uint64>();
        if (index==0) {
            value = "";
            return;
        }
        const auto info = tag_file.get_item_info(index);
        auto item_buffer = tag_file.get_item_buffer(index);
        item_buffer.read_string(info.count - 1/*exclude null byte*/, value);
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkString is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkString is not supported yet");
    }

    operator std::string_view() const { return value; }
    operator std::string_view() { return value; }

private:
    std::string value;
};


template<class T>
std::unique_ptr<T> unique_ptr_downcast(std::unique_ptr<Havok::BaseType> &&p) {
    if (auto *t = dynamic_cast<T *>(p.get())) {
        p.release();
        return std::unique_ptr<T>(t);
    }
    throw std::bad_cast{};
}

template<typename T, typename Allocator>
class hkArray : public Havok::BaseType, public std::vector<T> {
public:
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        const auto index = buffer.read_pod<uint64>();
        buffer.read_pod<uint32>(); // Unused
        buffer.read_pod<uint32>(); // Unused
        if (index == 0) {
            return;
        }
        const auto info = tag_file.get_item_info(index);
        auto item_buffer = tag_file.get_item_buffer(index);
        if (info.count > 0) {
            this->resize(info.count);
            if constexpr (std::is_base_of_v<BaseType, T>) {
                for (uint32 i = 0; i < info.count; i++) {
                    this->operator[](i).read(item_buffer, tag_file);
                }
            }
            else if constexpr (std::is_base_of_v<std::unique_ptr<BaseType>, T>) {
                for (uint32 i = 0; i < info.count; i++) {
                    this->operator[](i)->read(item_buffer, tag_file);
                }
            }
            else {
                for (uint32 i = 0; i < info.count; i++) {
                    this->operator[](i) = item_buffer.read_pod<T>();
                }
            }
        }
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkPtrAndInt is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkPtrAndInt is not supported yet");
    }
};

template<typename T>
struct hkRelArray : hkArray<T, std::monostate> {
};

template<typename T, uint32 UNK>
class hkFreeListArray : Havok::BaseType {
public:
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkPtrAndInt is not supported yet");
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkPtrAndInt is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkPtrAndInt is not supported yet");
    }
};

template<typename E, typename S>
class hkEnum : public Havok::BaseType {
public:
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        value = static_cast<E>(buffer.read_pod<S>());
    }

    void print(std::ostream &out) const override {
        out << std::to_underlying(value);
    }

    void to_json(std::ostream &out) const override {
        out << std::to_underlying(value);
    }

    E value;
};

template<typename E, typename S>
std::ostream &operator<<(std::ostream &os, const hkEnum<E, S> &value) {
    os << std::to_underlying(value.value);
    return os;
}


template<typename E, typename S>
class hkFlags : public hkEnum<E, S> {
};

template<typename T, typename B, uint32 MASK>
struct hkPtrAndInt : Havok::BaseType {
    T *ptrAndInt;

    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkPtrAndInt is not supported yet");
    }

    void print(std::ostream &out) const override {
        out << "ptrAndInt: " << ptrAndInt;
    }

    void to_json(std::ostream &out) const override {
        out << "ptrAndInt: " << ptrAndInt;
    }
};

template<typename K, typename V>
class hkHashMap : public Havok::BaseType, public std::vector<std::pair<K, V> > {
public:
    hkHashMap() = default;

    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkPtrAndInt is not supported yet");
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkPtrAndInt is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkPtrAndInt is not supported yet");
    }
};

template<typename T>
    requires std::is_base_of_v<Havok::BaseType, T>
struct hkRefPtr : Havok::BaseType {
    hkRefPtr() = default;

    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        const auto index = buffer.read_pod<uint64>();
        if (index == 0) {
            ptr.reset(nullptr);
            return;
        }
        const auto info = tag_file.get_item_info(index);
        auto item = Havok::Tag::get_item(tag_file, index);
        ptr = std::move(item);
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkRefPtr is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkRefPtr is not supported yet");
    }

    hkRefPtr(const hkRefPtr &other) = delete;

    hkRefPtr(hkRefPtr &&other) noexcept : BaseType(std::move(other)),
                                          ptr(std::move(other.ptr)) {
    }

    hkRefPtr &operator=(const hkRefPtr &other) {
        if (this == &other)
            return *this;
        BaseType::operator =(other);
        ptr = other.ptr;
        return *this;
    }

    hkRefPtr &operator=(hkRefPtr &&other) noexcept {
        if (this == &other)
            return *this;
        BaseType::operator =(std::move(other));
        ptr = std::move(other.ptr);
        return *this;
    }

    [[nodiscard]] T *get() const noexcept { return ptr.get(); }
    T &operator*() const { return *ptr; }
    T *operator->() const noexcept { return ptr.get(); }
    explicit operator bool() const noexcept { return ptr; }

    void release() { ptr.release(); }
    void reset() { ptr.reset(); }

protected:
    std::unique_ptr<T> ptr;
};

struct hkRefVariant : hkRefPtr<Havok::BaseType> {
};


template<typename T>
struct hkPtr : Havok::BaseType {
    hkPtr() = default;

    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        static_assert(std::is_base_of_v<BaseType, T>, "hkPtr<T> requires T : BaseType");
        const auto index = buffer.read_pod<uint64>();
        if (index == 0) {
            ptr.reset(nullptr);
            return;
        }
        const auto info = tag_file.get_item_info(index);
        auto item = Havok::Tag::get_item(tag_file, index);
        if constexpr (std::is_same_v<T, hkRefVariant>) {
            ptr = std::make_unique<T>(std::move(item));
        }
        else {
            ptr = std::move(unique_ptr_downcast<T>(std::move(item)));
        }
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkRefPtr is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkRefPtr is not supported yet");
    }

    hkPtr(const hkPtr &other) : BaseType(other),
                                ptr(other.ptr) {
    }

    hkPtr(hkPtr &&other) noexcept : BaseType(std::move(other)),
                                    ptr(std::move(other.ptr)) {
    }

    hkPtr &operator=(const hkPtr &other) {
        if (this == &other)
            return *this;
        BaseType::operator =(other);
        ptr = other.ptr;
        return *this;
    }

    hkPtr &operator=(hkPtr &&other) noexcept {
        if (this == &other)
            return *this;
        BaseType::operator =(std::move(other));
        ptr = std::move(other.ptr);
        return *this;
    }

    T *get() const noexcept { return ptr.get(); }
    T &operator*() const { return *ptr; }
    T *operator->() const noexcept { return ptr.get(); }
    explicit operator bool() const noexcept { return ptr; }

private:
    std::unique_ptr<T> ptr;
};

template<typename V, uint32 HASH>
class hkHandle : public Havok::BaseType {
public:
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkHandle is not supported yet");
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkHandle is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkHandle is not supported yet");
    }

private:
    uint32 hash = HASH;
    V value;
};

template<typename V>
class hkaiIndex : public Havok::BaseType {
public:
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkHandle is not supported yet");
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkHandle is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkHandle is not supported yet");
    }

private:
    V value;
};

template<typename K, typename V>
class hkaiPackedKey_ : public Havok::BaseType {
public:
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkHandle is not supported yet");
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkHandle is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkHandle is not supported yet");
    }

private:
    K key;
    V value;
};

template<typename T>
struct hkFreeListArrayElement : Havok::BaseType {
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkFreeListArrayElement is not supported yet");
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkFreeListArrayElement is not supported yet");
    }

    void to_json(std::ostream &out) const override {
        throw std::runtime_error("hkFreeListArrayElement is not supported yet");
    }

    T value;
};

namespace Havok {
    template<typename T>
    concept HkType = std::is_base_of_v<BaseType, std::remove_cv_t<T> >;

    template<typename P>
    concept HkPointerLike = requires(const P &p)
    {
        p.get();
    };

    template<HkType To, HkType From>
    To *as(From *obj) {
        return dynamic_cast<To *>(obj);
    }

    template<HkType To, HkType From>
    const To *as(const From *obj) {
        return dynamic_cast<const To *>(obj);
    }

    template<HkType To, HkPointerLike Ptr>
    auto as(const Ptr &obj) {
        return as<To>(obj.get());
    }

    template<HkType To, HkType From, template<typename> typename Ptr>
    Ptr<To> convert(Ptr<From> &obj) {
        if (auto *ptr = dynamic_cast<To *>(obj.get())) {
            obj.release();
            return Ptr<To>(ptr);
        }
        return nullptr;
    }

    template<HkType To, HkType From, template<typename> typename Ptr>
    Ptr<To> convert(Ptr<From> &&obj) {
        return convert<To>(obj);
    }

    template<HkType To, HkType From>
    std::unique_ptr<To> convert(std::unique_ptr<From> &&obj) {
        if (auto *ptr = dynamic_cast<To *>(obj.get())) {
            obj.release();
            return std::unique_ptr<To>(ptr);
        }
        return nullptr;
    }
}

#endif //APEXPREDATOR_HAVOK_SUPPORT_TYPES_H
