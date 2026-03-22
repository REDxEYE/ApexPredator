// Created by RED on 19.09.2025.

#include "apex/adf/adf.h"

#include "apex/hashes.h"
#include "redscore/platform/logger.h"
#include "redscore/platform/file/memory_buffer.h"


ADF::Type ADF::Type::from_buffer(IO::File &buffer) {
    switch (auto def = buffer.read_pod<TypeDef>(); def.type) {
        case MetaType::StringType:
        case MetaType::Recursive:
        case MetaType::Primitive: {
            return {def, {}};
        }
        case MetaType::Structure: {
            const auto member_count = buffer.read_pod<uint32>();
            auto data = std::vector<StructMemberInfo>(member_count);

            for (int i = 0; i < member_count; ++i) {
                data[i] = buffer.read_pod<StructMemberInfo>();
            }

            return {def, data};
        }
        case MetaType::Enumeration: {
            const auto member_count = buffer.read_pod<uint32>();
            auto data = std::vector<EnumMemberInfo>(member_count);
            for (int i = 0; i < member_count; ++i) {
                data[i] = buffer.read_pod<EnumMemberInfo>();
            }
            return {def, data};
        }

        case MetaType::Bitfield: {
            auto bit_width = buffer.read_pod<uint32>();
            return {def, bit_width};
        }

        case MetaType::StringHash:
        case MetaType::Pointer: {
            auto inner_type_hash = buffer.read_pod<uint32>();
            return {def, inner_type_hash};
        }

        case MetaType::Array:
        case MetaType::InlineArray: {
            auto array_count = buffer.read_pod<uint32>();
            return {def, array_count};
        }

        case MetaType::DeferredType: {
            throw std::runtime_error("DeferredType is not supported in Type::from_buffer");
        }
    }
    throw std::runtime_error("Unknown MetaType in Type::from_buffer");
}

Buffer ADF::ADFFile::get_instance_data(const uint32 instance_id) const {
    auto &instance = m_instances[instance_id];
    m_buffer->set_position(instance.offset);
    std::vector<uint8> data(instance.size);
    m_buffer->read_exact(data);
    return Buffer(std::move(data));
}


ADF::ADFFile ADF::ADFFile::from_buffer(std::unique_ptr<IO::File> buffer) {
    const auto header = buffer->read_pod<Header>();
    const std::string comment = buffer->read_cstring();

    buffer->set_position(header.stringhash_offset, std::ios::beg);
    for (int i = 0; i < header.stringhash_count; ++i) {
        std::string hash_str = buffer->read_cstring();
        const auto string_hash = buffer->read_pod<uint64>();
        if (check_hash_presence(string_hash)) {
            continue;
        }
        store_hash_name(string_hash, hash_str);
    }

    std::vector<std::string> strings;
    strings.reserve(header.nametable_count);
    buffer->set_position(header.nametable_offset + header.nametable_count, std::ios::beg);
    for (int i = 0; i < header.nametable_count; ++i) {
        strings.push_back(buffer->read_cstring());
    }

    buffer->set_position(header.typedef_offset, std::ios::beg);
    std::vector<Type> types;
    types.reserve(header.typedef_count);
    for (int i = 0; i < header.typedef_count; ++i) {
        types.push_back(Type::from_buffer(*buffer));
    }

    buffer->set_position(header.instance_offset, std::ios::beg);
    std::vector<Instance> instances;
    instances.reserve(header.instance_count);
    for (int i = 0; i < header.instance_count; ++i) {
        instances.push_back(buffer->read_pod<Instance>());
    }
    return {header, comment, strings, instances, types, std::move(buffer)};
}

ADF::ADFFile ADF::ADFFile::from_buffer(const uint8 *data, const uint32 size) {
    auto buffer = std::vector<uint8>(size);
    std::copy_n(data, size, buffer.data());
    return from_buffer(std::move(std::make_unique<IO::MemoryFile>(std::move(buffer))));
}

