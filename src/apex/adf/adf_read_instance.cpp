// Created by RED on 11.03.2026.
#include "apex/adf/adf.h"
#include "apex/adf/adf_base_type.h"

#include <memory>

#include "apex/adf/adf_support_types.h"
#include "redscore/platform/file/memory_file.h"

extern ADF::TypeInfoMap adf_type_info;

std::unique_ptr<ADF::BaseType> ADF::ADFFile::read_instance(const uint32 index) {
    const auto &instance_info = m_instances[index];
    m_buffer->set_position(instance_info.offset, std::ios::beg);

    std::vector<uint8> instance_data(instance_info.size);
    m_buffer->read_exact(instance_data);

    auto type_info = adf_type_info.find(instance_info.type_hash);

    if (type_info != adf_type_info.end()) {
        const TypeInfo *type = type_info->second;
        std::unique_ptr<BaseType> instance = type->new_instance();
        IO::MemoryViewFile buffer(instance_data);
        instance->read(buffer);
        return instance;
    }
    return nullptr;
}

std::unique_ptr<ADF::BaseType> Deferred::read(IO::File &buffer) {
    auto info = buffer.read_pod<Deferred>();
    if (info.size==0) {
        return {};
    }
    const auto inner_type = adf_type_info.find(info.type_hash);
    if (inner_type == adf_type_info.end()) {
        throw std::runtime_error(std::format("Failed to find type with hash 0x{:08X}", info.type_hash));
    }

    const std::streamoff original_offset = buffer.get_position();
    buffer.set_position(info.offset, std::ios::beg);
    std::unique_ptr<ADF::BaseType> result = inner_type->second->new_instance();
    result->read(buffer);

    buffer.set_position(original_offset,std::ios::beg);
    return std::move(result);
}