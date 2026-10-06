// Created by RED on 19.09.2025.

#include "apex/adf/adf.h"

#include "apex/hashes.h"
#include "redscore/platform/logger.h"
#include "redscore/platform/file/memory_file.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

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

IO::Buffer ADF::ADFFile::get_instance_data(const uint32 instance_id) const {
    auto &instance = m_instances[instance_id];
    m_buffer->set_position(instance.offset);
    std::vector<uint8> data(instance.size);
    m_buffer->read_exact(data);
    return IO::Buffer(std::move(data));
}


ADF::ADFFile ADF::ADFFile::from_buffer(std::unique_ptr<IO::File> buffer, const uint32 small_instance_alignment) {
    const auto small_header = buffer->read_pod<SmallHeader>();
    if (std::memcmp(small_header.ident, ADF_SMALL_MAGIC, 4) == 0) {
        if (small_instance_alignment == 0) {
            throw std::invalid_argument("Small ADF requires the root type's alignment");
        }
        const uint32 offset = std::max<uint32>(sizeof(SmallHeader), small_instance_alignment);
        const auto file_size = buffer->get_size();
        if (file_size < offset || file_size - offset > std::numeric_limits<uint32>::max()) {
            throw std::runtime_error("Invalid small ADF instance size");
        }
        Header header{};
        std::memcpy(header.ident, small_header.ident, sizeof(header.ident));
        std::vector<Instance> instances{{0, small_header.type_hash, offset,
                                         static_cast<uint32>(file_size - offset), 0}};
        return {header, {}, {"unnamed"}, instances, {}, std::move(buffer)};
    }
    buffer->set_position(0);
    const auto header = buffer->read_pod<Header>();
    const std::string comment = buffer->read_cstring();

    buffer->set_position(header.stringhash_offset, std::ios::beg);
    for (int i = 0; i < header.stringhash_count; ++i) {
        std::string hash_str = buffer->read_cstring();
        const auto string_hash = buffer->read_pod<uint64>();
        if (check_hash_presence(string_hash)) {
            continue;
        }
        store_hash_name(hash_str);
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

ADF::ADFFile ADF::ADFFile::from_buffer(const uint8 *data, const uint32 size,
                                      const uint32 small_instance_alignment) {
    auto buffer = std::vector<uint8>(size);
    std::copy_n(data, size, buffer.data());
    return from_buffer(std::make_unique<IO::MemoryFile>(std::move(buffer)), small_instance_alignment);
}

