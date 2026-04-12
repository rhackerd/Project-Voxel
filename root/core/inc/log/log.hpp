#pragma once

#include <fmt/color.h>
#include <fmt/chrono.h>
#include <chrono>
#include <mutex>
#include <string>
#include <string_view>

namespace N::Log {

enum class LogLevel { Info, Warn, Error, Debug };

inline std::mutex& log_mutex() {
    static std::mutex m;
    return m;
}

inline void nova_log(LogLevel level, std::string_view logger_name, std::string_view msg, std::string_view file, int line) {
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::current_zone()->to_local(std::chrono::floor<std::chrono::seconds>(now));

    std::string_view fname = file;
    auto slash = fname.find_last_of("/\\");
    if (slash != std::string_view::npos) fname = fname.substr(slash + 1);

    fmt::color color;
    std::string_view label;
    switch (level) {
        case LogLevel::Info:  color = fmt::color::green;  label = "INFO";  break;
        case LogLevel::Warn:  color = fmt::color::yellow; label = "WARN";  break;
        case LogLevel::Error: color = fmt::color::red;    label = "ERROR"; break;
        case LogLevel::Debug: color = fmt::color::cyan;   label = "DEBUG"; break;
    }

    std::lock_guard<std::mutex> lock(log_mutex());
    fmt::print(fmt::fg(fmt::color::gray), "[{:%H:%M:%S}] ", time);
    fmt::print(fmt::fg(color), "{}: ", label);
    fmt::print("{}", msg);
    if (!fname.empty())
        fmt::print(fmt::fg(fmt::color::gray), " [{}:{}]", fname, line);
    fmt::print("\n");
}

struct Logger {
    std::string name;

    explicit Logger(std::string_view n) : name(n) {}

    void info (std::string_view msg, std::string_view file = "", int line = 0) const { nova_log(LogLevel::Info,  name, msg, file, line); }
    void warn (std::string_view msg, std::string_view file = "", int line = 0) const { nova_log(LogLevel::Warn,  name, msg, file, line); }
    void error(std::string_view msg, std::string_view file = "", int line = 0) const { nova_log(LogLevel::Error, name, msg, file, line); }
    void debug(std::string_view msg, std::string_view file = "", int line = 0) const { nova_log(LogLevel::Debug, name, msg, file, line); }
};

} // namespace Nova::Core

// ── Macros ────────────────────────────────────────────────────────────────────

#ifdef NOVA_LOGGING_DISABLE
    #define NOVA_INFO(logger, fmt, ...)  ((void)0)
    #define NOVA_WARN(logger, fmt, ...)  ((void)0)
    #define NOVA_ERROR(logger, fmt, ...) ((void)0)
    #define NOVA_DEBUG(logger, fmt, ...) ((void)0)
    #define NINFO(fmt, ...)  ((void)0)
    #define NWARN(fmt, ...)  ((void)0)
    #define NERROR(fmt, ...) ((void)0)
    #define NDEBUG(fmt, ...) ((void)0)
#else
    #define NINFO(msg, ...)  oLogger().info(::fmt::format(msg,  ##__VA_ARGS__), __FILE__, __LINE__)
    #define NWARN(msg, ...)  oLogger().warn(::fmt::format(msg,  ##__VA_ARGS__), __FILE__, __LINE__)
    #define NERROR(msg, ...) oLogger().error(::fmt::format(msg, ##__VA_ARGS__), __FILE__, __LINE__)
    #define NDEBUG(msg, ...) oLogger().debug(::fmt::format(msg, ##__VA_ARGS__), __FILE__, __LINE__)

    #define NOVA_INFO(logger, msg, ...)  (logger).info(::fmt::format(msg, ##__VA_ARGS__), __FILE__, __LINE__)
    #define NOVA_WARN(logger, msg, ...)  (logger).warn(::fmt::format(msg, ##__VA_ARGS__), __FILE__, __LINE__)
    #define NOVA_ERROR(logger, msg, ...) (logger).error(::fmt::format(msg, ##__VA_ARGS__), __FILE__, __LINE__)
    #define NOVA_DEBUG(logger, msg, ...) (logger).debug(::fmt::format(msg, ##__VA_ARGS__), __FILE__, __LINE__)

#endif

#define NOVA_LOG_DEF(name) \
    inline static N::Log::Logger& oLogger() { \
        static N::Log::Logger logger{name}; \
        return logger; \
    }

namespace N::Log {
    inline Logger& globalLogger() {
        static Logger l{"Global"};
        return l;
    }
}

#define LOG_INFO(msg, ...)  N::Log::globalLogger().info(::fmt::format(msg,  ##__VA_ARGS__), __FILE__, __LINE__)
#define LOG_WARN(msg, ...)  N::Log::globalLogger().warn(::fmt::format(msg,  ##__VA_ARGS__), __FILE__, __LINE__)
#define LOG_ERROR(msg, ...) N::Log::globalLogger().error(::fmt::format(msg, ##__VA_ARGS__), __FILE__, __LINE__)
#define LOG_DEBUG(msg, ...) N::Log::globalLogger().debug(::fmt::format(msg, ##__VA_ARGS__), __FILE__, __LINE__)