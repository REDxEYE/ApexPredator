// Created by RED on 17.01.2026.

#include <stdio.h>
#include "../../include/platform/logger.h"

#include "int_def.h"

struct Logger {
    FILE *output;
};


void Log_init(Logger *log, FILE *output) {
    log->output = output;
}

void Log_write(const Logger *log, const char *prefix, const char *source, uint32 source_line, const char *fmt,
               va_list va) {
    if (source != NULL) {
        fprintf(log->output, "%s [%s:%d]: ", prefix, source, source_line);
    } else {
        fprintf(log->output, "%s: ", prefix);
    }
    vfprintf(log->output, fmt, va);
    fprintf(log->output, "\n");
}

void Log_info(const Logger *log, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log_info_va(log, fmt, args);
    va_end(args);
}

void Log_warning(const Logger *log, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log_warning_va(log, fmt, args);
    va_end(args);
}

void Log_error(const Logger *log, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log_error_va(log, fmt, args);
    va_end(args);
}

void Log_info_s(const Logger *log, const char* source, uint32 source_line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log_info_s_va(log, source, source_line, fmt, args);
    va_end(args);
}

void Log_warning_s(const Logger *log, const char* source, uint32 source_line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log_warning_s_va(log, source, source_line, fmt, args);
    va_end(args);
}

void Log_error_s(const Logger *log, const char* source, uint32 source_line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log_error_s_va(log, source, source_line, fmt, args);
    va_end(args);
}

void Log_info_va(const Logger *log, const char *fmt, va_list va) {
    Log_write(log, "[INFO ]", NULL, 0, fmt, va);
}

void Log_warning_va(const Logger *log, const char *fmt, va_list va) {
    Log_write(log, "[WARN ]", NULL, 0, fmt, va);
}

void Log_error_va(const Logger *log, const char *fmt, va_list va) {
    Log_write(log, "[ERROR]", NULL, 0, fmt, va);
}

void Log_info_s_va(const Logger* log, const char* source, uint32 source_line, const char* fmt, va_list va) {
    Log_write(log, "[INFO ]", source, source_line, fmt, va);
}
void Log_warning_s_va(const Logger* log, const char* source, uint32 source_line, const char* fmt, va_list va) {
    Log_write(log, "[WARN ]", source, source_line, fmt, va);
}
void Log_error_s_va(const Logger* log, const char* source, uint32 source_line, const char* fmt, va_list va) {
    Log_write(log, "[ERROR]", source, source_line, fmt, va);
}


Logger *GLog_get() {
    static Logger g_Logger = {0};
    if (g_Logger.output == NULL) {
        Log_init(&g_Logger, stdout);
    }
    return &g_Logger;
}


void GLog_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log_info_va(GLog_get(), fmt, args);
    va_end(args);
}

void GLog_warning(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log_warning_va(GLog_get(), fmt, args);
    va_end(args);
}

void GLog_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log_error_va(GLog_get(), fmt, args);
    va_end(args);
}

void GLog_info_s(const char* source, uint32 source_line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log_info_s_va(GLog_get(), source, source_line, fmt, args);
    va_end(args);
}
void GLog_warning_s(const char* source, uint32 source_line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log_warning_s_va(GLog_get(), source, source_line, fmt, args);
    va_end(args);
}
void GLog_error_s(const char* source, uint32 source_line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log_error_s_va(GLog_get(), source, source_line, fmt, args);
    va_end(args);
}

const char * GLog_file_name(const char *name) {
    const char* last_slash = name;
    for (const char* c = name; *c != '\0'; c++) {
        if (*c == '/' || *c == '\\') {
            last_slash = c + 1;
        }
    }
    return last_slash;
}
