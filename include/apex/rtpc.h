// Created by RED on 03.10.2025.

#ifndef APEXPREDATOR_RTPC_H
#define APEXPREDATOR_RTPC_H
#include "int_def.h"
#include "platform/common_arrays.h"
#include "utils/hash_helper.h"
#include "utils/dynamic_array.h"
#include "utils/dynamic_map.h"
#include "utils/buffer/buffer.h"

#define RTPC_MAGIC "RTPC"

typedef struct RuntimeNode RuntimeNode;

typedef enum {
    PROP_TYPE_NONE = 0,
    PROP_TYPE_U32 = 1,
    PROP_TYPE_F32 = 2,
    PROP_TYPE_STR = 3,
    PROP_TYPE_VEC2 = 4,
    PROP_TYPE_VEC3 = 5,
    PROP_TYPE_VEC4 = 6,
    PROP_TYPE_MAT3X3 = 7,
    PROP_TYPE_MAT4X4 = 8,
    PROP_TYPE_ARRAY_U32 = 9,
    PROP_TYPE_ARRAY_F32 = 10,
    PROP_TYPE_ARRAY_U8 = 11,
    PROP_TYPE_DEPRECIATED_12 = 12,
    PROP_TYPE_OBJID = 13,
    PROP_TYPE_EVENT = 14,
    PROP_TYPE_UNK_15 = 15,
    PROP_TYPE_UNK_16 = 16,
} PropType;

typedef struct RuntimeEvent{
    uint32 a;
    uint32 b;
}RuntimeEvent;

DYNAMIC_ARRAY_STRUCT(RuntimeEvent,RuntimeEvent);

typedef struct RuntimeProp {
    // String name;
    uint32 name_hash;
    PropType type;

    struct {
        union {
            uint32 uint32_value;
            float32 float32_value;
            String string_value;
            float32 vec2_value[2];
            float32 vec3_value[3];
            float32 vec4_value[4];
            float32 matrix33_value[9];
            float32 matrix44_value[16];
            DynamicArray_uint32 uint32_array_value;
            DynamicArray_float32 float32_array_value;
            DynamicArray_uint8 uint8_array_value;
            uint64 objid_value;
            DynamicArray_RuntimeEvent event_value;
            uint64 unk15_value;
            uint64 unk16_value;
        };
    }value;
} RuntimeProp;

DYNAMIC_ARRAY_STRUCT(RuntimeProp, RuntimeProp);

DYNAMIC_INT_MAP_STRUCT(RuntimeProp, RuntimeProp);

DYNAMIC_ARRAY_STRUCT(RuntimeNode, RuntimeNode);

DYNAMIC_INT_MAP_STRUCT(RuntimeNode, RuntimeNode);

typedef struct RuntimeNode {
    uint32 name_hash;

    DynamicArray_RuntimeProp props;
    DynamicArray_RuntimeNode children;

    uint32 heap_allocated:1;
} RuntimeNode;

RuntimeNode *RuntimeContainer_from_buffer(Buffer *buffer);

RuntimeNode *RuntimeNode_new(void);

void RuntimeNode_init(RuntimeNode *node, uint32 prop_count, uint32 children_count);
RuntimeProp* RuntimeNode_get_prop(const RuntimeNode* node, const char* name);
RuntimeProp* RuntimeNode_get_prop_by_hash(const RuntimeNode* node, uint32 hash);

// Shortcut getters for RuntimeProp values. All return NULL when prop not found. Please use RuntimeNode_has_prop to check if prop exist
uint32 RuntimeNode_get_prop_u32(const RuntimeNode* node, const char* name);
float32 RuntimeNode_get_prop_f32(const RuntimeNode* node, const char* name);
StringView RuntimeNode_get_prop_str(const RuntimeNode* node, const char* name);
float32* RuntimeNode_get_prop_vec2(const RuntimeNode* node, const char* name);
float32* RuntimeNode_get_prop_vec3(const RuntimeNode* node, const char* name);
float32* RuntimeNode_get_prop_vec4(const RuntimeNode* node, const char* name);
float32* RuntimeNode_get_prop_mat3x3(const RuntimeNode* node, const char* name);
float32* RuntimeNode_get_prop_mat4x4(const RuntimeNode* node, const char* name);
DynamicArray_uint32* RuntimeNode_get_prop_array_u32(const RuntimeNode* node, const char* name);
DynamicArray_float32* RuntimeNode_get_prop_array_f32(const RuntimeNode* node, const char* name);
DynamicArray_uint8* RuntimeNode_get_prop_array_u8(const RuntimeNode* node, const char* name);
uint64 RuntimeNode_get_prop_objid(const RuntimeNode* node, const char* name);
DynamicArray_RuntimeEvent* RuntimeNode_get_prop_event(const RuntimeNode* node, const char* name);

uint32 RuntimeNode_get_prop_by_hash_u32(const RuntimeNode* node, uint32 hash);
float32 RuntimeNode_get_prop_by_hash_f32(const RuntimeNode* node, uint32 hash);
StringView RuntimeNode_get_prop_by_hash_str(const RuntimeNode* node, uint32 hash);
float32* RuntimeNode_get_prop_by_hash_vec2(const RuntimeNode* node, uint32 hash);
float32* RuntimeNode_get_prop_by_hash_vec3(const RuntimeNode* node, uint32 hash);
float32* RuntimeNode_get_prop_by_hash_vec4(const RuntimeNode* node, uint32 hash);
float32* RuntimeNode_get_prop_by_hash_mat3x3(const RuntimeNode* node, uint32 hash);
float32* RuntimeNode_get_prop_by_hash_mat4x4(const RuntimeNode* node, uint32 hash);
DynamicArray_uint32* RuntimeNode_get_prop_by_hash_array_u32(const RuntimeNode* node, uint32 hash);
DynamicArray_float32* RuntimeNode_get_prop_by_hash_array_f32(const RuntimeNode* node, uint32 hash);
DynamicArray_uint8* RuntimeNode_get_prop_by_hash_array_u8(const RuntimeNode* node, uint32 hash);
uint64 RuntimeNode_get_prop_by_hash_objid(const RuntimeNode* node, uint32 hash);
DynamicArray_RuntimeEvent* RuntimeNode_get_prop_by_hash_event(const RuntimeNode* node, uint32 hash);
bool RuntimeNode_has_prop(const RuntimeNode *node, const char *name);


void RuntimeNode_print(const RuntimeNode* node, FILE* output, uint32 indent);
void RuntimeNode_emit_json(const RuntimeNode *node, String *out, uint32 indent);

void RuntimeNode_free(RuntimeNode *node);

void RuntimeProp_init(RuntimeProp *prop, PropType type);

void RuntimeProp_print(const RuntimeProp* prop, FILE* output, uint32 indent);
void RuntimeProp_emit_json(const RuntimeProp *prop, String *out, uint32 indent);

void RuntimeProp_free(RuntimeProp* prop);

#endif //APEXPREDATOR_RTPC_H
