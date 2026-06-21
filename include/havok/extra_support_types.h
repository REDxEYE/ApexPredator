// Created by RED on 05.03.2026.
#pragma once
#include "havok/generated/havok_types_fwd.h"

template<typename STORAGE>
    requires std::is_base_of_v<Havok::BaseType, STORAGE>
struct hkcdStaticTree_Tree : STORAGE {
    HavokTypes::hkAabb domain;

    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        this->nodes.read(buffer, tag_file);
        domain.read(buffer, tag_file);
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkcdStaticTree_Tree is not supported yet");
    }

    [[nodiscard]] nlohmann::json to_json() const override {
        nlohmann::json obj;
        obj["nodes"] = this->nodes.to_json();
        obj["domain"] = domain.to_json();
        return obj;
    }
};

template<typename STORAGE>
struct hkcdDynamicTree_Tree : Havok::BaseType {
    HavokTypes::hkUint32 numLeaves; // offset: 24, size: 4
    HavokTypes::hkUint32 path; // offset: 28, size: 4
    unsigned short root; // offset: 32, size: 2
    STORAGE storage;

    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkcdStaticTree_Tree is not supported yet");
    }

    void print(std::ostream &out) const override {
        throw std::runtime_error("hkcdStaticTree_Tree is not supported yet");
    }

    [[nodiscard]] nlohmann::json to_json() const override {
        throw std::runtime_error("hkcdStaticTree_Tree is not supported yet");
    }
};

template<typename CODEC>
struct hkcdStaticTree: Havok::BaseType {
    hkArray<CODEC, std::monostate> nodes; // offset: 0, size: 16
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        nodes.read(buffer, tag_file);
    }
};

template<typename CODEC>
struct hkcdStaticTree_DynamicStorage : hkcdStaticTree<CODEC> {

    void print(std::ostream &os) const override {
        throw std::runtime_error("hkcdStaticTree_DynamicStorage is not supported yet");
    }

    [[nodiscard]] nlohmann::json to_json() const override {
        throw std::runtime_error("hkcdStaticTree_DynamicStorage is not supported yet");
    }
};

template<typename CODEC>
struct hkcdDynamicTree_DefaultDynamicStorage : hkcdStaticTree<CODEC> {
    hkArray<CODEC, std::monostate> nodes; // offset: 0, size: 16

    void print(std::ostream &os) const override {
        throw std::runtime_error("hkcdStaticTree_DynamicStorage is not supported yet");
    }

    [[nodiscard]] nlohmann::json to_json() const override {
        throw std::runtime_error("hkcdStaticTree_DynamicStorage is not supported yet");
    }
};

template<typename T>
struct hknpSparseCompactMap : Havok::BaseType {
    uint32 secondaryKeyMask; // offset: 0, size: 4
    uint32 sencondaryKeyBits; // offset: 4, size: 4
    hkArray<T, std::monostate> primaryKeyToIndex; // offset: 8, size: 16
    hkArray<T, std::monostate> valueAndSecondaryKeys; // offset: 24, size: 16

    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        secondaryKeyMask = buffer.read_pod<u32>();
        sencondaryKeyBits = buffer.read_pod<u32>();
        primaryKeyToIndex.read(buffer,tag_file);
        valueAndSecondaryKeys.read(buffer,tag_file);
        // throw std::runtime_error("hknpSparseCompactMap is not supported yet");
    }

    void print(std::ostream &os) const override {
        throw std::runtime_error("hknpSparseCompactMap is not supported yet");
    }

    [[nodiscard]] nlohmann::json to_json() const override {
        nlohmann::json obj;
        obj["secondaryKeyMask"] = secondaryKeyMask;
        obj["sencondaryKeyBits"] = sencondaryKeyBits;
        obj["primaryKeyToIndex"] = primaryKeyToIndex.to_json();
        obj["valueAndSecondaryKeys"] = valueAndSecondaryKeys.to_json();
        return obj;
    }
};


template<typename tTRIANGLE_DATA>
struct hkcdStaticMeshTreeBase_PrimitiveDataRunBase : Havok::BaseType {
    tTRIANGLE_DATA value; // offset: 0, size: 2
    uint32 index; // offset: 2, size: 1
    uint32 count; // offset: 3, size: 1

    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkcdStaticMeshTreeBase_PrimitiveDataRunBase is not supported yet");
    }

    void print(std::ostream &os) const override {
        throw std::runtime_error("hkcdStaticMeshTreeBase_PrimitiveDataRunBase is not supported yet");
    }

    [[nodiscard]] nlohmann::json to_json() const override {
        throw std::runtime_error("hkcdStaticMeshTreeBase_PrimitiveDataRunBase is not supported yet");
    }
};

template<typename tStorage>
struct hkBitFieldBase : Havok::BaseType {
    tStorage storage; // offset: 0, size: 24

    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkBitFieldBase is not supported yet");
    }

    void print(std::ostream &os) const override {
        throw std::runtime_error("hkBitFieldBase is not supported yet");
    }

    [[nodiscard]] nlohmann::json to_json() const override {
        throw std::runtime_error("hkBitFieldBase is not supported yet");
    }
};

template<typename tStorage>
struct hkBitFieldStorage : Havok::BaseType {
    tStorage words; // offset: 0, size: 16
    int numBits; // offset: 16, size: 4

    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        words.read(buffer, tag_file);
        numBits = buffer.read_pod<int>();
    }

    void print(std::ostream &os) const override {
        throw std::runtime_error("hkBitFieldStorage is not supported yet");
    }

    [[nodiscard]] nlohmann::json to_json() const override {
        nlohmann::json obj;
        obj["words"] = words.to_json();
        obj["numBits"] = numBits;
        return obj;
    }
};

template<typename tPACKED, typename tSHARED, int64 vPACKED_BITS, int64 vSHARED_BITS>
//<hkUint32, hkUint64, 11, 21>
struct hkcdStaticMeshTreeCommonConfig : Havok::BaseType {
    void read(IO::File &buffer, Havok::Tag::TagFile &tag_file) override {
        throw std::runtime_error("hkcdStaticMeshTreeCommonConfig is not supported yet");
    }

    void print(std::ostream &os) const override {
        throw std::runtime_error("hkcdStaticMeshTreeCommonConfig is not supported yet");
    }

    [[nodiscard]] nlohmann::json to_json() const override {
        throw std::runtime_error("hkcdStaticMeshTreeCommonConfig is not supported yet");
    }
};



inline bool operator==(const HavokTypes::hkStringPtr& lhs, const HavokTypes::hkStringPtr& rhs) {
    return std::string_view(lhs.stringAndFlag) == rhs.stringAndFlag;
}

inline bool operator==(const HavokTypes::hkStringPtr& lhs, const std::string_view rhs) {
    return lhs.stringAndFlag == rhs;
}
