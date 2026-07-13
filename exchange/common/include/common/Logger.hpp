#pragma once

#include <string>
#include <string_view>

namespace exchange::common {

    enum class LogLevel {
        Debug,
        Info,
        Warn,
        Error
    };

    // Minimal thread-safe logging facade.
    //
    // Deliberately a thin wrapper, not a singleton with global state: each
    // service constructs its own Logger instance and passes it explicitly to
    // components that need it (dependency injection). This keeps us honest
    // about the "no global state" requirement and makes components testable
    // in isolation.
    class Logger {
    public:
        explicit Logger(std::string_view serviceName);

        void log(LogLevel level, std::string_view message) const;

        void debug(std::string_view message) const { log(LogLevel::Debug, message); }
        void info(std::string_view message)  const { log(LogLevel::Info, message); }
        void warn(std::string_view message)  const { log(LogLevel::Warn, message); }
        void error(std::string_view message) const { log(LogLevel::Error, message); }

    private:
        std::string serviceName_;
    };

} // namespace exchange::common