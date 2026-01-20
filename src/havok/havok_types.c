// Created by RED on 20.01.2026.

#include "havok/havok_types.h"

#include "havok/havok_codegen.h"
#include "platform/logger.h"
#include "utils/common.h"
#include "utils/hash_helper.h"

void HavokTemplateArgument_free(HavokTemplateArgument *arg) {
    String_free(&arg->name);
    arg->type_hash = 0;
    arg->number = 0;
    arg->is_number = 0;
    arg->is_class = 0;
}

void HavokRecordMember_free(HavokRecordMember *member) {
    String_free(&member->name);
    member->type_hash = 0;
    member->flags = 0;
    member->offset = 0;
}

void HavokType_init(HavokType *type) {
    String_init(&type->name, 32);
    type->hash = 0;
    type->parent_hash = 0;
    type->size = 0;
    type->align = 0;
    DA_init(&type->template_arguments, HavokTemplateArgument, 4);
    DA_init(&type->members, HavokRecordMember, 8);
}

void HavokType_free(HavokType *type) {
    String_free(&type->name);
    DA_free_with_inner(&type->template_arguments, {HavokTemplateArgument_free(it);});
    DA_free_with_inner(&type->members, {HavokRecordMember_free(it);});
}

void Havok_TypeLibrary_init(Havok_TypeLibrary *lib) {
    TracyCZoneN(ctx, "Havok_TypeLibrary_init", 1);
    DM_init(&lib->types, HavokType, 1024);
    DA_init(&lib->exported_hashes, uint64, 1024);
    DM_init(&lib->object_functions, HAVOK_ObjectMethods, 1024);
    TracyCZoneEnd(ctx);
}

void Havok_TypeLibrary_free(Havok_TypeLibrary *lib) {
    TracyCZoneN(ctx, "Havok_TypeLibrary_free", 1);
    for (int i = 0; i < lib->types.values.count; ++i) {
        HavokType *type = DA_at(&lib->types.values, i);
        HavokType_free(type);
    }
    DM_free(&lib->types);
    DM_free(&lib->object_functions);
    DA_free(&lib->exported_hashes);
    TracyCZoneEnd(ctx);
}

HavokType *Havok_TypeLibrary_find_by_name(Havok_TypeLibrary *lib, const char *name) {
    const uint64 type_hash = hash_cstring(name);
    return DM_get(&lib->types, type_hash);
}

HavokType *Havok_TypeLibrary__register_type(Havok_TypeLibrary *lib, const HKTagType *tf_type);

HavokType *Havok_TypeLibrary__register_type(Havok_TypeLibrary *lib, const HKTagType *tf_type) {
    String *full_tf_type_name = Havok_full_tag_type_name(tf_type);
    const uint64 type_hash = hash_string(full_tf_type_name);
    if (DA_contains(&lib->exported_hashes, &type_hash, compare_hashes64)) {
        String_free(full_tf_type_name);
        return DM_get(&lib->types, type_hash);
    }

    if (tf_type->parent != NULL) {
        Havok_TypeLibrary__register_type(lib, tf_type->parent);
    }


    DA_FORI(tf_type->template_args, i) {
        const HKTagTemplateArgument *tf_arg = &tf_type->template_args.items[i];
        if (tf_arg->is_class) {
            assert(tf_arg->type != NULL);
            Havok_TypeLibrary__register_type(lib, tf_arg->type);
        }
    }

    HavokType *existing_type = DM_get(&lib->types, type_hash);
    if (existing_type != NULL) {
        String *existing_full_name = Havok_full_type_name(lib, existing_type);
        if (!String_equals(existing_full_name, full_tf_type_name)) {
            GLog_Warning("Duplicate type hash for type %s/%s", String_data(full_tf_type_name),
                   String_data(existing_full_name));
            String_free(full_tf_type_name);
            String_free(existing_full_name);
            exit(1);
        }
        String_free(full_tf_type_name);
        String_free(existing_full_name);
        return existing_type;
    }
    String_free(full_tf_type_name);

    if (tf_type->data_type == HKTYPE_POINTER && tf_type->name.buffer[0] == 'T') {
        assert(tf_type->members.count==0 && "Pointer type cannot have members");
    }

    DA_FORI(tf_type->members, j) {
        const HKTagTypeMember *tf_member = &tf_type->members.items[j];
        if (tf_member->type != NULL) {
            Havok_TypeLibrary__register_type(lib, tf_member->type);
        }
    }
    bool is_enum = false;
    if (String_cequals(&tf_type->name, "hkEnum")) {
        is_enum = true;
        // Enums first template arg is it's actual type. It needs to be processed and treated as enum;
        const HKTagTemplateArgument *enum_type_arg = &tf_type->template_args.items[0];
        HavokType *inner_enum_type = Havok_TypeLibrary__register_type(lib, enum_type_arg->type);
        inner_enum_type->is_enum = true;
    }

    HavokType *new_type = DM_insert(&lib->types, type_hash);
    HavokType_init(new_type);
    String_copy_from(&new_type->name, &tf_type->name);
    new_type->hash = type_hash;
    new_type->size = tf_type->size;
    new_type->align = tf_type->align;
    new_type->is_enum = is_enum;

    if (tf_type->members.count > 0) {
        new_type->is_record = 1;
    } else if (tf_type->data_type == HKTYPE_POINTER) {
        new_type->is_ptr = 1;
    } else if (tf_type->data_type == HKTYPE_ARRAY) {
        if (String_cequals(&tf_type->name, "T[N]")) {
            new_type->is_fixed_array = 1;
        } else {
            new_type->is_array = 1;
        }
    } else {
        new_type->is_primitive = 1;
    }
    if (tf_type->parent != NULL) {
        String *parent_full_name = Havok_full_tag_type_name(tf_type->parent);
        new_type->parent_hash = hash_string(parent_full_name);
        String_free(parent_full_name);
    }
    DA_reserve(&new_type->template_arguments, tf_type->template_args.count);
    DA_FORI(tf_type->template_args, j) {
        const HKTagTemplateArgument *tf_arg = &tf_type->template_args.items[j];
        HavokTemplateArgument *new_arg = DA_append_get(&new_type->template_arguments);
        String_copy_from(&new_arg->name, &tf_arg->name);
        new_arg->is_class = tf_arg->is_class;
        new_arg->is_number = tf_arg->is_number;
        new_arg->number = tf_arg->number;
        if (tf_arg->is_class) {
            assert(tf_arg->type != NULL);
            String *arg_full_name = Havok_full_tag_type_name(tf_arg->type);
            new_arg->type_hash = hash_string(arg_full_name);
            String_free(arg_full_name);
        }
    }
    DA_FORI(tf_type->members, j) {
        const HKTagTypeMember *tf_member = &tf_type->members.items[j];
        HavokRecordMember *new_member = DA_append_get(&new_type->members);
        String_copy_from(&new_member->name, &tf_member->name);
        if (String_cequals(&new_member->name, "bool")) {
            String_from_cstr(&new_member->name, "bool__");
        }
        new_member->flags = tf_member->flags;
        new_member->offset = tf_member->offset;
        if (tf_member->type != NULL) {
            String *member_type_full_name = Havok_full_tag_type_name(tf_member->type);
            new_member->type_hash = hash_string(member_type_full_name);
            String_free(member_type_full_name);
        } else {
            GLog_Error("Member %s has NULL type", String_data(&tf_member->name));
        }
    }

    if (new_type->parent_hash != 0) {
        const HavokType *parent_type = DM_get(&lib->types, new_type->parent_hash);
        if (new_type->size == 0) {
            new_type->size = parent_type->size;
        }
        if (new_type->align == 0) {
            new_type->align = parent_type->align;
        }
    }

    DA_append(&lib->exported_hashes, &type_hash);
    return new_type;
}

void Havok_TypeLibrary_copy_from_tag_file(Havok_TypeLibrary *lib, TagFile *tf) {
    DA_FORI(tf->types, i) {
        if (i == 0)continue;
        const HKTagType *tf_type = DA_at(&tf->types, i);
        Havok_TypeLibrary__register_type(lib, tf_type);
    }
}