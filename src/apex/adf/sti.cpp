// Created by RED on 19.09.2025.
#include "apex/adf/sti.h"

#include "redscore/platform/logger.h"
#include "utils/hash_helper.h"

std::string STI::Type::type_name() const {
    switch (type) {
        case DataType::Array: {
            const auto type_and_size = std::get<TypeAndSize>(data);
            if (type_and_size.type_hash == STI_TYPE_HASH_DEFERRED) {
                return std::format("Array<Deferred, {}>", type_and_size.count);
            }
            return name_;
        }
        case DataType::Pointer: {
            return name_ + "*";
        }
        case DataType::Primitive:
        case DataType::Structure:
        case DataType::InlineArray:
        case DataType::StringType:
        case DataType::Recursive:
        case DataType::Bitfield:
        case DataType::Enumeration:
            return name_;
        case DataType::DeferredType:
            return "std::unique_ptr<BaseType>";
        case DataType::StringHash: {
            return std::format("StringHash<0x{:08X}, {}>", std::get<uint32>(data), size);
        }
    }
    throw std::runtime_error("Unknown type");
}

std::string STI::Type::name() const {
    if (type == DataType::Array) {
        const auto type_and_size = std::get<TypeAndSize>(data);
        if (type_and_size.type_hash == STI_TYPE_HASH_DEFERRED) {
            return std::format("Deferred_Array", type_and_size.count);
        }
        return name_;
    }
    if (type==DataType::Pointer) {
        return name_ + "_Ptr";
    }
    if (type == DataType::DeferredType) {
        return "Deferred";
    }
    return name_;
}

STI::TypeLibrary::TypeLibrary() {
    //s8 = 0x580D0A62
    m_types.emplace(STI_TYPE_HASH_INT8, Type("int8", STI_TYPE_HASH_INT8, 1, 1, DataType::Primitive, {}));

    //u8 = 0x0ca2821d
    m_types.emplace(STI_TYPE_HASH_UINT8, Type("uint8", STI_TYPE_HASH_UINT8, 1, 1, DataType::Primitive, {}));

    //s16 = 0xD13FCF93
    m_types.emplace(STI_TYPE_HASH_INT16, Type("int16", STI_TYPE_HASH_INT16, 2, 2, DataType::Primitive, {}));

    //u16 = 0x86d152bd
    m_types.emplace(STI_TYPE_HASH_UINT16, Type("uint16", STI_TYPE_HASH_UINT16, 2, 2, DataType::Primitive, {}));

    //s32 = 0x192fe633
    m_types.emplace(STI_TYPE_HASH_INT32, Type("int32", STI_TYPE_HASH_INT32, 4, 4, DataType::Primitive, {}));

    //u32 = 0x075e4e4f
    m_types.emplace(STI_TYPE_HASH_UINT32, Type("uint32", STI_TYPE_HASH_UINT32, 4, 4, DataType::Primitive, {}));

    //s64 = 0xAF41354F
    m_types.emplace(STI_TYPE_HASH_INT64, Type("int64", STI_TYPE_HASH_INT64, 8, 8, DataType::Primitive, {}));

    //u64 = 0xA139E01F
    m_types.emplace(STI_TYPE_HASH_UINT64, Type("uint64", STI_TYPE_HASH_UINT64, 8, 8, DataType::Primitive, {}));

    //f32 = 0x7515A207
    m_types.emplace(STI_TYPE_HASH_FLOAT32, Type("float32", STI_TYPE_HASH_FLOAT32, 4, 4, DataType::Primitive, {}));

    //f64 = 0xC609F663
    m_types.emplace(STI_TYPE_HASH_FLOAT64, Type("float64", STI_TYPE_HASH_FLOAT64, 8, 8, DataType::Primitive, {}));

    //string = 0x8955583E
    m_types.emplace(STI_TYPE_HASH_STRING, Type("String", STI_TYPE_HASH_STRING, 8, 8, DataType::StringType, {}));

    //Deferred = 0xDEFE88ED
    m_types.emplace(STI_TYPE_HASH_DEFERRED,
                    Type("Deferred", STI_TYPE_HASH_DEFERRED, 16, 8, DataType::DeferredType, {}));
}

STI::DataType remap_adf_type(const ADF::MetaType adf_meta_type) {
    switch (adf_meta_type) {
        case ADF::MetaType::Primitive:
            return STI::DataType::Primitive;
        case ADF::MetaType::Structure:
            return STI::DataType::Structure;
        case ADF::MetaType::Pointer:
            return STI::DataType::Pointer;
        case ADF::MetaType::Array:
            return STI::DataType::Array;
        case ADF::MetaType::InlineArray:
            return STI::DataType::InlineArray;
        case ADF::MetaType::StringType:
            return STI::DataType::StringType;
        case ADF::MetaType::Recursive:
            return STI::DataType::Recursive;
        case ADF::MetaType::Bitfield:
            return STI::DataType::Bitfield;
        case ADF::MetaType::Enumeration:
            return STI::DataType::Enumeration;
        case ADF::MetaType::StringHash:
            return STI::DataType::StringHash;
        case ADF::MetaType::DeferredType:
            return STI::DataType::DeferredType;
        default:
            GLog_Error("Unknown ADF MetaType {}", adf_meta_type);
            throw std::runtime_error("Unknown ADF MetaType");
    }
}

const STI::Type &STI::TypeLibrary::register_type(const ADF::Type &adf_type, const ADF::ADFFile &adf) {
    const uint32 type_hash = adf_type.def().hash;
    const ADF::TypeDef &type_def = adf_type.def();
    std::string type_name = std::string(adf.get_string(type_def.name_id));
    uint32 name_hash = hash_string(type_name);

    // Check if type already registered and sizes match
    if (const auto it = m_types.find(type_hash); it != m_types.end()) {
        const Type &existing_type = it->second;
        if (existing_type.size != type_def.size) {
            GLog_Warning(
                "Type hash collision for type hash {:08X}: existing type '{}' has size {}, new type '{}' has size {}",
                type_hash, existing_type.type_name(), existing_type.size, type_name, type_def.size);
            throw std::runtime_error("Type hash collision with different sizes");
        }
    }

    switch (type_def.type) {
        case ADF::MetaType::Primitive: {
            m_already_seen_name_hashes.emplace(name_hash, type_hash);
            Type new_type(type_name, type_hash, type_def.size, type_def.alignment,
                          remap_adf_type(type_def.type), {});
            const auto &[entry, _] = m_types.emplace(type_hash, new_type);
            return entry->second;
        }
        case ADF::MetaType::Structure: {
            m_already_seen_name_hashes.emplace(name_hash, type_hash);
            const auto &adf_members = std::get<std::vector<ADF::StructMemberInfo> >(adf_type.type_data());
            std::vector<StructMember> members;
            members.reserve(adf_members.size());
            for (const auto &adf_member: adf_members) {
                std::string member_name = std::string(adf.get_string(adf_member.name_id));
                if (member_name == type_name) {
                    member_name += "_";
                }

                members.emplace_back(member_name,
                                     adf_member.type_hash,
                                     adf_member.size,
                                     adf_member.offset,
                                     adf_member.bit_offset,
                                     adf_member.default_type,
                                     adf_member.default_value);
            }
            const auto &[entry, _] = m_types.emplace(type_hash,
                                                     Type(type_name, type_hash,
                                                          type_def.size, type_def.alignment,
                                                          remap_adf_type(type_def.type),
                                                          members));
            return entry->second;
        }
        case ADF::MetaType::Pointer: {
            type_name = type_name.substr(0, type_name.size()-1);
            m_already_seen_name_hashes.emplace(hash_string(type_name), type_hash);
            const auto &[entry, _] = m_types.emplace(type_hash,
                                                     Type(type_name, type_hash,
                                                          type_def.size, type_def.alignment,
                                                          remap_adf_type(type_def.type),
                                                          type_def.element_type_hash));
            return entry->second;
        }
        case ADF::MetaType::Array: {
            type_name = type_name.substr(2, type_name.size() - 3);
            type_name += "_Array";
            name_hash = hash_string(type_name);

            const auto& existing_type_hash = m_already_seen_name_hashes.find(name_hash);
            if (existing_type_hash != m_already_seen_name_hashes.end() && existing_type_hash->second != type_hash) {
                GLog_Warning("Type name hash collision for type name '{}' with hash {:08X}", type_name, name_hash);
                type_name = adf.get_string(type_def.name_id);
                type_name = type_name.substr(2, type_name.size() - 3);
                type_name = std::format("{}_{:08X}_Array", type_name, type_hash);
                name_hash = hash_string(type_name);
            }
            m_already_seen_name_hashes.emplace(name_hash, type_hash);

            auto [type,_] = m_types.emplace(type_hash,
                                                     Type(type_name, type_hash,
                                                          type_def.size, type_def.alignment,
                                                          remap_adf_type(type_def.type),
                                                          TypeAndSize(0, type_def.element_type_hash)));
            return type->second;
        }
        case ADF::MetaType::InlineArray: {
            type_name = type_name.substr(3, type_name.size() - 4);
            type_name += std::format("_InlineArray_{}", type_def.element_len);
            name_hash = hash_string(type_name);

            const auto& existing_type_hash = m_already_seen_name_hashes.find(name_hash);
            if (existing_type_hash != m_already_seen_name_hashes.end() && existing_type_hash->second != type_hash) {
                GLog_Warning("Type name hash collision for type name '{}' with hash {:08X}", type_name, name_hash);
                type_name = std::format("{}_{:08X}_InlineArray_{}", adf.get_string(type_def.name_id), type_hash, type_def.element_len);
            }
            m_already_seen_name_hashes.emplace(name_hash, type_hash);

            TypeAndSize data(type_def.element_len, type_def.element_type_hash);
            const auto &[entry, _] = m_types.emplace(type_hash,
                                                     Type(type_name, type_hash,
                                                          type_def.size, type_def.alignment,
                                                          remap_adf_type(type_def.type), data));
            return entry->second;
        }
        case ADF::MetaType::StringType: {
            m_already_seen_name_hashes.emplace(name_hash, type_hash);
            const auto &[entry, _] = m_types.emplace(type_hash,
                                                     Type(type_name, type_hash,
                                                          type_def.size, type_def.alignment,
                                                          remap_adf_type(type_def.type), {}));
            return entry->second;
        }
        case ADF::MetaType::Recursive: {
            throw std::runtime_error("Recursive types are not supported in TypeLibrary::register_type");
        }
        case ADF::MetaType::Bitfield: {

            if (type_def.type == ADF::MetaType::Bitfield) {
                const size_t pos = type_name.find(':');
                if (pos != std::string::npos) {
                    type_name = type_name.substr(0, pos);
                }
            }
            m_already_seen_name_hashes.emplace(hash_string(type_name), type_hash);
            const auto &[entry, _] = m_types.emplace(type_hash,
                                                     Type(type_name, type_hash,
                                                          type_def.size, type_def.alignment,
                                                          remap_adf_type(type_def.type),
                                                          type_def.element_len));
            return entry->second;
        }
        case ADF::MetaType::Enumeration: {
            const auto &adf_members = std::get<std::vector<ADF::EnumMemberInfo> >(adf_type.type_data());
            std::vector<EnumMember> members;
            members.reserve(adf_members.size());
            for (const auto &[name_id, value]: adf_members) {
                members.emplace_back(adf.get_string(name_id), value);
            }
            m_already_seen_name_hashes.emplace(name_hash, type_hash);
            const auto &[entry, _] = m_types.emplace(type_hash,
                                                     Type(type_name, type_hash,
                                                          type_def.size, type_def.alignment,
                                                          remap_adf_type(type_def.type),
                                                          members));
            return entry->second;
        }
        case ADF::MetaType::StringHash: {
            m_already_seen_name_hashes.emplace(name_hash, type_hash);
            const auto &[entry, _] = m_types.emplace(type_hash,
                                                     Type(type_name, type_hash,
                                                          type_def.size, type_def.alignment,
                                                          remap_adf_type(type_def.type),
                                                          type_def.element_type_hash));
            return entry->second;
        }
        case ADF::MetaType::DeferredType: {
            throw std::runtime_error("DeferredType is not supported in TypeLibrary::register_type");
        }
    }
    throw std::runtime_error("Unknown ADF MetaType in TypeLibrary::register_type");
}

std::optional<std::reference_wrapper<const STI::Type> > STI::TypeLibrary::get_type(const uint32 hash) const {
    if (const auto it = m_types.find(hash); it != m_types.end()) {
        return it->second;
    }
    return std::nullopt;
}

const std::unordered_map<uint32, STI::Type> &STI::TypeLibrary::types() const {
    return m_types;
}

void STI::register_types_from_adf(TypeLibrary &lib, const ADF::ADFFile &adf) {
    for (const auto &adf_type: adf.types()) {
        lib.register_type(adf_type, adf);
    }
}
