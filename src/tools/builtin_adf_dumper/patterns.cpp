// Created by RED on 15.02.2026.

#include "Windows.h"
#include "patterns.h"

#include <ctype.h>
#include <stdlib.h>
#include "string.h"


void pattern_free(Pattern *p) {
    if (!p) return;
    free(p->bytes);
    free(p->mask);
    p->bytes = NULL;
    p->mask = NULL;
    p->len = 0;
}

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = tolower((unsigned char) c);
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return -1;
}

static int parse_byte_token(const char *tok, uint8_t *out_byte, uint8_t *out_mask) {
    // Accept: "??", "?", "A3", "a3"
    if (!tok || !*tok) return 0;

    if (tok[0] == '?') {
        *out_byte = 0;
        *out_mask = 0;
        return 1;
    }

    int h1 = hexval((unsigned char) tok[0]);
    int h2 = hexval((unsigned char) tok[1]);
    if (h1 < 0 || h2 < 0) return 0;

    *out_byte = (uint8_t) ((h1 << 4) | h2);
    *out_mask = 1;
    return 1;
}

int pattern_compile(Pattern *out, const char *pattern_str) {
    if (!out || !pattern_str) return 0;
    memset(out, 0, sizeof(*out));

    // First pass: count tokens
    size_t count = 0;
    const char *s = pattern_str;
    while (*s) {
        while (*s && isspace((unsigned char) *s)) s++;
        if (!*s) break;
        // token ends at whitespace
        while (*s && !isspace((unsigned char) *s)) s++;
        count++;
    }
    if (count == 0) return 0;

    out->bytes = (uint8_t *) malloc(count);
    out->mask = (uint8_t *) malloc(count);
    if (!out->bytes || !out->mask) {
        pattern_free(out);
        return 0;
    }

    // Second pass: parse tokens
    size_t i = 0;
    s = pattern_str;
    while (*s) {
        while (*s && isspace((unsigned char) *s)) s++;
        if (!*s) break;

        const char *tok_start = s;
        while (*s && !isspace((unsigned char) *s)) s++;
        size_t tok_len = (size_t) (s - tok_start);

        char tok[8];
        if (tok_len >= sizeof(tok)) {
            pattern_free(out);
            return 0;
        }
        memcpy(tok, tok_start, tok_len);
        tok[tok_len] = '\0';

        uint8_t b = 0, m = 0;
        if (!parse_byte_token(tok, &b, &m)) {
            pattern_free(out);
            return 0;
        }

        out->bytes[i] = b;
        out->mask[i] = m;
        i++;
    }

    out->len = i;
    return out->len != 0;
}

const uint8_t *pattern_find_first(const void *data, const size_t size, const Pattern *p) {
    if (!data || !p || p->len == 0) return NULL;
    if (size < p->len) return NULL;

    const uint8_t *buf = (const uint8_t *) data;
    size_t last = size - p->len;

    for (size_t i = 0; i <= last; i++) {
        size_t j = 0;
        for (; j < p->len; j++) {
            if (p->mask[j] && buf[i + j] != p->bytes[j])
                break;
        }
        if (j == p->len)
            return buf + i;
    }
    return NULL;
}

Region get_module() {
    const uintptr_t module_base = (uintptr_t) GetModuleHandleA(NULL);
    const IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *) module_base;
    const IMAGE_NT_HEADERS64 *nt_headers64 = (IMAGE_NT_HEADERS64 *) (module_base + dos_header->e_lfanew);
    const uintptr_t module_end = module_base + nt_headers64->OptionalHeader.SizeOfImage;
    return {module_base, module_end};
}

Region get_pe_section(const Region module, const char *section) {
    const IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *) module.base;
    const IMAGE_NT_HEADERS64 *nt_headers64 = (IMAGE_NT_HEADERS64 *) (module.base + dos_header->e_lfanew);
    const IMAGE_SECTION_HEADER *sections = (IMAGE_SECTION_HEADER *) (module.base + dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS64));

    for (size_t i = 0; i < nt_headers64->FileHeader.NumberOfSections; i++) {
        const IMAGE_SECTION_HEADER *sh = &sections[i];
        if (memcmp(sh->Name, section, 8) == 0) {
            return {module.base + sh->VirtualAddress, module.base + sh->VirtualAddress + sh->Misc.VirtualSize};
        }
    }
    return {0, 0};
}
