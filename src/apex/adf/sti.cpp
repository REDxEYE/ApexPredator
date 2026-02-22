// Created by RED on 19.09.2025.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/lookup3.h"
#include "apex/adf/sti.h"

#include "platform/logger.h"
#include "utils/hash_helper.h"
#include "utils/string.h"


void STI_TypeLibrary_init(STI_TypeLibrary *lib) {
    TracyCZoneN(ctx, "STI_TypeLibrary_init", 1);
    DM_init(&lib->already_seen_name_hashes, uint32, 64);
    DM_init(&lib->types, STI_Type, 64);
    String tmp = {};

    //s8 = 0x580D0A62
    STI_Type *type = (STI_Type *)DM_insert(&lib->types, STI_TYPE_HASH_INT8);
    STI_Type_init(type, STI_Primitive, STI_TYPE_HASH_INT8, String_from_cstr(&tmp, "int8"));
    type->size = 1;
    type->alignment = 1;

    //u8 = 0x0ca2821d
    type = (STI_Type *)DM_insert(&lib->types, STI_TYPE_HASH_UINT8);
    STI_Type_init(type, STI_Primitive, STI_TYPE_HASH_UINT8, String_from_cstr(&tmp, "uint8"));
    type->size = 1;
    type->alignment = 1;

    //s16 = 0xD13FCF93
    type = (STI_Type *)DM_insert(&lib->types, STI_TYPE_HASH_INT16);
    STI_Type_init(type, STI_Primitive, STI_TYPE_HASH_INT16, String_from_cstr(&tmp, "int16"));
    type->size = 2;
    type->alignment = 2;

    //u16 = 0x86d152bd
    type = (STI_Type *)DM_insert(&lib->types, STI_TYPE_HASH_UINT16);
    STI_Type_init(type, STI_Primitive, STI_TYPE_HASH_UINT16, String_from_cstr(&tmp, "uint16"));
    type->size = 2;
    type->alignment = 2;

    //s32 = 0x192fe633
    type = (STI_Type *)DM_insert(&lib->types, STI_TYPE_HASH_INT32);
    STI_Type_init(type, STI_Primitive, STI_TYPE_HASH_INT32, String_from_cstr(&tmp, "int32"));
    type->size = 4;
    type->alignment = 4;

    //u32 = 0x075e4e4f
    type = (STI_Type *)DM_insert(&lib->types, STI_TYPE_HASH_UINT32);
    STI_Type_init(type, STI_Primitive, STI_TYPE_HASH_UINT32, String_from_cstr(&tmp, "uint32"));
    type->size = 4;
    type->alignment = 4;

    //s64 = 0xAF41354F
    type = (STI_Type *)DM_insert(&lib->types, STI_TYPE_HASH_INT64);
    STI_Type_init(type, STI_Primitive, STI_TYPE_HASH_INT64, String_from_cstr(&tmp, "int64"));
    type->size = 8;
    type->alignment = 8;

    //u64 = 0xA139E01F
    type = (STI_Type *)DM_insert(&lib->types, STI_TYPE_HASH_UINT64);
    STI_Type_init(type, STI_Primitive, STI_TYPE_HASH_UINT64, String_from_cstr(&tmp, "uint64"));
    type->size = 8;
    type->alignment = 8;

    //f32 = 0x7515A207
    type = (STI_Type *)DM_insert(&lib->types, STI_TYPE_HASH_FLOAT32);
    STI_Type_init(type, STI_Primitive, STI_TYPE_HASH_FLOAT32, String_from_cstr(&tmp, "float32"));
    type->size = 4;
    type->alignment = 4;

    //f64 = 0xC609F663
    type = (STI_Type *)DM_insert(&lib->types, STI_TYPE_HASH_FLOAT64);
    STI_Type_init(type, STI_Primitive, STI_TYPE_HASH_FLOAT64, String_from_cstr(&tmp, "float64"));
    type->size = 8;
    type->alignment = 8;

    //string = 0x8955583E
    type = (STI_Type *)DM_insert(&lib->types, STI_TYPE_HASH_STRING);
    STI_Type_init(type, STI_StringType, STI_TYPE_HASH_STRING, String_from_cstr(&tmp, "String"));
    type->size = 8;
    type->alignment = 8;

    //Deferred = 0xDEFE88ED
    type = (STI_Type *)DM_insert(&lib->types, STI_TYPE_HASH_DEFERRED);
    STI_Type_init(type, STI_DeferredType, STI_TYPE_HASH_DEFERRED, String_from_cstr(&tmp, "Deferred"));
    type->size = 16;
    type->alignment = 8;

    String_free(&tmp);
    TracyCZoneEnd(ctx);
}

STI_Type *STI_TypeLibrary_register_adf_type(STI_TypeLibrary *lib, const ADF *adf, const ADFType *adf_type) {
    const uint32 adf_type_hash = adf_type->def.hash;
    String *type_name = String_new_from_str(&adf->strings.items[adf_type->def.name_id]);
    uint32 name_hash = hash_string(type_name);

    STI_Type *type = (STI_Type *)DM_get(&lib->types, adf_type_hash);
    if (type != NULL) {
        return type;
    }


    if (adf_type->def.type == ADF_Bitfield) {
        String bitfield_name = {};
        const uint32 pos = String_find_chr(type_name, ':');
        String_from_cstr2(&bitfield_name, String_cstr(type_name), pos != -1 ? pos : String_size(type_name));
        name_hash = hash_string(&bitfield_name);
        String_move_from(type_name, &bitfield_name);
    }
    else if (adf_type->def.type == ADF_Array) {
        String array_name = {};
        String_from_cstr(&array_name, "Array_");
        const STI_Type *inner_type = (const STI_Type *)DM_get(&lib->types, adf_type->def.element_type_hash);
        if (inner_type == NULL) {
            String_append_cstr2(&array_name, String_cstr(type_name) + 2, String_size(type_name) - 3);
        }
        else {
            String_append_str(&array_name, &inner_type->name);
        }

        name_hash = hash_string(&array_name);
        String_move_from(type_name, &array_name);
    }
    else if (adf_type->def.type == ADF_InlineArray) {
        String inline_array_name = {};
        const STI_Type *inner_type = (const STI_Type *)DM_get(&lib->types, adf_type->def.element_type_hash);
        if (inner_type == NULL) {
            String_from_cstr2(&inline_array_name, String_cstr(type_name) + 2, String_size(type_name) - 3);
        }
        else {
            String_copy_from(&inline_array_name, &inner_type->name);
        }
        name_hash = hash_string(&inline_array_name);
        String_move_from(type_name, &inline_array_name);
    }else if (adf_type->def.type==ADF_StringType) {
        String_prepend_format(type_name, "STI_");
        name_hash = hash_string(type_name);
    }

    type = (STI_Type *)DM_insert(&lib->types, adf_type_hash);

    const uint32 *other_adf_hash = NULL;
    if ((other_adf_hash = (const uint32 *)DM_get(&lib->already_seen_name_hashes, name_hash)) != NULL) {
        const STI_Type* other_type = (const STI_Type *)DM_get(&lib->types, *other_adf_hash);
        if (*other_adf_hash != adf_type_hash) {
            if (other_type->data.array_data.type_hash == adf_type->def.element_type_hash && adf_type->def.type == ADF_Array) {
                STI_Type_init(type, STI_Alias, adf_type_hash, type_name);
                type->data.deferred_data.type_hash = *other_adf_hash;

                *(uint32 *) DM_insert(&lib->already_seen_name_hashes, name_hash) = adf_type_hash;
                String_free(type_name);
                return type;
            }
            if (adf_type->def.type != ADF_InlineArray && adf_type->def.type != ADF_Bitfield) {
                GLog_Warning("Type name collision for type name %s", String_cstr(type_name));
                String_append_format(type_name, "_%08X", adf_type_hash);
                name_hash = hash_string(type_name);
            }
        }
    }


    // if (other_adf_hash!=NULL && adf_type->def.type == ADF_Array) {
    //     const STI_Type* other_type = DM_get(&lib->types, *other_adf_hash);
    //     if (other_type->data.array_data.type_hash == adf_type->def.element_type_hash) {
    //         STI_Type_init(type, STI_Alias, adf_type_hash, type_name);
    //         type->data.deferred_data.type_hash = *other_adf_hash;
    //
    //         *(uint32 *) DM_insert(&lib->already_seen_name_hashes, name_hash) = adf_type_hash;
    //         return type;
    //     }
    // }

    STI_Type_init(type, (STI_DataType) adf_type->def.type, adf_type_hash, type_name);

    *(uint32 *) DM_insert(&lib->already_seen_name_hashes, name_hash) = adf_type_hash;

    type->size = adf_type->def.size;
    type->alignment = adf_type->def.alignment;

    switch ((STI_DataType) adf_type->def.type) {
        case STI_Structure: {
            DA_init(&type->data.struct_data.members, STI_StructMember, adf_type->type_data.struct_data.members.count);
            for (int i = 0; i < adf_type->type_data.struct_data.members.count; ++i) {
                const ADFStructMemberInfo *adf_member = &adf_type->type_data.struct_data.members.items[i];
                STI_StructMember *member = (STI_StructMember *)DA_append_get(&type->data.struct_data.members);
                String_copy_from(&member->name, &adf->strings.items[adf_member->name_id]);
                if (String_size(&member->name) == 0) {
                    String_format(&member->name, "m%i", i);
                }
                member->offset = adf_member->offset;
                member->size = adf_member->size;
                member->bit_offset = adf_member->bit_offset;
                member->type_hash = adf_member->type_hash;
            }
            break;
        }
        case STI_Enumeration: {
            DA_init(&type->data.enum_data.members, STI_EnumMember, adf_type->type_data.enum_data.members.count);
            for (int i = 0; i < adf_type->type_data.enum_data.members.count; ++i) {
                const ADFEnumMemberInfo *adf_member = &adf_type->type_data.enum_data.members.items[i];
                STI_EnumMember *member = (STI_EnumMember *)DA_append_get(&type->data.enum_data.members);
                String_copy_from(&member->name, &adf->strings.items[adf_member->name_id]);
                member->value = adf_member->value;
            }
            break;
        }
        case STI_Pointer: {
            type->data.deferred_data.type_hash = adf_type->def.element_type_hash;
            break;
        }
        case STI_Array: {
            type->data.array_data.type_hash = adf_type->def.element_type_hash;
            break;
        }
        case STI_InlineArray: {
            type->data.array_data.type_hash = adf_type->def.element_type_hash;
            type->data.array_data.count = adf_type->def.element_len;
            break;
        }
        // case STI_StringType: {
        //     break;
        // }
        // case STI_Recursive: {
        //     break;
        // }
        case STI_Bitfield: {
            type->data.deferred_data.type_hash = adf_type->type_data.deferred_data.type_hash;
            type->data.bits_data = adf_type->def.element_len;
            break;
        }
        case STI_StringHash: {
            type->data.deferred_data.type_hash = adf_type->type_data.deferred_data.type_hash;
            break;
        }
        // case STI_DeferredType: {
        //     break;
        // }
        default: {
            GLog_Error("Unsupported ADF type %i for type %s", adf_type->def.type, String_cstr(type_name));
            abort();
        }
    }
    String_free(type_name);
    return NULL;
}

int32 STI_TypeLibrary_types_count(const STI_TypeLibrary *lib) {
    return DM_count(&lib->types);
}

const STI_Type *STI_TypeLibrary_get_type(const STI_TypeLibrary *lib, const uint32 type_hash) {
    return (STI_Type *)DM_get(&lib->types, type_hash);
}

void STI_TypeLibrary_free(STI_TypeLibrary *lib) {
    TracyCZoneN(ctx, "STI_TypeLibrary_free", 1);
    for (int i = 0; i < DM_count(&lib->types); ++i) {
        STI_Type *type = (STI_Type *)DM_get_value(&lib->types, i);
        STI_Type_free(type);
    }

    DM_free(&lib->types);
    // DM_free(&lib->object_functions);
    DM_free(&lib->already_seen_name_hashes);
    DA_free(&lib->exported_hashes);
    TracyCZoneEnd(ctx);
}

void STI_Type_free(STI_Type *type) {
    String_free(&type->name);
    switch (type->type) {
        case STI_Structure: {
            DA_free_with_inner(&type->data.struct_data.members, {String_free(&((STI_StructMember*)it)->name);});
            break;
        }
        case STI_Enumeration: {
            DA_free_with_inner(&type->data.enum_data.members, {String_free(&((STI_EnumMember*)it)->name);});
            break;
        }
        case STI_Primitive:
        case STI_DeferredType:
        case STI_Array:
        case STI_InlineArray:
        case STI_Bitfield:
        case STI_StringHash:
        case STI_Alias:
        case STI_StringType:
        case STI_Pointer: {
            break;
        }
        default: {
            GLog_Error("Unknown type %i", type->type);
            assert(false && "Unknown type");
        };
    }
}

void STI_Type_init(STI_Type *type, const STI_DataType meta_type, uint32 hash, const String *name) {
    String_copy_from(&type->name, name);
    type->type = meta_type;
    type->hash = hash;
    switch (meta_type) {
        case STI_Structure: {
            DA_init(&type->data.struct_data.members, STI_StructMember, 1);
            break;
        }
        case STI_Enumeration: {
            DA_init(&type->data.enum_data.members, STI_EnumMember, 1);
            break;
        }
        case STI_Primitive:
        case STI_Bitfield:
        case STI_Pointer:
        case STI_StringHash:
        case STI_Array:
        case STI_DeferredType:
        case STI_Alias:
        case STI_StringType:
        case STI_InlineArray: {
            break;
        }
        default: {
            GLog_Error("Unknown type %i", meta_type);
            assert(false && "Should not reach");
        };
    }
}
