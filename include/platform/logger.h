// Created by RED on 17.01.2026.

#ifndef APEXPREDATOR_LOGGER_H
#define APEXPREDATOR_LOGGER_H

#include <format>
#include <string_view>
#include <iostream>
#include "int_def.h"

enum class LogLevel {
    Info,
    Warning,
    Error
};

class Logger {
public:
    explicit Logger(std::ostream& output = std::cout);
    ~Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = default;
    Logger& operator=(Logger&&) = default;

    template<typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) const {
        log(LogLevel::Info, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void warning(std::format_string<Args...> fmt, Args&&... args) const {
        log(LogLevel::Warning, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args) const {
        log(LogLevel::Error, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void info_s(std::string_view source, uint32 source_line, std::format_string<Args...> fmt, Args&&... args) const {
        log_s(LogLevel::Info, source, source_line, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void warning_s(std::string_view source, uint32 source_line, std::format_string<Args...> fmt, Args&&... args) const {
        log_s(LogLevel::Warning, source, source_line, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void error_s(std::string_view source, uint32 source_line, std::format_string<Args...> fmt, Args&&... args) const {
        log_s(LogLevel::Error, source, source_line, std::format(fmt, std::forward<Args>(args)...));
        output_->flush();
    }

    void set_output(std::ostream& output);

private:
    void log(LogLevel level, std::string_view message) const;
    void log_s(LogLevel level, std::string_view source, uint32 source_line, std::string_view message) const;

    std::ostream* output_;
};

// Global logger interface
namespace GLog {
    void init(std::ostream& output = std::cout);
    Logger& get();

    template<typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) {
        get().info(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warning(std::format_string<Args...> fmt, Args&&... args) {
        get().warning(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args) {
        get().error(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info_s(std::string_view source, uint32 source_line, std::format_string<Args...> fmt, Args&&... args) {
        get().info_s(source, source_line, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warning_s(std::string_view source, uint32 source_line, std::format_string<Args...> fmt, Args&&... args) {
        get().warning_s(source, source_line, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error_s(std::string_view source, uint32 source_line, std::format_string<Args...> fmt, Args&&... args) {
        get().error_s(source, source_line, fmt, std::forward<Args>(args)...);
    }

    std::string_view file_name(std::string_view name);
}

// Convenience macros
#define GLog_Info(...) GLog::info_s(GLog::file_name(__FILE__), __LINE__, __VA_ARGS__)
#define GLog_Warning(...) GLog::warning_s(GLog::file_name(__FILE__), __LINE__, __VA_ARGS__)
#define GLog_Error(...) GLog::error_s(GLog::file_name(__FILE__), __LINE__, __VA_ARGS__)

#endif //APEXPREDATOR_LOGGER_H