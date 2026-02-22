// Created by RED on 07.02.2026.

#include "utils/string_view.h"

#include <assert.h>

bool StringView_is_empty(const StringView self) {
    return self.view.data == NULL || self.view.len == 0;
}

StringView StringView_empty() {
    StringView sv = {};
    sv.view.data = NULL;
    sv.view.len = 0;
    return sv;
}

StringView StringView_from_cstr(const char *str) {
    StringView sv = {};
    if (!str) {
        sv.view.data = NULL;
        sv.view.len = 0;
        return sv;
    }
    sv.view = zstr_view_from(str);
    return sv;
}

StringView StringView_from_cstr2(const char *str, const uint32 len) {
    StringView sv = {};
    if (!str) {
        sv.view.data = NULL;
        sv.view.len = 0;
        return sv;
    }
    sv.view = zstr_view_from(str);
    sv.view.len = len;
    return sv;
}

StringView StringView_from_string(const String *string) {
    StringView sv = {};
    if (string==NULL) {
        sv.view.data = NULL;
        sv.view.len = 0;
        return sv;
    }
    sv.view = zstr_as_view(&string->owned);
    return sv;
}

const char * StringView_cstr(const StringView self) {
    return self.view.data;
}

uint32 StringView_size(const StringView self) {
    return (uint32)self.view.len;
}

String* StringView_to_new_string(const StringView self) {
    return String_new_from_cstr2(self.view.data, (uint32)self.view.len);
}

void StringView_to_string(const StringView self, String *out) {
    if (out==NULL) {
        assert(false && "StringView_to_string: output string is NULL");
        return;
    }
    String_from_cstr2(out, self.view.data, (uint32)self.view.len);
}

int32 StringView_find_subcstring(const StringView self, const char *sub) {
    if (!sub) return -1;
    const char *found = strstr(self.view.data, sub);
    if (!found) return -1;
    return (int32)(found - self.view.data);
}

bool StringView_equals(const StringView self, const StringView other) {
    if (self.view.len != other.view.len) return false;
    if (self.view.data == other.view.data) return true; // Same pointer and length, must be equal

    return memcmp(self.view.data, other.view.data, self.view.len) == 0;
}

bool StringView_cequals(const StringView self, const char *other) {
    if (!other) return false;
    const size_t other_len = strlen(other);
    if (self.view.len != other_len) return false;

    return memcmp(self.view.data, other, other_len) == 0;
}

bool StringView_cends_with(const StringView self, const char *suffix) {
    if (!suffix) return false;
    return zstr_view_ends_with(self.view, suffix);
}

void String_copy_from_view(String *string, const StringView view) {
    if (string==NULL) {
        assert(false && "String_copy_from_view: string is NULL");
        return;
    }
    zstr_cat(&string->owned, StringView_cstr(view));
}
