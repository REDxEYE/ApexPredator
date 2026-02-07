// Created by RED on 23.09.2025.

/* Internal helpers */
#include <stdint.h>
#include <ctype.h>
#include <errno.h>

#include "utils/path.h"
#include "platform/logger.h"


#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#  include <direct.h>
#  define PATH_MKDIR(p) _mkdir(p)
#  define PATH_NATIVE_SEP '\\'
#else
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>
#  include <dirent.h>
#  define PATH_MKDIR(p) mkdir(p, 0777)
#  define PATH_NATIVE_SEP '/'
#endif

static int Path__is_sep(char c) { return c == '/' || c == '\\'; }

/*
  Returns the index after any root/prefix so mkdir starts at the right component.
  Handles POSIX '/', Windows drive roots like 'C:\', and UNC prefixes '\\server\share\'.
*/
static uint32 Path__mkdir_start_index(const char *s, uint32 len) {
#ifdef _WIN32
    if (len >= 2 && isalpha((unsigned char) s[0]) && s[1] == ':') {
        uint32 i = 2;
        if (i < len && Path__is_sep(s[i])) i += 1;
        return i;
    }
    if (len >= 2 && Path__is_sep(s[0]) && Path__is_sep(s[1])) {
        uint32 i = 2, sep_seen = 2, segments = 0;
        while (i < len && segments < 2) {
            while (i < len && Path__is_sep(s[i])) {
                i++;
                sep_seen++;
            }
            if (i == len) break;
            while (i < len && !Path__is_sep(s[i])) i++;
            segments++;
        }
        if (i < len && Path__is_sep(s[i])) i++;
        return i <= len ? i : len;
    }
    if (len && Path__is_sep(s[0])) return 1;
    return 0;
#else
    return (len && s[0] == '/') ? 1u : 0u;
#endif
}

/*
  Creates intermediate directories for the first upto_len bytes of s.
  Returns 0 on success, -1 on error (errno preserved).
*/
static int Path__ensure_upto(const char *s, const uint32 upto_len) {
    String cur = {0};

    const uint32 start = Path__mkdir_start_index(s, upto_len);
    uint32 i = 0;

    if (start > 0) {
        String_append_format(&cur, "%.*s", (int) start, s);
    }

    while (i < upto_len) {
        char c = s[i++];
        if (Path__is_sep(c)) {
            char last = 0;
            if (String_size(&cur) > 0) last = String_cstr(&cur)[String_size(&cur) - 1];
            if (last != PATH_NATIVE_SEP) {
                char sep[1] = {PATH_NATIVE_SEP};
                String_append_cstr2(&cur, sep, 1);
            }
            if (String_size(&cur) >= start) {
                char *p = String_data(&cur);
                int r = PATH_MKDIR(p);
                if (r != 0 && errno != EEXIST) {
                    int saved = errno;
                    String_free(&cur);
                    errno = saved;
                    return -1;
                }
            }
        }
        else {
            String_append_cstr2(&cur, &c, 1);
        }
    }

    if (String_size(&cur) > 0) {
        const char *p = String_data(&cur);
        const int r = PATH_MKDIR(p);
        if (r != 0 && errno != EEXIST) {
            const int saved = errno;
            String_free(&cur);
            errno = saved;
            return -1;
        }
    }

    String_free(&cur);
    return 0;
}

static int Path__is_absolute(const char *s, uint32_t len) {
    if (len == 0) return 0;
#ifdef _WIN32
    if (len >= 2 && isalpha((unsigned char) s[0]) && s[1] == ':') return 1;
    if (len >= 2 && Path__is_sep(s[0]) && Path__is_sep(s[1])) return 1;
    if (Path__is_sep(s[0])) return 1;
    return 0;
#else
    return s[0] == '/';
#endif
}

static void Path__append_normalized(Path *base, const char *component, uint32_t len) {
    if (len == 0) return;

    if (Path__is_absolute(component, len)) {
        String_reserve(base, 0);
        for (uint32_t i = 0; i < len; ++i) {
            char c = component[i];
            if (Path__is_sep(c)) c = PATH_NATIVE_SEP;
            String_append_cstr2(base, &c, 1);
        }
        return;
    }

    uint32_t i = 0;
    while (i < len && Path__is_sep(component[i])) i++;

    if (String_size(base) > 0) {
        char last = String_cstr(base)[String_size(base) - 1];
        if (!Path__is_sep(last)) {
            char sep = PATH_NATIVE_SEP;
            String_append_cstr2(base, &sep, 1);
        }
    }

    for (; i < len; ++i) {
        char c = component[i];
        if (Path__is_sep(c)) c = PATH_NATIVE_SEP;
        String_append_cstr2(base, &c, 1);
    }
}

void Path_normalize_native(Path *path) {
    const char *s = String_cstr(path);
    uint32_t len = String_size(path);
    if (len == 0) return;

    String normalized = {0};
    String_init(&normalized, len);

    for (uint32_t i = 0; i < len; ++i) {
        char c = s[i];
        if (Path__is_sep(c)) c = PATH_NATIVE_SEP;
        String_append_cstr2(&normalized, &c, 1);
    }

    String_free(path);
    *path = normalized;
}

void Path_normalize_posix(Path *path) {
    // Normalize inplace
    char *s = String_data(path);
    const uint32_t len = String_size(path);
    if (len == 0) return;
    for (uint32_t i = 0; i < len; ++i) {
        char c = s[i];
        if (Path__is_sep(c)) c = '/';
        s[i] = c;
    }
}

void Path_normalize_windows(Path *path) {
    // Normalize inplace
    char *s = String_data(path);
    const uint32_t len = String_size(path);
    if (len == 0) return;
    for (uint32_t i = 0; i < len; ++i) {
        char c = s[i];
        if (Path__is_sep(c)) c = '\\';
        s[i] = c;
    }
}

void Path_replace_invalid_fs_chars(Path *filename, const char replacement) {
    char *s = String_data(filename);
    const uint32_t len = String_size(filename);
    if (len == 0) return;

    for (uint32_t i = 0; i < len; ++i) {
        char c = s[i];
        if (c < 32 || c == '<' || c == '>' || c == '|' || c == '"' || c == '?' || c == '*' || c == ':' || c == '/' || c
            == '\\') {
            c = replacement;
        }
        s[i] = c;
    }
}

void Path_get_parent(const Path *path, Path *out_parent) {
    const char *s = String_cstr(path);
    const uint32_t len = String_size(path);
    if (len == 0) {
        String_reserve(out_parent, 0);
        return;
    }

    int32_t last_sep = -1;
    for (int32_t i = (int32_t) len - 1; i >= 0; --i) {
        if (Path__is_sep(s[i])) {
            last_sep = i;
            break;
        }
    }
    if (last_sep < 0) {
        String_reserve(out_parent, 0);
        return;
    }
    String_init(out_parent, last_sep);
    String_append_format(out_parent, "%.*s", (int) last_sep, s);
}

/*
 Ensures all directories in the given path exist. Treats a trailing separator
 as a directory. Returns 0 on success, -1 on error (errno set).
*/
int Path_ensure_dirs(const Path *path) {
    const char *s = String_cstr(path);
    uint32_t len = String_size(path);
    if (len == 0) return 0;

    int trailing_sep = Path__is_sep(s[len - 1]) ? 1 : 0;
    uint32_t upto = trailing_sep ? (len - 1) : len;

    return Path__ensure_upto(s, upto);
}

/*
 Ensures all parent directories for the given path exist, skipping the final
 component. Returns 0 on success, -1 on error (errno set).
*/
int Path_ensure_parent_dirs(const Path *path) {
    const char *s = String_cstr(path);
    uint32_t len = String_size(path);
    if (len == 0) return 0;

    int32_t last_sep = -1;
    for (int32_t i = (int32_t) len - 1; i >= 0; --i) {
        if (Path__is_sep(s[i])) {
            last_sep = i;
            break;
        }
    }
    if (last_sep < 0) return 0;

    return Path__ensure_upto(s, (uint32_t) last_sep);
}

/*
 Joins base with the given String component, normalizing separators and
 handling absolute components as replacements.
*/
void Path_join(Path *base, const String *component) {
    const char *s = String_cstr(component);
    uint32_t len = String_size(component);
    Path__append_normalized(base, s, len);
}

void Path_join_sv(Path *base, const StringView component) {
    const char *s = StringView_cstr(component);
    const uint32_t len = StringView_size(component);
    Path__append_normalized(base, s, len);
}

/*
 Joins base with the given C-string component, normalizing separators and
 handling absolute components as replacements.
*/
void Path_join_cstr(Path *base, const char *component) {
    if (!component) return;
    uint32_t len = (uint32_t) strlen(component);
    Path__append_normalized(base, component, len);
}

void Path_join_format(Path *base, const char *fmt, ...) {
    if (!fmt) return;
    va_list args;
    va_start(args, fmt);
    char buffer[1024];
    int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (n > 0) {
        uint32_t len = (uint32_t) ((n < (int) sizeof(buffer)) ? n : (int) sizeof(buffer) - 1);
        Path__append_normalized(base, buffer, len);
    }
}

void Path_convert_to_wsl(Path *src) {
#ifndef WSL_ENV
    return;
#else
    if (String_size(src) == 0) {
        return;
    }
    const char *in_buffer = String_cstr(src);
    if (in_buffer[0] == '.') {
        return;
    }
    String converted = {};
    const char drive = in_buffer[0];

    String_format(&converted, "/mnt/%c%s", (drive > 'A' ? drive + ' ' : drive), in_buffer + 2);

    String_replace_char(&converted, "\\", '/');
    String_move_from(src, &converted);

#endif
}

void find_files_by_ext(const char *dir, const String *ext, DynamicArray_Path *tab_files);
#ifdef WIN32
#include <Windows.h>

void find_files_by_ext(const char *dir, const String *ext, DynamicArray_Path *tab_files) {
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", dir);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;

        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s\\%s", dir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            find_files_by_ext(full_path, ext, tab_files);
        }
        else {
            const char *file_ext = strrchr(fd.cFileName, '.');
            if (file_ext && strcmp(file_ext, String_cstr(ext)) == 0) {
                String *tmp = DA_append_get(tab_files);
                String_from_cstr(tmp, full_path);
            }
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
}

#else
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

static int is_dir_path(const char *fullpath, const struct dirent *ent) {

// Use d_type if available and reliable; otherwise lstat
#ifdef DT_DIR
if (ent&& ent->d_type!= DT_UNKNOWN) {
        return ent->d_type == DT_DIR;
    }
#endif
struct stat st;
    if (lstat(fullpath, &st)== 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

void find_files_by_ext(const char *dir, const String *ext, DynamicArray_Path *tab_files) {
    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    char full_path[PATH_MAX];

    while ((ent = readdir(d)) != NULL) {
        const char *name = ent->d_name;

        if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
            continue;

        int n = snprintf(full_path, sizeof full_path, "%s/%s", dir, name);
        if (n < 0 || (size_t) n >= sizeof full_path)
            continue; // path too long, skip
        GLog_info("Visiting: %s", full_path);
        if (is_dir_path(full_path, ent)) {
            find_files_by_ext(full_path, ext, tab_files);
        }
        else {
            const char *file_ext = strrchr(name, '.');
            if (file_ext && strcmp(file_ext, String_cstr(ext)) == 0) {
                String_from_cstr(DA_append_get(tab_files), full_path);
            }
        }
    }
    closedir(d);
}
#endif

void Path_rglob(const Path *path, const String *ext, DynamicArray_Path *out) {
    DA_init(out, Path, 1);
    if (String_size(path) == 0) return;
    find_files_by_ext(String_cstr(path), ext, out);
}

void Path_remove_extension(const Path *path, Path *extensionless) {
    String_init(extensionless, String_size(path));
    const char *s = String_cstr(path);
    int32_t last_dot = -1;
    for (int32_t i = (int32_t) String_size(path) - 1; i >= 0; --i) {
        if (s[i] == '.') {
            last_dot = i;
            break;
        }
        if (Path__is_sep(s[i])) {
            break;
        }
    }
    if (last_dot < 0) {
        String_copy_from(extensionless, path);
    }
    else {
        String_append_cstr2(extensionless, s, (uint32_t) last_dot);
    }
}

void Path_remove_extension_sv(const StringView path, Path *extensionless) {
    String_init(extensionless, StringView_size(path));
    const char *s = StringView_cstr(path);
    int32_t last_dot = -1;
    for (int32_t i = (int32_t) StringView_size(path) - 1; i >= 0; --i) {
        if (s[i] == '.') {
            last_dot = i;
            break;
        }
        if (Path__is_sep(s[i])) {
            break;
        }
    }
    if (last_dot < 0) {
        String_append_cstr2(extensionless, s, StringView_size(path));
    }
    else {
        String_append_cstr2(extensionless, s, (uint32_t) last_dot);
    }
}

void Path_replace_extension(const Path *path, const char *new_extension, Path *out) {
    const size_t ext_size = new_extension ? strlen(new_extension) : 0;
    const char *s = String_cstr(path);
    int32_t last_dot = -1;
    for (int32_t i = (int32_t) String_size(path) - 1; i >= 0; --i) {
        if (s[i] == '.') {
            last_dot = i;
            break;
        }
        if (Path__is_sep(s[i])) {
            break;
        }
    }
    if (last_dot < 0) {
        String_init(out, String_size(path) + (uint32_t) ext_size + 1);
        String_copy_from(out, path);
        if (new_extension && new_extension[0] != '\0') {
            String_append_cstr(out, ".");
            String_append_cstr(out, new_extension);
        }
    }
    else {
        String_init(out, last_dot + (uint32_t) ext_size + 1);
        String_append_cstr2(out, s, (uint32_t) last_dot);
        if (new_extension && new_extension[0] != '\0') {
            String_append_cstr(out, ".");
            String_append_cstr(out, new_extension);
        }
    }
}

void Path_replace_extension_sv(StringView path, const char *new_extension, Path *out) {
    const size_t ext_size = new_extension ? strlen(new_extension) : 0;
    const char *s = StringView_cstr(path);
    int32_t last_dot = -1;
    for (int32_t i = (int32_t) StringView_size(path) - 1; i >= 0; --i) {
        if (s[i] == '.') {
            last_dot = i;
            break;
        }
        if (Path__is_sep(s[i])) {
            break;
        }
    }
    if (last_dot < 0) {
        String_init(out, StringView_size(path) + (uint32_t) ext_size + 1);
        String_append_cstr2(out, s, StringView_size(path));
        if (new_extension && new_extension[0] != '\0') {
            String_append_cstr(out, ".");
            String_append_cstr(out, new_extension);
        }
    }
    else {
        String_init(out, last_dot + (uint32_t) ext_size + 1);
        String_append_cstr2(out, s, (uint32_t) last_dot);
        if (new_extension && new_extension[0] != '\0') {
            String_append_cstr(out, ".");
            String_append_cstr(out, new_extension);
        }
    }
}

void Path_replace_extension_inplace(Path *path, const char *new_extension) {
    if (new_extension == NULL)return;
    const size_t ext_size = strlen(new_extension);
    const char *s = String_cstr(path);
    int32_t last_dot = -1;
    for (int32_t i = (int32_t) String_size(path) - 1; i >= 0; --i) {
        if (s[i] == '.') {
            last_dot = i;
            break;
        }
        if (Path__is_sep(s[i])) {
            break;
        }
    }

    if (last_dot < 0) {
        String_append_format(path, ".%s", new_extension);
    }
    else {
        if (last_dot + ext_size + 1 > String_size(path)) {
            String_reserve(path, (uint32_t) (last_dot + ext_size + 1));
            if (path->owned.is_long) {
                path->owned.l.len = (uint32_t) (last_dot);
            }
            else {
                path->owned.s.len = (uint8_t) (last_dot);
            }
        }
        String_append_format(path, ".%s", new_extension);
    }
}

void Path_filename(const Path *path, Path *filename) {
    String_init(filename, String_size(path));
    const char *s = String_cstr(path);
    int32_t last_sep = -1;
    for (int32_t i = (int32_t) String_size(path) - 1; i >= 0; --i) {
        if (Path__is_sep(s[i])) {
            last_sep = i;
            break;
        }
    }
    if (last_sep < 0) {
        String_copy_from(filename, path);
    }
    else {
        String_append_cstr2(filename, s + last_sep + 1, String_size(path) - (uint32_t) last_sep - 1);
    }
}

void Path_filename_sv(const StringView view, Path *filename) {
    String_init(filename, StringView_size(view));
    const char *s = view.view.data;
    int32_t last_sep = -1;
    for (int32_t i = (int32_t) StringView_size(view) - 1; i >= 0; --i) {
        if (Path__is_sep(s[i])) {
            last_sep = i;
            break;
        }
    }
    if (last_sep < 0) {
        String_append_cstr2(filename, s, StringView_size(view));
    }
    else {
        String_append_cstr2(filename, s + last_sep + 1, StringView_size(view) - (uint32_t) last_sep - 1);
    }
}

bool Path_exists(const Path *path) {
    if (String_size(path) == 0) return false;
    const char *s = String_cstr(path);
#ifdef WIN32
    DWORD attrs = GetFileAttributesA(s);
    return attrs != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat(s, &st) == 0;
#endif
}
