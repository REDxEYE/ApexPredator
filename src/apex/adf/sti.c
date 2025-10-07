// Created by RED on 19.09.2025.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/lookup3.h"
#include "apex/adf/sti.h"
#include "utils/string.h"


void STI_TypeLibrary_init(STI_TypeLibrary *lib) {
    DM_init(&lib->name_hash_to_type, uint32, 64);
    DM_init(&lib->types, STI_Type, 64);
    DM_init(&lib->object_functions, STI_ObjectMethods, 64);
    DM_init(&lib->hash_strings, String, 64);

    String tmp = {0};

    //s8 = 0x580D0A62
    STI_Type *type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_INT8,
                                              String_move(String_from_cstr(&tmp, "int8")));
    type->info.type = STI_Primitive;
    type->info.size = 1;
    type->info.alignment = 1;
    type->info.hash = STI_TYPE_HASH_INT8;
    type->info.name_id = UINT64_MAX;

    //u8 = 0x0ca2821d
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_UINT8,
                                    String_move(String_from_cstr(&tmp, "uint8")));
    type->info.type = STI_Primitive;
    type->info.size = 1;
    type->info.alignment = 1;
    type->info.hash = STI_TYPE_HASH_UINT8;
    type->info.name_id = UINT64_MAX;

    //s16 = 0xD13FCF93
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_INT16,
                                    String_move(String_from_cstr(&tmp, "int16")));
    type->info.type = STI_Primitive;
    type->info.size = 2;
    type->info.alignment = 2;
    type->info.hash = STI_TYPE_HASH_INT16;
    type->info.name_id = UINT64_MAX;

    //u16 = 0x86d152bd
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_UINT16,
                                    String_move(String_from_cstr(&tmp, "uint16")));
    type->info.type = STI_Primitive;
    type->info.size = 2;
    type->info.alignment = 2;
    type->info.hash = STI_TYPE_HASH_UINT16;
    type->info.name_id = UINT64_MAX;

    //s32 = 0x192fe633
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_INT32,
                                    String_move(String_from_cstr(&tmp, "int32")));
    type->info.type = STI_Primitive;
    type->info.size = 4;
    type->info.alignment = 4;
    type->info.hash = STI_TYPE_HASH_INT32;
    type->info.name_id = UINT64_MAX;

    //u32 = 0x075e4e4f
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_UINT32,
                                    String_move(String_from_cstr(&tmp, "uint32")));
    type->info.type = STI_Primitive;
    type->info.size = 4;
    type->info.alignment = 4;
    type->info.hash = STI_TYPE_HASH_UINT32;
    type->info.name_id = UINT64_MAX;

    //s64 = 0xAF41354F
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_INT64,
                                    String_move(String_from_cstr(&tmp, "int64")));
    type->info.type = STI_Primitive;
    type->info.size = 8;
    type->info.alignment = 8;
    type->info.hash = STI_TYPE_HASH_INT64;
    type->info.name_id = UINT64_MAX;

    //u64 = 0xA139E01F
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_UINT64,
                                    String_move(String_from_cstr(&tmp, "uint64")));
    type->info.type = STI_Primitive;
    type->info.size = 8;
    type->info.alignment = 8;
    type->info.hash = STI_TYPE_HASH_UINT64;
    type->info.name_id = UINT64_MAX;

    //f32 = 0x7515A207
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_FLOAT32,
                                    String_move(String_from_cstr(&tmp, "float32")));
    type->info.type = STI_Primitive;
    type->info.size = 4;
    type->info.alignment = 4;
    type->info.hash = STI_TYPE_HASH_FLOAT32;
    type->info.name_id = UINT64_MAX;

    //f64 = 0xC609F663
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_FLOAT64,
                                    String_move(String_from_cstr(&tmp, "float64")));
    type->info.type = STI_Primitive;
    type->info.size = 8;
    type->info.alignment = 8;
    type->info.hash = STI_TYPE_HASH_FLOAT64;
    type->info.name_id = UINT64_MAX;

    //string = 0x8955583E
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_STRING,
                                    String_move(String_from_cstr(&tmp, "String")));
    type->info.type = STI_Primitive;
    type->info.size = 16;
    type->info.alignment = 8;
    type->info.hash = STI_TYPE_HASH_STRING;
    type->info.name_id = UINT64_MAX;

    //UnkSpecial = 0xDEFE88ED
    type = STI_TypeLibrary_new_type(lib, STI_DeferredType, STI_TYPE_HASH_UNK,
                                    String_move(String_from_cstr(&tmp, "Deferred")));
    type->info.type = STI_DeferredType;
    type->info.size = 16;
    type->info.alignment = 8;
    type->info.hash = STI_TYPE_HASH_UNK;
    type->info.name_id = UINT64_MAX;

    String_free(&tmp);
}

STI_Type *STI_TypeLibrary_new_type(STI_TypeLibrary *lib, STI_MetaType meta_type, uint32 type_hash, String *name) {
    STI_Type *type = DM_insert(&lib->types, type_hash);
    uint32 *slot = NULL;
    if (type) {
        STI_Type_init(type, meta_type);
        String_move_from(&type->name, name);
        uint32 name_hash = 0;
        uint32 init_tmp = 0;
        hashlittle2(String_data(&type->name), type->name.size, &name_hash, &init_tmp);

        uint32 *other = DM_get(&lib->name_hash_to_type, name_hash);

        if (other != NULL) {
            if ((*other) == type_hash || meta_type == STI_InlineArray) {
                //same type, no problem
                return type;
            }

            printf("Warning: Name collision for type name %s (hash %08X)\n", String_data(&type->name), name_hash);
            String_append_cstr(&type->name, "_2");
            init_tmp = 0;
            hashlittle2(String_data(&type->name), type->name.size, &name_hash, &init_tmp);
            if (DM_get(&lib->name_hash_to_type, name_hash) != NULL) {
                printf("Error: Second name collision for type name %s (hash %08X)\n", String_data(&type->name),
                       name_hash);
                exit(1);
            }
            slot = DM_insert(&lib->name_hash_to_type, name_hash);
        } else {
            slot = DM_insert(&lib->name_hash_to_type, name_hash);
        }
        *slot = type_hash;

        return type;
    }
    return NULL;
}

int32 STI_TypeLibrary_types_count(const STI_TypeLibrary *lib) {
    return DM_count(&lib->types);
}

const STI_Type *STI_TypeLibrary_get_type(const STI_TypeLibrary *lib, const uint32 type_hash) {
    return DM_get(&lib->types, type_hash);
}

void STI_TypeLibrary_free(STI_TypeLibrary *lib) {
    for (int i = 0; i < DM_count(&lib->types); ++i) {
        STI_Type *type = DM_get_value(&lib->types, i);
        STI_Type_free(type);
    }

    DM_free(&lib->types);
    DM_free(&lib->object_functions);
    DM_free(&lib->name_hash_to_type);
    DA_free(&lib->exported_hashes);

    for (int i = 0; i < lib->hash_strings.keys.count; ++i) {
        String_free(&lib->hash_strings.values.items[i]);
    }
    DM_free(&lib->hash_strings);
}

void STI_Type_free(STI_Type *type) {
    String_free(&type->name);
    switch (type->type) {
        case STI_Structure: {
            DA_free_with_inner(&type->type_data.struct_data.members, {String_free(&((STI_StructMember*)it)->name);});
            break;
        }
        case STI_Enumeration: {
            for (int i = 0; i < type->type_data.enum_data.members.count; ++i) {
                String_free(&type->type_data.enum_data.members.items[i].name);
            }
            DA_free(&type->type_data.enum_data.members);
            break;
        }
        case STI_Primitive:
        case STI_DeferredType:
        case STI_Array:
        case STI_InlineArray:
        case STI_Bitfield:
        case STI_StringHash:
        case STI_Pointer: {
            break;
        }
        default: {
            printf("Unknown type %i\n", type->type);
            assert(false && "Unknown type");
        };
    }
}

void STI_Type_init(STI_Type *type, STI_MetaType meta_type) {
    type->type = meta_type;
    switch (meta_type) {
        case STI_Structure: {
            DA_init(&type->type_data.struct_data.members, STI_StructMember, 1);
            break;
        }
        case STI_Enumeration: {
            DA_init(&type->type_data.enum_data.members, STI_EnumMember, 1);
            break;
        }
        case STI_Primitive:
        case STI_Bitfield:
        case STI_Pointer:
        case STI_StringHash:
        case STI_Array:
        case STI_DeferredType:
        case STI_InlineArray: {
            break;
        }
        default: {
            printf("Unknown type %i\n", meta_type);
            assert(false && "Should not reach");
        };
    }
}
