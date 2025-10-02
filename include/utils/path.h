// Created by RED on 23.09.2025.

#ifndef APEXPREDATOR_PATH_H
#define APEXPREDATOR_PATH_H

#include "utils/string.h"
#include "utils/dynamic_array.h"


/* Forward declarations to avoid depending on the full String header here. */
typedef String Path;

DYNAMIC_ARRAY_STRUCT(Path, Path);


int Path_ensure_dirs(const Path *path);

/*
 Ensures all parent directories for the given path exist, skipping the final
 component. Returns 0 on success, -1 on error (errno set).
*/
int Path_ensure_parent_dirs(const Path *path);

/*
 Joins base with the given String component, normalizing separators and
 handling absolute components as replacements.
*/
void Path_join(Path *base, const String *component);

/*
 Joins base with the given C-string component, normalizing separators and
 handling absolute components as replacements.
*/
void Path_join_cstr(Path *base, const char *component);

/*
 Joins base with the given formatted component, normalizing separators and
 handling absolute components as replacements.
*/
void Path_join_format(Path *base, const char *fmt, ...);

void Path_convert_to_wsl(Path *out, Path *in);

void Path_rglob(const Path *path, const String *ext, DynamicArray_Path* out);

void Path_remove_extension(const Path* path, Path* extensionless);

void Path_filename(const Path* path, Path* filename);

bool Path_exists(const Path* path);

#endif //APEXPREDATOR_PATH_H