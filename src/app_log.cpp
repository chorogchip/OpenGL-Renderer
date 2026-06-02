#include "app_log.h"

#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace {
    std::ofstream g_log_file;

    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    void log_impl(const std::string& level, const std::string& msg) {
        std::string log_line = "[" + get_timestamp() + "] [" + level + "] " + msg;
        std::cout << log_line << std::endl;
        if (g_log_file.is_open()) {
            g_log_file << log_line << std::endl;
            g_log_file.flush();
        }
    }
}

namespace app_log {
    void init(const std::string& filepath) {
        g_log_file.open(filepath, std::ios::app);
        if (!g_log_file.is_open()) {
            std::cerr << "Failed to open log file: " << filepath << std::endl;
        }
    }

    void close() {
        if (g_log_file.is_open()) {
            g_log_file.close();
        }
    }

    void info(const std::string& msg) {
        log_impl("INFO", msg);
    }

    void warn(const std::string& msg) {
        log_impl("WARN", msg);
    }

    void error(const std::string& msg) {
        log_impl("ERR", msg);
    }

    int64_t now_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    void log_elapsed(const std::string& label, int64_t start_ms) {
        int64_t elapsed = now_ms() - start_ms;
        std::string msg = label + ": " + std::to_string(elapsed) + "ms";
        info(msg);
    }
}
