// Created by RED on 07.02.2026.

#ifndef APEXPREDATOR_STRING_VIEW_H
#define APEXPREDATOR_STRING_VIEW_H

#include "int_def.h"
#include "string.h"
#include "zstr.h"

typedef struct StringView {
    zstr_view view;
} StringView;

#define SV_FMT "%.*s"

#define SV_ARGS(sv) StringView_size((sv)), StringView_cstr((sv))

#define as_sv(string) (StringView_from_string((string)))
#define sv_is_null(string_view) (StringView_is_empty((string_view)))
#define sv_is_not_null(string_view) (!StringView_is_empty((string_view)))

bool StringView_is_empty(StringView self);
StringView StringView_empty();
StringView StringView_from_cstr(const char *str);
StringView StringView_from_cstr2(const char *str, uint32 len);
StringView StringView_from_string(const String *string);

const char* StringView_cstr(StringView self);
uint32 StringView_size(StringView self);

String* StringView_to_new_string(StringView self);
void StringView_to_string(StringView self, String *out);

int32 StringView_find_subcstring(StringView self, const char *sub);
bool StringView_equals(StringView self, StringView other);
bool StringView_cequals(StringView self, const char *other);

void String_copy_from_view(String* string, StringView view);


#endif //APEXPREDATOR_STRING_VIEW_H
