// Created by RED on 23.09.2025.

/* Internal helpers */
#include <stdint.h>
#include <ctype.h>
#include <errno.h>

#include "utils/path.h"


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
static int Path__ensure_upto(const char *s, uint32 upto_len) {
    String cur = {0};
    String_init(&cur, 0);

    uint32 start = Path__mkdir_start_index(s, upto_len);
    uint32 i = 0;

    if (start > 0) {
        String_append_format(&cur, "%.*s", (int) start, s);
    }

    while (i < upto_len) {
        char c = s[i++];
        if (Path__is_sep(c)) {
            char last = 0;
            if (cur.size > 0) last = String_data(&cur)[cur.size - 1];
            if (last != PATH_NATIVE_SEP) {
                char sep[1] = {PATH_NATIVE_SEP};
                String_append_cstr2(&cur, sep, 1);
            }
            if (cur.size >= start) {
                char *p = cur.buffer;
                int r = PATH_MKDIR(p);
                if (r != 0 && errno != EEXIST) {
                    int saved = errno;
                    String_free(&cur);
                    errno = saved;
                    return -1;
                }
            }
        } else {
            String_append_cstr2(&cur, &c, 1);
        }
    }

    if (cur.size > 0) {
        char *p = cur.buffer;
        int r = PATH_MKDIR(p);
        if (r != 0 && errno != EEXIST) {
            int saved = errno;
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
        String_resize(base, 0);
        for (uint32_t i = 0; i < len; ++i) {
            char c = component[i];
            if (Path__is_sep(c)) c = PATH_NATIVE_SEP;
            String_append_cstr2(base, &c, 1);
        }
        return;
    }

    uint32_t i = 0;
    while (i < len && Path__is_sep(component[i])) i++;

    if (base->size > 0) {
        char last = String_data(base)[base->size - 1];
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

/*
 Ensures all directories in the given path exist. Treats a trailing separator
 as a directory. Returns 0 on success, -1 on error (errno set).
*/
int Path_ensure_dirs(const Path *path) {
    const char *s = String_data(path);
    uint32_t len = path->size;
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
    const char *s = String_data(path);
    uint32_t len = path->size;
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
    const char *s = String_data(component);
    uint32_t len = component->size;
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

void Path_convert_to_wsl(Path *out, Path *in) {
#ifdef WIN32
        String_copy_from(out, in);
#else
        String_init(out, in->size + 10);
        char *buffer = out->buffer;

        char drive = in->buffer[0];
        snprintf(buffer, out->capacity, "/mnt/%c%s", (drive > 'A' ? drive + ' ' : drive), in->buffer + 2);
        // Fix slashes
        for (int i = 0; i < out->capacity; ++i) {
            if (buffer[i] == '\\') {
                buffer[i] = '/';
            }
        }
        out->size = strlen(String_data(out));
#endif
}
void find_files_by_ext(const char *dir, const String *ext, DynamicArray_Path *tab_files);
#ifdef WIN32
#include <Windows.h>

void find_files_by_ext(const char *dir, const String* ext, DynamicArray_Path *tab_files) {
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
        } else {
            const char *file_ext = strrchr(fd.cFileName, '.');
            if (file_ext && strcmp(file_ext, String_data(ext)) == 0) {
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

        if (is_dir_path(full_path, ent)) {
            find_files_by_ext(full_path, ext, tab_files);
        } else {
            const char *file_ext = strrchr(name, '.');
            if (file_ext && strcmp(file_ext, String_data(ext)) == 0) {
                String_from_cstr(DA_append_get(tab_files), full_path);

            }
        }
    }
    closedir(d);
}
#endif

void Path_rglob(const Path *path, const String *ext, DynamicArray_Path *out) {
    DA_init(out, Path, 1);
    if (path->size == 0) return;
    find_files_by_ext(String_data(path), ext, out);
}
