// Created by RED on 04.01.2026.
#include "havok/havok_helpers.h"

#include "havok/generated/havok_generated.h"
#include "platform/logger.h"
#include "utils/memory_profiling.h"
#include "utils/hash_helper.h"

TypedPtr * TagFile_get_item(const TagFile *tf, const uint32 index) {
    const HKItem *item = &tf->items.items[index+1];
    HKTagType *hk_tag_type = &tf->types.items[item->type];
    const uint32 type_hash = hash_string(HKTagType_stable_name(hk_tag_type));
    const HavokTypeInfo *type_info = *(HavokTypeInfo **) DM_get(&HAVOK_TYPES_type_info, type_hash);
    TypedPtr *item_obj = (TypedPtr*)mp_malloc(type_info->size);
    type_info->init(item_obj);
    type_info->read(item_obj, tf, &tf->data.items[item->offset]);

    return item_obj;
}

void TagFile_free_item(TypedPtr *item) {
    item->type_info_->free(item);
    mp_free(item);
}


