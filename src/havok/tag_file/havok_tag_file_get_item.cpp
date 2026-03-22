// Created by RED on 03.03.2026.

#include "havok/tag_file/havok_tag_file_get_item.h"

#include "havok/havok_support_types.h"
#include "havok/tag_file/havok_tag_file.h"
#include "redscore/platform/logger.h"


namespace HavokTypes {
    struct hkContainerHeapAllocator;
}

extern Havok::TypeInfoMap havok_type_info;

std::unique_ptr<Havok::BaseType> Havok::Tag::get_item(TagFile &tag_file, uint32 i) {
    if (i == 0 || i >= tag_file.items().size()) {
        GLog_Error("Item index {} out of range", i);
        throw std::runtime_error("Item index out of range");
    }
    const auto &item = tag_file.get_item_info(i);

    auto item_slice = tag_file.data()->take_span(item.type()->size() * item.count, item.offset);

    const auto type_res = havok_type_info.find(item.type()->hash);
    if (type_res == havok_type_info.end()) {
        GLog_Error("Unknown type hash: {:08X}", item.type()->hash);
        throw std::runtime_error("Unknown type hash");
    }
    const auto type = type_res->second;
    if (type->new_instance != nullptr) {
        if (item.count == 1) {
            auto instance = type->new_instance();
            instance->read(item_slice, tag_file);
            return std::move(instance);
        }
        auto instances = std::make_unique<hkArray<std::unique_ptr<BaseType>, HavokTypes::hkContainerHeapAllocator> >();
        instances->reserve(item.count);
        for (uint32 j = 0; j < item.count; j++) {
            auto instance = type->new_instance();
            instance->read(item_slice, tag_file);
            instances->push_back(std::move(instance));
        }
        return std::move(instances);
    }
    GLog_Error("Type {} does not have new_instance function", type->name);
    throw std::runtime_error("Type does not have new_instance function");
}
