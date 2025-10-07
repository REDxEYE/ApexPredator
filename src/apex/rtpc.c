// Created by RED on 03.10.2025.
#include <assert.h>

#include "apex/rtpc.h"

#include "apex/hashes.h"

#pragma pack(push, 1)
typedef struct {
    char ident[4];
    uint32 version;
} RTPCHeader;

typedef struct {
    uint32 name_hash;
    uint32 data_offset;
    uint16 prop_count;
    uint16 child_count;
} RuntimeNodeHeader;

typedef struct {
    uint32 name_hash;

    union {
        uint32 uint_value;
        float32 float_value;
    } data_raw;

    uint8 prop_type;
} RuntimePropHeader;
#pragma pack(pop)

void RuntimeNode__from_buffer(RuntimeNode *node, Buffer *buffer);

void RuntimeProp__from_buffer(RuntimeProp *prop, Buffer *buffer);

RuntimeNode *RuntimeContainer_from_buffer(Buffer *buffer) {
    RTPCHeader header;
    if (buffer->read(buffer, &header, sizeof(RTPCHeader), NULL) != BUFFER_SUCCESS) {
        printf("[ERROR]: Failed to read RTPC header\n");
        exit(1);
    }
    if (header.version != 3) {
        printf("[ERROR]: Unsupported RTPC version: %u\n", header.version);
        exit(1);
    }

    RuntimeNode *root_node = RuntimeNode_new(NULL);
    RuntimeNode__from_buffer(root_node, buffer);

    return root_node;
}

RuntimeNode *RuntimeNode_new(String *name) {
    RuntimeNode *node = malloc(sizeof(RuntimeNode));
    if (node == NULL) {
        printf("[ERROR]: Failed to allocate memory\n");
        exit(1);
    }
    memset(node, 0, sizeof(RuntimeNode));
    RuntimeNode_init(node, name);
    node->heap_allocated = 1;
    return node;
}

void RuntimeNode_init(RuntimeNode *node, String *name) {
    if (name != NULL) {
        String_move_from(&node->name, name);
    } else {
        String_init(&node->name, 64);
    }
    DA_init(&node->props, RuntimeProp, 4);
    DA_init(&node->children, RuntimeNode, 4);
}

RuntimeProp *RuntimeNode_get_prop(RuntimeNode *node, const char *name) {
    uint32 hash = hash_cstring(name);
    return RuntimeNode_get_prop_by_hash(node, hash);
}

RuntimeProp *RuntimeNode_get_prop_by_hash(RuntimeNode *node, uint32 hash) {
    for (uint32 i = 0; i < node->props.count; ++i) {
        RuntimeProp *prop = DA_at(&node->props, i);
        if (prop->name_hash == hash) {
            return prop;
        }
    }
    return NULL;
}

uint32 RuntimeNode_get_prop_by_hash_u32(RuntimeNode *node, uint32 hash) {
    RuntimeProp *prop = RuntimeNode_get_prop_by_hash(node, hash);
    if (prop == NULL || prop->type != PROP_TYPE_U32) {
        return 0;
    }
    return prop->value.uint32_value;
}

float32 RuntimeNode_get_prop_by_hash_f32(RuntimeNode *node, uint32 hash) {
    RuntimeProp *prop = RuntimeNode_get_prop_by_hash(node, hash);
    if (prop == NULL || prop->type != PROP_TYPE_F32) {
        return 0.0f;
    }
    return prop->value.float32_value;
}

String *RuntimeNode_get_prop_by_hash_str(RuntimeNode *node, uint32 hash) {
    RuntimeProp *prop = RuntimeNode_get_prop_by_hash(node, hash);
    if (prop == NULL || prop->type != PROP_TYPE_STR) {
        return NULL;
    }
    return &prop->value.string_value;
}

float32 *RuntimeNode_get_prop_by_hash_vec2(RuntimeNode *node, uint32 hash) {
    RuntimeProp *prop = RuntimeNode_get_prop_by_hash(node, hash);
    if (prop == NULL || prop->type != PROP_TYPE_VEC2) {
        return NULL;
    }
    return prop->value.vec2_value;
}

float32 *RuntimeNode_get_prop_by_hash_vec3(RuntimeNode *node, uint32 hash) {
    RuntimeProp *prop = RuntimeNode_get_prop_by_hash(node, hash);
    if (prop == NULL || prop->type != PROP_TYPE_VEC3) {
        return NULL;
    }
    return prop->value.vec3_value;
}

float32 *RuntimeNode_get_prop_by_hash_vec4(RuntimeNode *node, uint32 hash) {
    RuntimeProp *prop = RuntimeNode_get_prop_by_hash(node, hash);
    if (prop == NULL || prop->type != PROP_TYPE_VEC4) {
        return NULL;
    }
    return prop->value.vec4_value;
}

float32 * RuntimeNode_get_prop_by_hash_mat3x3(RuntimeNode *node, uint32 hash) {
    RuntimeProp *prop = RuntimeNode_get_prop_by_hash(node, hash);
    if (prop == NULL || prop->type != PROP_TYPE_MAT3X3) {
        return NULL;
    }
    return prop->value.matrix33_value;
}

float32 * RuntimeNode_get_prop_by_hash_mat4x4(RuntimeNode *node, uint32 hash) {
    RuntimeProp *prop = RuntimeNode_get_prop_by_hash(node, hash);
    if (prop == NULL || prop->type != PROP_TYPE_MAT4X4) {
        return NULL;
    }
    return prop->value.matrix44_value;
}

DynamicArray_uint32 * RuntimeNode_get_prop_by_hash_array_u32(RuntimeNode *node, uint32 hash) {
    RuntimeProp *prop = RuntimeNode_get_prop_by_hash(node, hash);
    if (prop == NULL || prop->type != PROP_TYPE_ARRAY_U32) {
        return NULL;
    }
    return &prop->value.uint32_array_value;
}

DynamicArray_float32 * RuntimeNode_get_prop_by_hash_array_f32(RuntimeNode *node, uint32 hash) {
    RuntimeProp *prop = RuntimeNode_get_prop_by_hash(node, hash);
    if (prop == NULL || prop->type != PROP_TYPE_ARRAY_F32) {
        return NULL;
    }
    return &prop->value.float32_array_value;
}

DynamicArray_uint8 * RuntimeNode_get_prop_by_hash_array_u8(RuntimeNode *node, uint32 hash) {
    RuntimeProp *prop = RuntimeNode_get_prop_by_hash(node, hash);
    if (prop == NULL || prop->type != PROP_TYPE_ARRAY_U8) {
        return NULL;
    }
    return &prop->value.uint8_array_value;
}

uint64 RuntimeNode_get_prop_by_hash_objid(RuntimeNode *node, uint32 hash) {
    RuntimeProp *prop = RuntimeNode_get_prop_by_hash(node, hash);
    if (prop == NULL || prop->type != PROP_TYPE_OBJID) {
        return 0;
    }
    return prop->value.objid_value;
}

DynamicArray_uint64 * RuntimeNode_get_prop_by_hash_event(RuntimeNode *node, uint32 hash) {
    RuntimeProp *prop = RuntimeNode_get_prop_by_hash(node, hash);
    if (prop == NULL || prop->type != PROP_TYPE_EVENT) {
        return NULL;
    }
    return &prop->value.event_value;
}

uint32 RuntimeNode_get_prop_u32(RuntimeNode *node, const char* name){
    return RuntimeNode_get_prop_by_hash_u32(node, hash_cstring(name));
}
float32 RuntimeNode_get_prop_f32(RuntimeNode *node, const char* name){
    return RuntimeNode_get_prop_by_hash_f32(node, hash_cstring(name));
}
String *RuntimeNode_get_prop_str(RuntimeNode *node, const char* name){
    return RuntimeNode_get_prop_by_hash_str(node, hash_cstring(name));
}
float32 *RuntimeNode_get_prop_vec2(RuntimeNode *node, const char* name){
    return RuntimeNode_get_prop_by_hash_vec2(node, hash_cstring(name));
}
float32 *RuntimeNode_get_prop_vec3(RuntimeNode *node, const char* name){
    return RuntimeNode_get_prop_by_hash_vec3(node, hash_cstring(name));
}
float32 *RuntimeNode_get_prop_vec4(RuntimeNode *node, const char* name){
    return RuntimeNode_get_prop_by_hash_vec4(node, hash_cstring(name));
}
float32 * RuntimeNode_get_prop_mat3x3(RuntimeNode *node, const char* name){
    return RuntimeNode_get_prop_by_hash_mat3x3(node, hash_cstring(name));
}
float32 * RuntimeNode_get_prop_mat4x4(RuntimeNode *node, const char* name){
    return RuntimeNode_get_prop_by_hash_mat4x4(node, hash_cstring(name));
}
DynamicArray_uint32 * RuntimeNode_get_prop_array_u32(RuntimeNode *node, const char* name){
    return RuntimeNode_get_prop_by_hash_array_u32(node, hash_cstring(name));
}
DynamicArray_float32 * RuntimeNode_get_prop_array_f32(RuntimeNode *node, const char* name){
    return RuntimeNode_get_prop_by_hash_array_f32(node, hash_cstring(name));
}
DynamicArray_uint8 * RuntimeNode_get_prop_array_u8(RuntimeNode *node, const char* name){
    return RuntimeNode_get_prop_by_hash_array_u8(node, hash_cstring(name));
}
uint64 RuntimeNode_get_prop_objid(RuntimeNode *node, const char* name){
    return RuntimeNode_get_prop_by_hash_objid(node, hash_cstring(name));
}
DynamicArray_uint64 * RuntimeNode_get_prop_event(RuntimeNode *node, const char* name){
    return RuntimeNode_get_prop_by_hash_event(node, hash_cstring(name));
}

bool RuntimeNode_has_prop(RuntimeNode *node, const char *name) {
    return RuntimeNode_get_prop(node, name) != NULL;
}

void RuntimeNode_print(RuntimeNode *node, FILE *output, uint32 indent) {
    for (uint32 i = 0; i < indent; ++i) {
        fputc(' ', output);
    }
    fprintf(output, "Node: \"%s\" (0x%08X)\n", String_data(&node->name), node->name_hash);
    for (uint32 i = 0; i < node->props.count; ++i) {
        RuntimeProp *prop = DA_at(&node->props, i);
        if (prop->type == PROP_TYPE_NONE)continue;
        RuntimeProp_print(prop, output, indent + 2);
    }
    for (uint32 i = 0; i < node->children.count; ++i) {
        RuntimeNode *child = DA_at(&node->children, i);
        RuntimeNode_print(child, output, indent + 2);
    }
}

void RuntimeNode__from_buffer(RuntimeNode *node, Buffer *buffer) {
    RuntimeNodeHeader header;
    if (buffer->read(buffer, &header, sizeof(RuntimeNodeHeader), NULL) != BUFFER_SUCCESS) {
        printf("[ERROR]: Failed to read RTPC root node header\n");
        exit(1);
    }

    RuntimeNode_init(node, find_name(header.name_hash));


    node->name_hash = header.name_hash;
    DA_reserve(&node->props, header.prop_count);
    DA_reserve(&node->children, header.child_count);
    int64 orig_pos;
    if (buffer->get_position(buffer, &orig_pos) != BUFFER_SUCCESS) {
        printf("[ERROR]: Failed to get current buffer position\n");
        exit(1);
    }
    if (buffer->set_position(buffer, header.data_offset, BUFFER_ORIGIN_START) != BUFFER_SUCCESS) {
        printf("[ERROR]: Failed to seek to node data offset: %u\n", header.data_offset);
        exit(1);
    }
    for (int i = 0; i < header.prop_count; ++i) {
        RuntimeProp *prop = DA_append_get(&node->props);
        RuntimeProp__from_buffer(prop, buffer);
    }
    // Align buffer position to 4
    int64 pos;
    if (buffer->get_position(buffer, &pos) != BUFFER_SUCCESS) {
        printf("[ERROR]: Failed to get current buffer position\n");
        exit(1);
    }
    if (buffer->set_position(buffer, (pos + 3) & ~3, BUFFER_ORIGIN_START) != BUFFER_SUCCESS) {
        printf("[ERROR]: Failed to align buffer position after reading RTPC properties\n");
        exit(1);
    }
    for (int i = 0; i < header.child_count; ++i) {
        RuntimeNode *child_node = DA_append_get(&node->children);
        RuntimeNode__from_buffer(child_node, buffer);
    }

    if (buffer->set_position(buffer, orig_pos, BUFFER_ORIGIN_START) != BUFFER_SUCCESS) {
        printf("[ERROR]: Failed to return back\n");
        exit(1);
    }
}

#define READ_FROM_OFFSET(offset, body)\
    do{\
        int64 orig_offset;\
        if(buffer->get_position(buffer, &orig_offset)!=BUFFER_SUCCESS){\
            printf("[ERROR]: Failed to get current buffer position\n");\
            exit(1);\
        }\
        if (buffer->set_position(buffer, offset, BUFFER_ORIGIN_START) != BUFFER_SUCCESS) {\
            printf("[ERROR]: Failed to seek to RTPC property data at offset: %u\n", offset);\
            exit(1);\
        }\
        {body}\
        if(buffer->set_position(buffer, orig_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) {\
            printf("[ERROR]: Failed to restore buffer position after reading RTPC property value\n");\
            exit(1);\
        }\
    }while(0)

void RuntimeProp__from_buffer(RuntimeProp *prop, Buffer *buffer) {
    RuntimePropHeader header;
    if (buffer->read(buffer, &header, sizeof(RuntimePropHeader), NULL) != BUFFER_SUCCESS) {
        printf("[ERROR]: Failed to read RTPC property header\n");
        exit(1);
    }
    RuntimeProp_init(prop, header.prop_type);
    prop->name_hash = header.name_hash;
    String_move_from(&prop->name, String_move(find_name(header.name_hash)));
    switch (prop->type) {
        case PROP_TYPE_NONE: {
            // Nothing to read
            break;
        }
        case PROP_TYPE_U32: {
            prop->value.uint32_value = header.data_raw.uint_value;
            break;
        }
        case PROP_TYPE_F32: {
            prop->value.float32_value = header.data_raw.float_value;
            break;
        }
        case PROP_TYPE_STR: {
            READ_FROM_OFFSET(header.data_raw.uint_value, {
                             if (buffer->read_cstring(buffer, &prop->value.string_value)!=BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC string property value\n");
                             exit(1);
                             }});
            break;
        }
        case PROP_TYPE_VEC2: {
            READ_FROM_OFFSET(header.data_raw.uint_value, {
                             if (buffer->read(buffer, prop->value.vec2_value, 4*2, NULL)!=BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC vec2 property value\n");
                             exit(1);
                             }});
            break;
        }
        case PROP_TYPE_VEC3: {
            READ_FROM_OFFSET(header.data_raw.uint_value, {
                             if (buffer->read(buffer, prop->value.vec3_value, 4*3, NULL)!=BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC vec3 property value\n");
                             exit(1);
                             }});
            break;
        }
        case PROP_TYPE_VEC4: {
            READ_FROM_OFFSET(header.data_raw.uint_value, {
                             if (buffer->read(buffer, prop->value.vec4_value, 4*4, NULL)!=BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC vec4 property value\n");
                             exit(1);
                             }});
            break;
        }
        case PROP_TYPE_MAT3X3: {
            READ_FROM_OFFSET(header.data_raw.uint_value, {
                             if (buffer->read(buffer, prop->value.vec4_value, 4*9, NULL)!=BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC MAT3X3 property value\n");
                             exit(1);
                             }});
            break;
        }
        case PROP_TYPE_MAT4X4: {
            READ_FROM_OFFSET(header.data_raw.uint_value, {
                             if (buffer->read(buffer, prop->value.vec4_value, 4*16, NULL)!=BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC MAT4X4 property value\n");
                             exit(1);
                             }});
            break;
        }
        case PROP_TYPE_ARRAY_U32: {
            READ_FROM_OFFSET(header.data_raw.uint_value, {
                             uint32 array_size;
                             if (buffer->read_uint32(buffer, &array_size)!=BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC u32 array size\n");
                             exit(1);
                             }
                             DA_reserve(&prop->value.uint32_array_value, array_size);
                             prop->value.uint32_array_value.count = array_size;
                             if (array_size>0)
                             if (buffer->read(buffer, prop->value.uint32_array_value.items, 4*array_size, NULL)!=
                                 BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC ARRAY_U32 property value\n");
                             exit(1);
                             }});
            break;
        }
        case PROP_TYPE_ARRAY_F32: {
            READ_FROM_OFFSET(header.data_raw.uint_value, {
                             uint32 array_size;
                             if (buffer->read_uint32(buffer, &array_size)!=BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC u32 array size\n");
                             exit(1);
                             }
                             DA_reserve(&prop->value.float32_array_value, array_size);
                             prop->value.float32_array_value.count = array_size;
                             if (array_size>0)
                             if (buffer->read(buffer, prop->value.float32_array_value.items, 4*array_size, NULL)!=
                                 BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC ARRAY_F32 property value\n");
                             exit(1);
                             }});
            break;
        }
        case PROP_TYPE_ARRAY_U8: {
            READ_FROM_OFFSET(header.data_raw.uint_value, {
                             uint32 array_size;
                             if (buffer->read_uint32(buffer, &array_size)!=BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC u32 array size\n");
                             exit(1);
                             }
                             DA_reserve(&prop->value.uint8_array_value, array_size);
                             prop->value.uint8_array_value.count = array_size;
                             if (array_size>0)
                             if (buffer->read(buffer, prop->value.uint8_array_value.items, array_size, NULL)!=
                                 BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC ARRAY_U8 property value\n");
                             exit(1);
                             }});
            break;
        }
        case PROP_TYPE_OBJID: {
            READ_FROM_OFFSET(header.data_raw.uint_value, {
                             if (buffer->read_uint64(buffer, &prop->value.objid_value)!=BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC OBJID property value\n");
                             exit(1);
                             }});
            break;
        }
        case PROP_TYPE_EVENT: {
            READ_FROM_OFFSET(header.data_raw.uint_value, {
                             uint32 array_size;
                             if (buffer->read_uint32(buffer, &array_size)!=BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC u32 array size\n");
                             exit(1);
                             }
                             DA_reserve(&prop->value.event_value, array_size);
                             prop->value.event_value.count = array_size;
                             if (array_size>0)
                             if (buffer->read(buffer, prop->value.event_value.items, 8*array_size, NULL)!=
                                 BUFFER_SUCCESS){
                             printf("[ERROR]: Failed to read RTPC EVENT property value\n");
                             exit(1);
                             }});
            break;
        }
        default: {
            printf("[ERROR]: Unknown RTPC property type: %d\n", prop->type);
            exit(1);
        }
    }
}


void RuntimeNode_free(RuntimeNode *node) {
    assert(node!=NULL);
    for (int i = 0; i < node->props.count; ++i) {
        RuntimeProp_free(DA_at(&node->props, i));
    }
    DA_free(&node->props);

    for (int i = 0; i < node->children.count; ++i) {
        RuntimeNode_free(DA_at(&node->children, i));
    }
    DA_free(&node->children);
    String_free(&node->name);
    node->name_hash = 0;
    if (node->heap_allocated)
        free(node);
}

void RuntimeProp_init(RuntimeProp *prop, PropType type) {
    prop->type = type;
    String_init(&prop->name, 16);
    switch (type) {
        case PROP_TYPE_NONE:
        case PROP_TYPE_U32:
        case PROP_TYPE_F32:
        case PROP_TYPE_VEC2:
        case PROP_TYPE_VEC3:
        case PROP_TYPE_VEC4:
        case PROP_TYPE_MAT3X3:
        case PROP_TYPE_MAT4X4:
        case PROP_TYPE_OBJID: {
            // Nothing to init
            break;
        }
        case PROP_TYPE_STR: {
            String_init(&prop->value.string_value, 64);
            break;
        }

        case PROP_TYPE_ARRAY_U32: {
            DA_init(&prop->value.uint32_array_value, uint32, 4);
            break;
        }
        case PROP_TYPE_ARRAY_F32: {
            DA_init(&prop->value.float32_array_value, float32, 4);
            break;
        }
        case PROP_TYPE_ARRAY_U8: {
            DA_init(&prop->value.uint8_array_value, uint8, 4);
            break;
        }
        case PROP_TYPE_EVENT: {
            DA_init(&prop->value.event_value, uint64, 4);
            break;
        }
        case PROP_TYPE_DEPRECIATED_12:
        case PROP_TYPE_UNK_15:
        case PROP_TYPE_UNK_16:
        default: {
            printf("[ERROR]: Unknown prop type: %d\n", type);
            exit(1);
        }
    }
}

void RuntimeProp_print(RuntimeProp *prop, FILE *output, uint32 indent) {
    for (uint32 i = 0; i < indent; ++i) {
        fputc(' ', output);
    }
    fprintf(output, "Prop: \"%s\" (0x%08X) Type: %d Value: ", String_data(&prop->name), prop->name_hash,
            prop->type);
    switch (prop->type) {
        case PROP_TYPE_NONE: {
            fprintf(output, "<NONE>\n");
            break;
        }
        case PROP_TYPE_U32: {
            const String *name_value = find_name(prop->value.uint32_value);
            if (name_value != NULL) {
                fprintf(output, "\"%s\" (0x%08X)\n", String_data(name_value), prop->value.uint32_value);
            } else {
                fprintf(output, "%u\n", prop->value.uint32_value);
            }

            break;
        }
        case PROP_TYPE_F32: {
            fprintf(output, "%f\n", prop->value.float32_value);
            break;
        }
        case PROP_TYPE_STR: {
            fprintf(output, "\"%s\"\n", String_data(&prop->value.string_value));
            break;
        }
        case PROP_TYPE_VEC2: {
            fprintf(output, "[%f, %f]\n", prop->value.vec2_value[0], prop->value.vec2_value[1]);
            break;
        }
        case PROP_TYPE_VEC3: {
            fprintf(output, "[%f, %f, %f]\n", prop->value.vec3_value[0], prop->value.vec3_value[1],
                    prop->value.vec3_value[2]);
            break;
        }
        case PROP_TYPE_VEC4: {
            fprintf(output, "[%f, %f, %f, %f]\n", prop->value.vec4_value[0], prop->value.vec4_value[1],
                    prop->value.vec4_value[2], prop->value.vec4_value[3]);
            break;
        }
        case PROP_TYPE_MAT3X3: {
            fprintf(output, "[");
            for (int i = 0; i < 9; ++i) {
                if (i > 0) fprintf(output, ", ");
                fprintf(output, "%f", prop->value.matrix33_value[i]);
            }
            fprintf(output, "]\n");
            break;
        }
        case PROP_TYPE_MAT4X4: {
            fprintf(output, "[");
            for (int i = 0; i < 16; ++i) {
                if (i > 0) fprintf(output, ", ");
                fprintf(output, "%f", prop->value.matrix44_value[i]);
            }
            fprintf(output, "]\n");
            break;
        }
        case PROP_TYPE_ARRAY_U32: {
            fprintf(output, "[");
            uint32 count = prop->value.uint32_array_value.count;
            if (count > 32) count = 32;
            for (int i = 0; i < count; ++i) {
                if (i > 0) fprintf(output, ", ");
                fprintf(output, "%u", prop->value.uint32_array_value.items[i]);
            }
            if (prop->value.uint8_array_value.count > 32) {
                fprintf(output, ", ... (%u total)", prop->value.uint32_array_value.count);
            }
            fprintf(output, "]\n");
            break;
        }
        case PROP_TYPE_ARRAY_F32: {
            fprintf(output, "[");
            uint32 count = prop->value.float32_array_value.count;
            if (count > 32) count = 32;
            for (int i = 0; i < count; ++i) {
                if (i > 0) fprintf(output, ", ");
                fprintf(output, "%f", prop->value.float32_array_value.items[i]);
            }
            if (prop->value.uint8_array_value.count > 32) {
                fprintf(output, ", ... (%u total)", prop->value.float32_array_value.count);
            }
            fprintf(output, "]\n");
            break;
        }
        case PROP_TYPE_ARRAY_U8: {
            fprintf(output, "[");
            uint32 count = prop->value.uint8_array_value.count;
            if (count > 32) count = 32;
            for (int i = 0; i < count; ++i) {
                if (i > 0) fprintf(output, ", ");
                fprintf(output, "%u", prop->value.uint8_array_value.items[i]);
            }
            if (prop->value.uint8_array_value.count > 32) {
                fprintf(output, ", ... (%u total)", prop->value.uint8_array_value.count);
            }
            fprintf(output, "]\n");
            break;
        }
        case PROP_TYPE_OBJID: {
            fprintf(output, "0x%016lX\n", prop->value.objid_value);
            break;
        }
        case PROP_TYPE_EVENT: {
            fprintf(output, "[");
            uint32 count = prop->value.event_value.count;
            if (count > 32) count = 32;
            for (int i = 0; i < count; ++i) {
                if (i > 0) fprintf(output, ", ");
                fprintf(output, "0x%016lX", prop->value.event_value.items[i]);
            }
            if (prop->value.event_value.count > 32) {
                fprintf(output, ", ... (%u total)", prop->value.event_value.count);
            }
            fprintf(output, "]\n");
            break;
        }
        default: {
            printf("[ERROR]: Unknown prop type: %d\n", prop->type);
            exit(1);
        }
    }
}

void RuntimeProp_free(RuntimeProp *prop) {
    String_free(&prop->name);
    prop->name_hash = 0;
    switch (prop->type) {
        case PROP_TYPE_NONE:
        case PROP_TYPE_U32:
        case PROP_TYPE_F32:
        case PROP_TYPE_VEC2:
        case PROP_TYPE_VEC3:
        case PROP_TYPE_VEC4:
        case PROP_TYPE_MAT3X3:
        case PROP_TYPE_MAT4X4:
        case PROP_TYPE_OBJID: {
            // Nothing to free
            break;
        }
        case PROP_TYPE_STR: {
            String_free(&prop->value.string_value);
            break;
        }

        case PROP_TYPE_ARRAY_U32: {
            DA_free(&prop->value.uint32_array_value);
            break;
        }
        case PROP_TYPE_ARRAY_F32: {
            DA_free(&prop->value.float32_array_value);
            break;
        }
        case PROP_TYPE_ARRAY_U8: {
            DA_free(&prop->value.uint8_array_value);
            break;
        }
        case PROP_TYPE_EVENT: {
            DA_free(&prop->value.event_value);
            break;
        }
        case PROP_TYPE_DEPRECIATED_12:
        case PROP_TYPE_UNK_15:
        case PROP_TYPE_UNK_16:
        default: {
            printf("[ERROR]: Unknown prop type: %d\n", prop->type);
            exit(1);
        }
    }
    prop->type = 0;
    memset(&prop->value, 0, sizeof(prop->value));
}

static void print_indent(FILE *out, uint32 indent) {
    for (uint32 i = 0; i < indent; ++i) fputc(' ', out);
}

void RuntimeProp_emit_json(RuntimeProp *prop, FILE *out, uint32 indent) {
    print_indent(out, indent);
    fprintf(out, "\"%s\": ", String_data(&prop->name));
    switch (prop->type) {
        case PROP_TYPE_NONE:
            fprintf(out, "null");
            break;
        case PROP_TYPE_U32:
            fprintf(out, "%u", prop->value.uint32_value);
            break;
        case PROP_TYPE_F32:
            fprintf(out, "%f", prop->value.float32_value);
            break;
        case PROP_TYPE_STR:
            fprintf(out, "\"%s\"", String_data(&prop->value.string_value));
            break;
        case PROP_TYPE_VEC2:
            fprintf(out, "[%f, %f]", prop->value.vec2_value[0], prop->value.vec2_value[1]);
            break;
        case PROP_TYPE_VEC3:
            fprintf(out, "[%f, %f, %f]", prop->value.vec3_value[0], prop->value.vec3_value[1],
                    prop->value.vec3_value[2]);
            break;
        case PROP_TYPE_VEC4:
            fprintf(out, "[%f, %f, %f, %f]", prop->value.vec4_value[0], prop->value.vec4_value[1],
                    prop->value.vec4_value[2], prop->value.vec4_value[3]);
            break;
        case PROP_TYPE_MAT3X3:
            fprintf(out, "[");
            for (int i = 0; i < 9; ++i) {
                if (i > 0) fprintf(out, ", ");
                fprintf(out, "%f", prop->value.matrix33_value[i]);
            }
            fprintf(out, "]");
            break;
        case PROP_TYPE_MAT4X4:
            fprintf(out, "[");
            for (int i = 0; i < 16; ++i) {
                if (i > 0) fprintf(out, ", ");
                fprintf(out, "%f", prop->value.matrix44_value[i]);
            }
            fprintf(out, "]");
            break;
        case PROP_TYPE_ARRAY_U32: {
            fprintf(out, "[");
            uint32 count = prop->value.uint32_array_value.count;
            if (count > 32) count = 32;
            for (uint32 i = 0; i < count; ++i) {
                if (i > 0) fprintf(out, ", ");
                fprintf(out, "%u", prop->value.uint32_array_value.items[i]);
            }
            if (prop->value.uint32_array_value.count > 32)
                fprintf(out, ", null");
            fprintf(out, "]");
            break;
        }
        case PROP_TYPE_ARRAY_F32: {
            fprintf(out, "[");
            uint32 count = prop->value.float32_array_value.count;
            if (count > 32) count = 32;
            for (uint32 i = 0; i < count; ++i) {
                if (i > 0) fprintf(out, ", ");
                fprintf(out, "%f", prop->value.float32_array_value.items[i]);
            }
            if (prop->value.float32_array_value.count > 32)
                fprintf(out, ", null");
            fprintf(out, "]");
            break;
        }
        case PROP_TYPE_ARRAY_U8: {
            fprintf(out, "[");
            uint32 count = prop->value.uint8_array_value.count;
            if (count > 32) count = 32;
            for (uint32 i = 0; i < count; ++i) {
                if (i > 0) fprintf(out, ", ");
                fprintf(out, "%u", prop->value.uint8_array_value.items[i]);
            }
            if (prop->value.uint8_array_value.count > 32)
                fprintf(out, ", null");
            fprintf(out, "]");
            break;
        }
        case PROP_TYPE_OBJID:
            fprintf(out, "\"0x%016lX\"", prop->value.objid_value);
            break;
        case PROP_TYPE_EVENT: {
            fprintf(out, "[");
            uint32 count = prop->value.event_value.count;
            if (count > 32) count = 32;
            for (uint32 i = 0; i < count; ++i) {
                if (i > 0) fprintf(out, ", ");
                fprintf(out, "\"0x%016lX\"", prop->value.event_value.items[i]);
            }
            if (prop->value.event_value.count > 32)
                fprintf(out, ", null");
            fprintf(out, "]");
            break;
        }
        default:
            fprintf(out, "null");
    }
}

void RuntimeNode_emit_json(RuntimeNode *node, FILE *out, uint32 indent) {
    print_indent(out, indent);
    fprintf(out, "{\n");
    print_indent(out, indent + 2);
    fprintf(out, "\"name\": \"%s\",\n", String_data(&node->name));
    print_indent(out, indent + 2);
    fprintf(out, "\"name_hash\": %u,\n", node->name_hash);

    // Properties
    if (node->props.count) {
        print_indent(out, indent + 2);
        fprintf(out, "\"props\": {\n");
        for (uint32 i = 0; i < node->props.count; ++i) {
            RuntimeProp *prop = DA_at(&node->props, i);
            if (prop->type == PROP_TYPE_NONE) continue;
            RuntimeProp_emit_json(prop, out, indent + 4);
            if (i + 1 < node->props.count) fprintf(out, ",");
            fprintf(out, "\n");
        }
        print_indent(out, indent + 2);
        fprintf(out, "},\n");
    }

    // Children
    if (node->children.count) {
        print_indent(out, indent + 2);
        fprintf(out, "\"children\": [\n");
        for (uint32 i = 0; i < node->children.count; ++i) {
            RuntimeNode *child = DA_at(&node->children, i);
            RuntimeNode_emit_json(child, out, indent + 4);
            if (i + 1 < node->children.count) fprintf(out, ",");
            fprintf(out, "\n");
        }
        print_indent(out, indent + 2);
        fprintf(out, "]\n");
    }

    print_indent(out, indent);
    fprintf(out, "}");
}
