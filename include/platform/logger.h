// Created by RED on 17.01.2026.

#ifndef APEXPREDATOR_LOGGER_H
#define APEXPREDATOR_LOGGER_H

#include <stdarg.h>

#include "int_def.h"

typedef struct Logger Logger;

void Log_init(Logger* log, FILE* output);

void Log_info(const Logger* log, const char* fmt, ...);
void Log_warning(const Logger* log, const char* fmt, ...);
void Log_error(const Logger* log, const char* fmt, ...);

void Log_info_s(const Logger* log, const char* source, uint32 source_line, const char* fmt, ...);
void Log_warning_s(const Logger* log, const char* source, uint32 source_line, const char* fmt, ...);
void Log_error_s(const Logger* log, const char* source, uint32 source_line, const char* fmt, ...);

void Log_info_va(const Logger* log, const char* fmt, va_list va);
void Log_warning_va(const Logger* log, const char* fmt, va_list va);
void Log_error_va(const Logger* log, const char* fmt, va_list va);

void Log_info_s_va(const Logger* log, const char* source, uint32 source_line, const char* fmt, va_list va);
void Log_warning_s_va(const Logger* log, const char* source, uint32 source_line, const char* fmt, va_list va);
void Log_error_s_va(const Logger* log, const char* source, uint32 source_line, const char* fmt, va_list va);

void GLog_init(FILE* output);

void GLog_info(const char* fmt, ...);
void GLog_warning(const char* fmt, ...);
void GLog_error(const char* fmt, ...);

void GLog_info_s(const char* source, uint32 source_line, const char* fmt, ...);
void GLog_warning_s(const char* source, uint32 source_line, const char* fmt, ...);
void GLog_error_s(const char* source, uint32 source_line, const char* fmt, ...);

inline const char* GLog_file_name(const char* name) {
    const char* last_slash = name;
    for (const char* c = name; *c != '\0'; c++) {
        if (*c == '/' || *c == '\\') {
            last_slash = c + 1;
        }
    }
    return last_slash;
}

#define GLog_Info(...) GLog_info_s(GLog_file_name(__FILE__), __LINE__, __VA_ARGS__)
#define GLog_Warning(...) GLog_warning_s(GLog_file_name(__FILE__), __LINE__, __VA_ARGS__)
#define GLog_Error(...) GLog_error_s(GLog_file_name(__FILE__), __LINE__, __VA_ARGS__)

#endif //APEXPREDATOR_LOGGER_H