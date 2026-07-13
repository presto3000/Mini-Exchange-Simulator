#include "common/Logger.hpp"

#include <iostream>
#include <mutex>
#include <chrono>
#include <format>

namespace exchange::common {

    namespace {
        // Module-local, not a class-level global: guards std::cerr across
        // threads within a single process. This is *not* shared mutable state
        // in the "no global state" architectural sense - it's a synchronization
        // primitive for an I/O sink, scoped to this translation unit.
        std::mutex g_ioMutex;

        constexpr std::string_view toString(LogLevel level) {
            switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info:  return "INFO ";
            case LogLevel::Warn:  return "WARN ";
            case LogLevel::Error: return "ERROR";
            }
            return "?????";
        }
    }

    Logger::Logger(std::string_view serviceName) : serviceName_(serviceName) {}

    void Logger::log(LogLevel level, std::string_view message) const {
        const auto now = std::chrono::system_clock::now();

        std::lock_guard lock(g_ioMutex);
        std::cerr << std::format("[{:%Y-%m-%d %H:%M:%S}] [{}] [{}] {}\n",
            std::chrono::floor<std::chrono::seconds>(now),
            toString(level),
            serviceName_,
            message);
    }

} // namespace exchange::common