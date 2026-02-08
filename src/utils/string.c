#include "utils/string.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "platform/logger.h"
#include "utils/memory_profiling.h"


void String_free(String *string) {
    if (string == NULL) {
        GLog_Warning("Trying to free NULL string");
        return;
    }
    zstr_free(&string->owned);
    if (string->heap_allocated) {
        mp_free(string);
    }
}

String* String_new(const uint32 size) {
    String *s = (String *)mp_malloc(sizeof(String));
    assert(s && "Out of memory");
    memset(s, 0, sizeof(String));
    s->heap_allocated = 1;
    String_init(s, size);
    return s;
}

String* String_new_from_cstr(const char *str) {
    String *s = (String *)mp_malloc(sizeof(String));
    assert(s && "Out of memory");
    memset(s, 0, sizeof(String));
    s->heap_allocated = 1;
    String_from_cstr(s, str);
    return s;
}

String * String_new_from_cstr2(const char *str, const uint32 len) {
    String *s = (String *)mp_malloc(sizeof(String));
    assert(s!=NULL && "Out of memory");
    memset(s, 0, sizeof(String));
    s->heap_allocated = 1;
    String_from_cstr2(s, str, len);
    return s;
}

String* String_new_from_str(const String *other) {
    String *s = (String *)mp_malloc(sizeof(String));
    assert(s && "Out of memory");
    memset(s, 0, sizeof(String));
    s->heap_allocated = 1;
    String_copy_from(s, other);
    return s;
}

void String_init(String *string, const uint32 size) {
    if (string==NULL) {
        assert(false && "String_init: string is NULL");
        return;
    }
    zstr_free(&string->owned);
    string->owned = zstr_with_capacity(size);
}

String *String_from_cstr(String *string, const char *str) {
    if (string==NULL) {
        assert(false && "String_from_cstr: string is NULL");
        return NULL;
    }
    if (!str) { String_init(string, 0); return string; }

    string->owned = zstr_from(str);

    return string;
}

String * String_from_cstr2(String *string, const char *str, uint32 len) {
    if (string==NULL) {
        assert(false && "String_from_cstr2: string is NULL");
        return NULL;
    }
    if (!str) { String_init(string, 0); return string; }

    string->owned = zstr_from_len(str, len);

    return string;
}


const char *String_cstr(const String *string) {
    if (string==NULL) {
        assert(false && "String_cstr: string is NULL");
        return NULL;
    }
    return zstr_cstr(&string->owned);
}

char * String_data(String *string) {
    if (string==NULL) {
        assert(false && "String_data: string is NULL");
        return NULL;
    }
    return zstr_data(&string->owned);
}

uint32 String_size(const String *string) {
    if (string==NULL) {
        assert(false && "String_size: string is NULL");
        return 0;
    }
    return (uint32)zstr_len(&string->owned);
}

void String_set_size(String *string, const uint32 size) {
    if (string==NULL) {
        assert(false && "String_set_size: string is NULL");
        return;
    }
    zstr_reserve(&string->owned, size);
    if (string->owned.is_long) {
        string->owned.l.len = size;
        string->owned.l.ptr[size] = '\0';
    } else {
        string->owned.s.len = (uint8_t)size;
        string->owned.s.buf[size] = '\0';
    }
}

void String_reserve(String *string, const uint32 size) {
    if (!string) return;
    zstr_reserve(&string->owned, size);
}

void String_trim_zeros(String *string) {
    if (!string) return;
    zstr_shrink_to_fit(&string->owned);
}

void String_fill(String *string, const uint32 offset, const uint32 size, char chr) {
    if (!string) return;
    zstr_reserve(&string->owned, offset + size);
    char *data = zstr_data(&string->owned);
    for (uint32 i = 0; i < size; ++i) {
        data[offset + i] = chr;
    }
}

void String_append_cstr(String *string, const char *str) {
    if (!string || !str) return;
    zstr_cat(&string->owned, str);
}

void String_append_cstr2(String *string, const char *str, const uint32 size) {
    if (!string || !str || size == 0) return;
    zstr_cat_len(&string->owned, str, size);
}

void String_append_str(String *s, const String *other) {
    if (!s || !other) return;
    zstr_cat_len(&s->owned, zstr_cstr(&other->owned), zstr_len(&other->owned));
}

void String_sub_string(const String *string, const uint32 start, int32 size, String *out) {
    if (!out) return;
    if (size==-1) {
        size = (int32)zstr_len(&string->owned) - (int32)start;
    }
    const zstr_view sub = zstr_sub(zstr_as_view(&string->owned), start, size);
    out->owned = zstr_from_view(sub);

}

int32 String_find_chr(const String *string, char chr) {
    if (!string) return -1;
    const char *data = zstr_cstr(&string->owned);
    const char *found = strchr(data, chr);
    if (!found) return -1;

    return (int32)(found - data);
}

void String_copy_from(String *dst, const String *src) {
    if (!dst || !src) return;
    zstr_free(&dst->owned);
    dst->owned = zstr_from(zstr_cstr(&src->owned));
}

void String_move_from(String *dst, String *src) {
    if (!dst || !src) return;
    zstr_free(&dst->owned);
    dst->owned = src->owned;
    src->owned = zstr_init();
}

void String_format(String *string, const char *fmt, ...) {
    if (!string || !fmt) return;

    va_list args;
    va_start(args, fmt);
    zstr_vmt_va(&string->owned, fmt, args);
    va_end(args);
}

void String_append_format(String *string, const char *fmt, ...) {
    if (!string || !fmt) return;

    zstr new = zstr_init();

    va_list args;
    va_start(args, fmt);
    zstr_vmt_va(&new, fmt, args);
    va_end(args);

    zstr_cat_len(&string->owned, zstr_cstr(&new), zstr_len(&new));

    zstr_free(&new);
}

void String_prepend_format(String *string, const char *fmt, ...) {
    if (!string || !fmt) return;

    zstr new = zstr_init();

    va_list args;
    va_start(args, fmt);
    zstr_vmt_va(&new, fmt, args);
    va_end(args);

    zstr old = string->owned;
    string->owned = zstr_init();
    zstr_cat_len(&string->owned, zstr_cstr(&new), zstr_len(&new));
    zstr_cat_len(&string->owned, zstr_cstr(&old), zstr_len(&old));

    zstr_free(&old);
    zstr_free(&new);
}

bool String_equals(const String *string, const String *other) {
    if (!string || !other) return false;
    return zstr_eq(&string->owned, &other->owned);
}

bool String_cequals(const String *string, const char *other) {
    if (!string || !other) return false;
    return zstr_view_eq(zstr_as_view(&string->owned), other);
}

bool String_ends_with(const String *string, const String *suffix) {
    if (!string || !suffix) return false;
    return zstr_ends_with(&string->owned, zstr_cstr(&suffix->owned));
}

bool String_cends_with(const String *string, const char *suffix) {
    if (!string || !suffix) return false;
    return zstr_ends_with(&string->owned, suffix);
}

bool String_cstarts_with(const String *string, const char *prefix) {
    if (!string || !prefix) return false;
    return zstr_starts_with(&string->owned, prefix);
}

char * String_detach(String *string) {
    if (!string) return NULL;

    char* detached = zstr_take(&string->owned);

    return detached;
}

uint32 String_find_subcstring(const String *string, const char *sub) {
    if (!string || !sub) return UINT32_MAX;

    const char *data = zstr_cstr(&string->owned);
    const char *found = strstr(data, sub);
    if (!found) return UINT32_MAX;

    return (uint32)(found - data);
}

void String_replace_char(String *string, const char *targets, const char replacement) {
    if (!string || !targets) return;

    char *data = zstr_data(&string->owned);
    const size_t len = zstr_len(&string->owned);
    const size_t target_count = strlen(targets);

    for (size_t i = 0; i < len; ++i) {
        for (size_t j = 0; j < target_count; ++j) {
            if (data[i] == targets[j]) {
                data[i] = replacement;
                break;
            }
        }
    }
}

