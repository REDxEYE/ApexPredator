#ifndef DECIMA_NATIVE_JSON_H
#define DECIMA_NATIVE_JSON_H

// Taken from https://github.com/ShadelessFox/decima-native/blob/master/include/json.h

#include <stdio.h>
#include <stdint.h>

#define jsonValueStr(_Ctx, _Value) jsonValue(_Ctx, (JsonValue) {.type = JsonType_String, .string = (_Value)})
#define jsonValueNum(_Ctx, _Value) jsonValue(_Ctx, (JsonValue) {.type = JsonType_Integer, .integer = (_Value)})
#define jsonValueFlt(_Ctx, _Value) jsonValue(_Ctx, (JsonValue) {.type = JsonType_Float, .float_ = (_Value)})
#define jsonValueBool(_Ctx, _Value) jsonValue(_Ctx, (JsonValue) {.type = JsonType_Bool, .integer = (_Value)})
#define jsonValueNull(_Ctx) jsonValue(_Ctx, (JsonValue) {.type = JsonType_Null})

#define jsonBeginCompactObject(_Ctx) do { jsonBeginObject(_Ctx); jsonCompact(_Ctx, 1); } while (0)
#define jsonEndCompactObject(_Ctx) do { jsonEndObject(_Ctx); jsonCompact(_Ctx, 0); } while (0)
#define jsonBeginCompactArray(_Ctx) do { jsonBeginArray(_Ctx); jsonCompact(_Ctx, 1); } while (0)
#define jsonEndCompactArray(_Ctx) do { jsonEndArray(_Ctx); jsonCompact(_Ctx, 0); } while (0)

#define jsonNameValueStr(_Ctx, _Name, _Value) \
    do {                                      \
        jsonName(_Ctx, _Name);                \
        jsonValueStr(_Ctx, _Value);           \
    } while (0)

#define jsonNameValueNum(_Ctx, _Name, _Value) \
    do {                                      \
        jsonName(_Ctx, _Name);                \
        jsonValueNum(_Ctx, _Value);           \
    } while (0)

#define jsonNameValueBool(_Ctx, _Name, _Value) \
    do {                                       \
        jsonName(_Ctx, _Name);                 \
        jsonValueBool(_Ctx, _Value);           \
    } while (0)

#define jsonNameObject(_Ctx, _Name) \
    do {                            \
        jsonName(_Ctx, _Name);      \
        jsonBeginObject(_Ctx);      \
    } while (0)

#define jsonNameArray(_Ctx, _Name) \
    do {                           \
        jsonName(_Ctx, _Name);     \
        jsonBeginArray(_Ctx);      \
    } while (0)

#define jsonNameCompactObject(_Ctx, _Name) \
    do {                                   \
        jsonName(_Ctx, _Name);             \
        jsonBeginCompactObject(_Ctx);      \
    } while (0)

#define jsonNameCompactArray(_Ctx, _Name) \
    do {                                  \
        jsonName(_Ctx, _Name);            \
        jsonBeginCompactArray(_Ctx);      \
    } while (0)

typedef struct JsonContext {
    FILE *stream;
    int compact;
    const char *name;
    size_t index;
    int scopes[32];
}JsonContext;

enum JsonType {
    JsonType_String,
    JsonType_Integer,
    JsonType_Float,
    JsonType_Bool,
    JsonType_Null
};

typedef struct JsonValue {
    enum JsonType type;
    union {
        const char *string;
        int64_t integer;
        double float_;
    };
}JsonValue;

void jsonInit(JsonContext *ctx, FILE *);

void jsonBeginObject(JsonContext *ctx);

void jsonEndObject(JsonContext *ctx);

void jsonBeginArray(JsonContext *ctx);

void jsonEndArray(JsonContext *ctx);

void jsonName(JsonContext *ctx, const char *name);

void jsonValue(JsonContext *ctx, JsonValue value);

void jsonCompact(JsonContext *ctx, int compact);

#endif //DECIMA_NATIVE_JSON_H
