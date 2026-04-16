// Created by RED on 19.09.2025.

#ifndef APEXPREDATOR_ADF_H
#define APEXPREDATOR_ADF_H
#include <format>
#include <utility>
#include <variant>

#include "apex/adf/adf_base_type.h"
#include "redscore/platform/buffer/buffer.h"
#include "redscore/platform/file/file.h"

#define ADF_MAGIC " FDA"

namespace ADF {

    template<typename T>
        requires std::is_base_of_v<BaseType, T>
    std::unique_ptr<T> convert(std::unique_ptr<BaseType> &obj) {
        if (auto *ptr = dynamic_cast<T *>(obj.get())) {
            obj.release();
            return std::unique_ptr<T>(ptr);
        }
        return nullptr;
    }

    template<typename T>
        requires std::is_base_of_v<BaseType, T>
    std::unique_ptr<T> convert(std::unique_ptr<BaseType> &&obj) {
        return convert<T>(obj);
    }

    template<typename T>
        requires std::is_base_of_v<BaseType, T>
    T *as(BaseType *obj) {
        return dynamic_cast<T *>(obj);
    }

    template<typename T>
        requires std::is_base_of_v<BaseType, T>
    const T *as(const BaseType *obj) {
        return dynamic_cast<const T *>(obj);
    }

    template<typename T>
        requires std::is_base_of_v<BaseType, T>
    T *as(const std::unique_ptr<BaseType> &obj) {
        return as<T>(obj.get());
    }

    template<typename T>
        requires std::is_base_of_v<BaseType, T>
    const T *as(const std::unique_ptr<const BaseType> &obj) {
        return as<T>(obj.get());
    }

    enum class MetaType:uint32 {
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

#pragma pack(push, 1)
    struct Header {
        char ident[4];
        uint32 version;
        uint32 instance_count;
        uint32 instance_offset;

        uint32 typedef_count;
        uint32 typedef_offset;

        uint32 stringhash_count;
        uint32 stringhash_offset;

        uint32 nametable_count;
        uint32 nametable_offset;

        uint32 total_size;

        uint32 m_MetaDataOffset;
        uint32 m_FlagField;
        uint32 m_IncludedLibraries;
        uint64 gap;
    };

    struct Instance {
        uint32 name_hash;
        uint32 type_hash;
        uint32 offset;
        uint32 size;
        uint64 name_id;
    };

    struct TypeDef {
        MetaType type;
        uint32 size;
        uint32 alignment;
        uint32 hash;
        uint64 name_id;
        uint16 flags;
        uint16 scalar_type;
        uint32 element_type_hash;
        uint32 element_len;
    };

    struct StructMemberInfo {
        uint64 name_id;
        uint32 type_hash;
        uint32 size;
        uint32 offset: 24;
        uint32 bit_offset: 8;
        uint32 default_type;
        uint64 default_value;
    };

    struct EnumMemberInfo {
        uint64 name_id;
        uint32 value;
    };

#pragma pack(pop)

    using TypeData = std::variant<
        std::vector<StructMemberInfo>,
        std::vector<EnumMemberInfo>,
        uint32
    >;

    class Type {
    public:
        static Type from_buffer(IO::File &buffer);

        Type(const TypeDef &def, TypeData type_data)
            : m_def(def),
              m_type_data(std::move(type_data)) {
        }

        [[nodiscard]] const TypeDef &def() const {
            return m_def;
        }

        [[nodiscard]] const TypeData &type_data() const {
            return m_type_data;
        }

    private:
        TypeDef m_def;
        TypeData m_type_data;
    };

    class ADFFile {
    public:
        ~ADFFile() {
            if (m_buffer) {
                m_buffer.reset();
            }
            m_instances.clear();
            m_types.clear();
            m_strings.clear();
            m_comment.clear();
        }

        IO::Buffer get_instance_data(uint32 instance_id) const;

        static ADFFile from_buffer(std::unique_ptr<IO::File> buffer);

        static ADFFile from_buffer(const uint8 *data, uint32 size);


        [[nodiscard]] std::string_view get_string(const uint32 index) const {
            if (index >= m_strings.size()) {
                throw std::out_of_range("String index out of range");
            }
            return m_strings[index];
        }

        [[nodiscard]] const std::vector<Instance> &instances() const {
            return m_instances;
        }

        [[nodiscard]] const std::vector<Type> &types() const {
            return m_types;
        }

        std::unique_ptr<BaseType> read_instance(uint32 index);

        template<typename T>
            requires std::is_base_of_v<BaseType, T>
        std::unique_ptr<T> read_instance(const uint32 index) {
            auto ptr = read_instance(index);
            return convert<T>(std::move(ptr));
        }



    private:
        ADFFile(const Header &m_header, std::string m_comment, const std::vector<std::string> &m_strings,
                const std::vector<Instance> &m_instances, const std::vector<Type> &m_types,
                std::unique_ptr<IO::File> buffer)
            : m_header(m_header),
              m_comment(std::move(m_comment)),
              m_strings(m_strings),
              m_instances(m_instances),
              m_types(m_types),
              m_buffer(std::move(buffer)) {
        }

        ADFFile() = default;

        Header m_header{};
        std::string m_comment;
        std::vector<std::string> m_strings;
        std::vector<Instance> m_instances;
        std::vector<Type> m_types;

        std::unique_ptr<IO::File> m_buffer;
    };
}


template<>
struct std::formatter<ADF::MetaType> : std::formatter<std::string_view> {
    auto format(const ADF::MetaType &type, std::format_context &ctx) const {
        std::string_view type_name;
        switch (type) {
            case ADF::MetaType::Primitive: type_name = "Primitive";
                break;
            case ADF::MetaType::Structure: type_name = "Structure";
                break;
            case ADF::MetaType::Pointer: type_name = "Pointer";
                break;
            case ADF::MetaType::Array: type_name = "Array";
                break;
            case ADF::MetaType::InlineArray: type_name = "InlineArray";
                break;
            case ADF::MetaType::StringType: type_name = "StringType";
                break;
            case ADF::MetaType::Recursive: type_name = "Recursive";
                break;
            case ADF::MetaType::Bitfield: type_name = "Bitfield";
                break;
            case ADF::MetaType::Enumeration: type_name = "Enumeration";
                break;
            case ADF::MetaType::StringHash: type_name = "StringHash";
                break;
            case ADF::MetaType::DeferredType: type_name = "DeferredType";
                break;
            default: type_name = "Unknown";
                break;
        }
        return std::formatter<std::string_view>::format(type_name, ctx);
    }
};

#endif //APEXPREDATOR_ADF_H
