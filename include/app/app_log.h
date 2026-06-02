#pragma once

#include <string>
#include <cstdint>

// Simple logging to file and stdout
// Timestamps included, separate info/warn/error levels
namespace app_log {
    void init(const std::string& filepath);
    void close();

    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

    int64_t now_ms();
    void log_elapsed(const std::string& label, int64_t start_ms);
}
