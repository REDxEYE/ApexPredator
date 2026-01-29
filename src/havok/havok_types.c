// Created by RED on 20.01.2026.

#include "havok/havok_types.h"

#include "apex/adf/sti.h"
#include "havok/havok_codegen.h"
#include "platform/logger.h"
#include "utils/common.h"
#include "utils/hash_helper.h"

String *Havok_full_tag_type_name(const HKTagType *type) {
    String *full_name = String_new(16);
    if (String_cstarts_with(&type->name, "T*")) {
        assert(type->template_args.count==1);
        String_append_str(full_name, &type->template_args.items[0].type->name);
        String_append_cstr(full_name, "*");
        return full_name;
    }
    String_append_str(full_name, &type->name);
    DA_FORI(type->template_args, i) {
        const HKTagTemplateArgument *arg = &type->template_args.items[i];
        if (arg->is_class) {
            assert(arg->type != NULL);
            String_append_cstr(full_name, "_");
            String_append_str(full_name, &arg->type->name);
        }
        else if (arg->is_number) {
            String_append_format(full_name, "_%u", arg->number);
        }
        else {
            String_append_cstr(full_name, "_");
            String_append_str(full_name, &arg->name);
        }
    }
    return full_name;
}


void HavokRecordMember_free(HavokRecordMember *member) {
    String_free(&member->name);
    member->type_hash = 0;
    member->flags = 0;
    member->offset = 0;
}

HavokType *HavokType_init(HavokType *type) {
    String_init(&type->name, 32);
    type->hash = 0;
    type->parent_hash = 0;
    type->size = 0;
    type->align = 0;
    // DA_init(&type->template_arguments, HavokTemplateArgument, 4);
    DA_init(&type->members, HavokRecordMember, 8);
    return type;
}

uint32 HavokType_hash(const HavokType *type) {
    return type->hash;
}

void HavokType_free(HavokType *type) {
    String_free(&type->name);
    // DA_free_with_inner(&type->template_arguments, {HavokTemplateArgument_free(it);});
    DA_free_with_inner(&type->members, {HavokRecordMember_free(it);});
}

void Havok_TypeLibrary_init(Havok_TypeLibrary *lib) {
    TracyCZoneN(ctx, "Havok_TypeLibrary_init", 1);
    DM_init(&lib->types, HavokType, 1024);
    DA_init(&lib->exported_hashes, uint64, 1024);
    TracyCZoneEnd(ctx);
}

void Havok_TypeLibrary_free(Havok_TypeLibrary *lib) {
    TracyCZoneN(ctx, "Havok_TypeLibrary_free", 1);
    for (int i = 0; i < lib->types.values.count; ++i) {
        HavokType *type = DA_at(&lib->types.values, i);
        HavokType_free(type);
    }
    DM_free(&lib->types);
    DA_free(&lib->exported_hashes);
    TracyCZoneEnd(ctx);
}

HavokType *Havok_TypeLibrary_find_by_name(const Havok_TypeLibrary *lib, const char *name) {
    const uint64 type_hash = hash_cstring(name);
    return DM_get(&lib->types, type_hash);
}

HavokType *Havok_TypeLibrary__register_type(Havok_TypeLibrary *lib, HKTagType *tf_type);

HavokType *Havok_TypeLibrary__register_type(Havok_TypeLibrary *lib, HKTagType *tf_type) {
    const String *tf_name = &tf_type->name;
    const uint32 type_hash = HKTagType_hash(tf_type);
    HavokType *existing_type;
    if ((existing_type = DM_get(&lib->types, type_hash)) != NULL) {
        return existing_type;
    }

    const String *type_name = HKTagType_stable_name(tf_type);
    HavokType new_type = {0};
    HavokType_init(&new_type);
    String_copy_from(&new_type.name, type_name);
    if (String_cstarts_with(tf_name, "hkArray")) {
        new_type.type = HK_ARRAY;
        const HavokType *inner_type = Havok_TypeLibrary__register_type(lib, tf_type->template_args.items[0].type);
        new_type.inner_type_hash = HavokType_hash(inner_type);
    }
    else if (String_cequals(tf_name, "hkStringPtr")) {
        new_type.type = HK_STRING;
    }
    else if (tf_type->members.count > 0 || tf_type->data_type == HKTYPE_RECORD) {
        new_type.type = HK_RECORD;
        DA_reserve(&new_type.members, tf_type->members.count);
    }
    else if ((String_cequals(&new_type.name, "voidPtr") && tf_type->data_type == HKTYPE_POINTER)) {
        new_type.type = HK_BASIC;
    }
    else if (String_cequals(tf_name, "T*")) {
        new_type.type = HK_PTR;
        const HavokType *inner_type = Havok_TypeLibrary__register_type(lib, tf_type->template_args.items[0].type);
        new_type.inner_type_hash = HavokType_hash(inner_type);
    }
    else if (String_cstarts_with(tf_name, "T[N]")) {
        new_type.type = HK_FIXED_ARRAY;
        const HavokType *inner_type = Havok_TypeLibrary__register_type(lib, tf_type->template_args.items[0].type);
        new_type.inner_type_hash = HavokType_hash(inner_type);
        new_type.array_size = tf_type->template_args.items[1].number;
    }
    else if (String_cequals(tf_name, "const_char*")) {
        new_type.type = HK_BASIC;
    }
    else if (String_cequals(tf_name, "hkEnum")) {
        HKTagType *tag_inner_type = tf_type->template_args.items[0].type;
        HKTagType *tag_storage_type = tf_type->template_args.items[1].type;
        const HavokType *storage_type = Havok_TypeLibrary__register_type(lib, tag_storage_type);
        const uint32 storage_hash = HavokType_hash(storage_type);
        HavokType *inner_type = Havok_TypeLibrary__register_type(lib, tag_inner_type);
        if (inner_type == NULL || storage_type == NULL) {
            GLog_Error("Failed to register hkEnum inner or storage type for %s", String_cstr(type_name));
            return NULL;
        }
        if (inner_type->type == HK_BASIC && !String_cequals(&inner_type->name, "void")) {
            inner_type->type = HK_PRIMITIVE;
            inner_type->parent_hash = storage_hash;
        }
        new_type.type = HK_ENUM;
    }
    else if (String_cequals(tf_name, "hkFlags")) {
        HKTagType *tag_inner_type = tf_type->template_args.items[0].type;
        HKTagType *tag_storage_type = tf_type->template_args.items[1].type;
        const HavokType *storage_type = Havok_TypeLibrary__register_type(lib, tag_storage_type);
        const uint32 storage_hash = HavokType_hash(storage_type);
        HavokType *inner_type = Havok_TypeLibrary__register_type(lib, tag_inner_type);
        if (inner_type == NULL || storage_type == NULL) {
            GLog_Error("Failed to register hkFlags inner or storage type for %s", String_cstr(type_name));
            return NULL;
        }
        if (inner_type->type == HK_BASIC && !String_cequals(&inner_type->name, "void")) {
            inner_type->type = HK_PRIMITIVE;
            inner_type->parent_hash = storage_hash;
        }
        new_type.type = HK_ENUM;
    }
    else if (tf_type->data_type == HKTYPE_BASIC ||
             tf_type->data_type == HKTYPE_OPAQUE ||
             tf_type->data_type == HKTYPE_FLOAT ||
             String_cequals(&tf_type->name, "hkVector4f") ||
             String_cequals(&tf_type->name, "hkRotationImpl") ||
             (String_cequals(tf_name, "void") && tf_type->data_type == HKTYPE_PRIMITIVE)
    ) {
        new_type.type = HK_BASIC;
    }
    else if (tf_type->data_type == HKTYPE_PRIMITIVE) {
        new_type.type = HK_PRIMITIVE;
    }
    else if (tf_type->data_type == HKTYPE_OPAQUE) {
        new_type.type = HK_BASIC;
    }
    else {
        printf("Unhandled case -> %s  [%s]\n", String_cstr(tf_name), HKTAGTYPE_NAMES[tf_type->data_type]);
        return NULL;
    }
    new_type.hash = type_hash;
    new_type.size = tf_type->size;
    new_type.align = tf_type->align;
    new_type.hash = type_hash;
    if (tf_type->parent != NULL) {
        const HavokType *parent_type = Havok_TypeLibrary__register_type(lib, tf_type->parent);
        if (parent_type != NULL) {
            new_type.parent_hash = HavokType_hash(parent_type);
            if (new_type.size == 0) {
                new_type.size = parent_type->size;
            }
            if (new_type.align == 0) {
                new_type.align = parent_type->align;
            }
        }
        else {
            GLog_Error("Failed to register parent type for %s", String_cstr(&new_type.name));
            return NULL;
        }
    }
    if (String_cequals(tf_name, "hkEnum")) {
        HKTagType *tag_storage_type = tf_type->template_args.items[1].type;
        const HavokType *storage_type = Havok_TypeLibrary__register_type(lib, tag_storage_type);
        new_type.parent_hash = storage_type->hash;
    }
    else if (String_cequals(tf_name, "hkFlags")) {
        HKTagType *tag_storage_type = tf_type->template_args.items[1].type;
        const HavokType *storage_type = Havok_TypeLibrary__register_type(lib, tag_storage_type);
        new_type.parent_hash = storage_type->hash;
    }

    // printf("Registering type -> %s [%s]\n", String_data(&new_type.name), HavokTypeMetaTypeNames[new_type.type]);

    HavokType *slot = DM_insert(&lib->types, type_hash);
    *slot = new_type;

    if (tf_type->members.count > 0 || tf_type->data_type == HKTYPE_RECORD) {
        DA_FORI(tf_type->members, i) {
            const HKTagTypeMember *tf_member = &tf_type->members.items[i];
            Havok_TypeLibrary__register_type(lib, tf_member->type);
        }
        HavokType *tmp_slot = DM_insert(&lib->types, type_hash);
        DA_FORI(tf_type->members, i) {
            const HKTagTypeMember *tf_member = &tf_type->members.items[i];
            HavokRecordMember new_member = {0};
            if (String_cequals(&tf_member->name, "bool")) {
                String_init(&new_member.name, 8);
                String_from_cstr(&new_member.name, "_bool");
            }
            else {
                String_copy_from(&new_member.name, &tf_member->name);
            }
            const HavokType *member_type = Havok_TypeLibrary__register_type(lib, tf_member->type);
            new_member.type_hash = HavokType_hash(member_type);
            new_member.offset = tf_member->offset;
            new_member.flags = tf_member->flags;
            DA_append(&tmp_slot->members, &new_member);
        }
    }

    return slot;
}

void Havok_TypeLibrary_copy_from_tag_file(Havok_TypeLibrary *lib, TagFile *tf) {
    DA_FORI(tf->types, i) {
        if (i == 0)continue;
        HKTagType *tf_type = DA_at(&tf->types, i);
        Havok_TypeLibrary__register_type(lib, tf_type);
    }
}

void register_type_info(TypeInfoMap *map, const uint32 hash, const HavokTypeInfo *type_info) {
    const HavokTypeInfo **slot = DM_insert(map, hash);
    *slot = type_info;
}
