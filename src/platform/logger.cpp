// Created by RED on 17.01.2026.

#include "platform/logger.h"
#include <format>
#include <string_view>
#include <iostream>
#include <memory>

// Logger implementation
Logger::Logger(std::ostream& output) : output_(&output) {}

void Logger::set_output(std::ostream& output) {
    output_ = &output;
}

void Logger::log(const LogLevel level, const std::string_view message) const {
    log_s(level, "", 0, message);
}

void Logger::log_s(const LogLevel level, std::string_view source, uint32 source_line, std::string_view message) const {
    std::string_view prefix;
    switch (level) {
        case LogLevel::Info:    prefix = "[INFO ]"; break;
        case LogLevel::Warning: prefix = "[WARN ]"; break;
        case LogLevel::Error:   prefix = "[ERROR]"; break;
    }

    if (!source.empty() && source_line > 0) {
        *output_ << std::format("{} [{}:{}]: {}\n", prefix, source, source_line, message);
    } else {
        *output_ << std::format("{}: {}\n", prefix, message);
    }
    output_->flush();
}

// Global logger namespace implementation
namespace GLog {
    namespace {
        std::unique_ptr<Logger> g_logger;
    }

    void init(std::ostream& output) {
        g_logger = std::make_unique<Logger>(output);
    }

    Logger& get() {
        if (!g_logger) {
            g_logger = std::make_unique<Logger>(std::cout);
        }
        return *g_logger;
    }

    std::string_view file_name(const std::string_view name) {
        size_t last_slash = 0;
        for (size_t i = 0; i < name.size(); ++i) {
            if (name[i] == '/' || name[i] == '\\') {
                last_slash = i + 1;
            }
        }
        return name.substr(last_slash);
    }
}
