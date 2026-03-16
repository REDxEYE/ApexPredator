// Created by RED on 19.09.2025.

#ifndef APEXPREDATOR_STI_H
#define APEXPREDATOR_STI_H

#include <unordered_map>
#include <filesystem>
#include <optional>
#include <variant>

#include "adf.h"
#include "int_def.h"
#include "utils/file/file.h"

namespace STI {
    enum class DataType {
        Primitive = 0,
        Structure = 1,
        Pointer = 2,
        Array = 3,
        InlineArray = 4,
        StringType = 5,
        Recursive = 6,
        Bitfield = 7,
        Enumeration = 8,
        StringHash = 9,
        DeferredType = 10,
    };

    struct StructMember {
        std::string name;
        uint32 type_hash;
        uint32 size;
        uint32 offset: 24;
        uint32 bit_offset: 8;
        uint32 default_type;
        uint64 default_value;

        StructMember(const std::string_view name, const uint32 type_hash, const uint32 size, const uint32 offset,
                     const uint32 bit_offset,
                     const uint32 default_type, const uint64 default_value)
            : name(name),
              type_hash(type_hash),
              size(size),
              offset(offset),
              bit_offset(bit_offset),
              default_type(default_type),
              default_value(default_value) {
        }
    };

    struct EnumMember {
        std::string name;
        uint32 value;

        EnumMember(const std::string_view name, const uint32 value)
            : name(name),
              value(value) {
        }
    };


    struct TypeAndSize {
        uint32 count;
        uint32 type_hash;

        TypeAndSize(const uint32 count, const uint32 type_hash)
            : count(count),
              type_hash(type_hash) {
        }
    };


    using TypeData = std::variant<
        std::vector<StructMember>,
        std::vector<EnumMember>,
        TypeAndSize,
        uint32,
        std::monostate
    >;

    struct Type {
        uint32 hash; // Original hash
        uint32 size;
        uint32 alignment;
        DataType type;
        TypeData data;


        Type(const std::string_view name, const uint32 hash, const uint32 size, const uint32 alignment,
             const DataType type, TypeData data) : hash(hash),
                                                   size(size),
                                                   alignment(alignment),
                                                   type(type),
                                                   name_(name),
                                                   data(std::move(data)) {
        }

        [[nodiscard]] std::string type_name() const;

        [[nodiscard]] std::string name() const;

    private:
        Type() = default;

        std::string name_;
    };

    class TypeLibrary {
    public:
        TypeLibrary();

        const Type &register_type(const ADF::Type &adf_type, const ADF::ADFFile &adf);

        std::optional<std::reference_wrapper<const Type> > get_type(uint32 hash) const;

        const std::unordered_map<uint32, Type> &types() const;

    private:
        std::unordered_map<uint32, Type> m_types{};
        std::unordered_map<uint32, uint32> m_already_seen_name_hashes{};
        std::vector<uint32> m_exported_hashes{};
    };

    void register_types_from_adf(TypeLibrary &lib, const ADF::ADFFile &adf);

    void generate_code(const TypeLibrary &lib,
                        const std::filesystem::path &sources_path,
                        const std::filesystem::path &headers_path);

    // Type *TypeLibrary_register_adf_type(TypeLibrary *lib, const ADF *adf, const ADFType *adf_type);
    //
    //  Type *TypeLibrary_new_type(TypeLibrary *lib, MetaType meta_type, uint32 type_hash, String* name);
    // Type *TypeLibrary_register_type(TypeLibrary *lib, ADFType *adf_type);
    //
    // int32 TypeLibrary_types_count(const TypeLibrary *lib);
    //
    // void TypeLibrary_free(TypeLibrary *lib);
    //
    // void TypeLibrary_generate_types(TypeLibrary *lib, const String *namespace_, FILE *header_output,
    //                                 const String *relative_header_path, FILE *impl_output);
    //
    // const Type *TypeLibrary_get_type(const TypeLibrary *lib, uint32 type_hash);
    //
    // void start_type_dump(TypeLibrary *lib);
}

template<>
struct std::formatter<STI::DataType> : std::formatter<std::string_view> {
    auto format(const STI::DataType data_type, std::format_context &ctx) const {
        std::string_view type_name;
        switch (data_type) {
            case STI::DataType::Primitive: type_name = "Primitive";
                break;
            case STI::DataType::Structure: type_name = "Structure";
                break;
            case STI::DataType::Pointer: type_name = "Pointer";
                break;
            case STI::DataType::Array: type_name = "Array";
                break;
            case STI::DataType::InlineArray: type_name = "InlineArray";
                break;
            case STI::DataType::StringType: type_name = "StringType";
                break;
            case STI::DataType::Recursive: type_name = "Recursive";
                break;
            case STI::DataType::Bitfield: type_name = "Bitfield";
                break;
            case STI::DataType::Enumeration: type_name = "Enumeration";
                break;
            case STI::DataType::StringHash: type_name = "StringHash";
                break;
            case STI::DataType::DeferredType: type_name = "DeferredType";
                break;
            default: type_name = "Unknown";
                break;
        }
        return std::formatter<std::string_view>::format(type_name, ctx);
    }
};

const uint32 STI_TYPE_HASH_INT8 = 0x580D0A62;
const uint32 STI_TYPE_HASH_UINT8 = 0x0CA2821D;
const uint32 STI_TYPE_HASH_INT16 = 0xD13FCF93;
const uint32 STI_TYPE_HASH_UINT16 = 0x86D152BD;
const uint32 STI_TYPE_HASH_INT32 = 0x192FE633;
const uint32 STI_TYPE_HASH_UINT32 = 0x075E4E4F;
const uint32 STI_TYPE_HASH_INT64 = 0xAF41354F;
const uint32 STI_TYPE_HASH_UINT64 = 0xA139E01F;
const uint32 STI_TYPE_HASH_FLOAT32 = 0x7515A207;
const uint32 STI_TYPE_HASH_FLOAT64 = 0xC609F663;
const uint32 STI_TYPE_HASH_STRING = 0x8955583E;
const uint32 STI_TYPE_HASH_DEFERRED = 0xDEFE88ED;

#endif //APEXPREDATOR_STI_H
