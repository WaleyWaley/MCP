#pragma once

#include "logger/AsyncLogger.h"
#include "logger/LogLevel.h"

#include <chrono>
#include <functional>
#include <memory>
#include <source_location>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

namespace logger_macro_detail {

inline auto to_ref(AsyncLogger& logger) noexcept -> AsyncLogger& { return logger; }
inline auto to_ref(AsyncLogger* logger) noexcept -> AsyncLogger& { return *logger; }
inline auto to_ref(const std::shared_ptr<AsyncLogger>& logger) noexcept -> AsyncLogger& { return *logger; }

inline auto is_valid(const AsyncLogger&) noexcept -> bool { return true; }
inline auto is_valid(const AsyncLogger* logger) noexcept -> bool { return logger != nullptr; }
inline auto is_valid(const std::shared_ptr<AsyncLogger>& logger) noexcept -> bool {
    return static_cast<bool>(logger);
}

inline auto make_event(AsyncLogger& logger, LogLevel level, std::source_location loc) -> LogEvent {
    const auto tid = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    const auto now = SystemClock::to_time_t(SystemClock::now());
    return LogEvent(
        std::string{logger.getLoggerName()},
        level,
        0U,
        tid,
        std::string{"MainThread"},
        now,
        0U,
        loc
    );
}

} // namespace logger_macro_detail

#define LOG_LEVEL(logger, level, fmt, ...)                                                     \
    do {                                                                                        \
        if (::logger_macro_detail::is_valid((logger))) {                                       \
            auto& _cotton_logger = ::logger_macro_detail::to_ref((logger));                    \
            if (_cotton_logger.isLevelEnable((level))) {                                       \
                auto _cotton_event =                                                           \
                    ::logger_macro_detail::make_event(_cotton_logger, (level),                 \
                                                     std::source_location::current());          \
                _cotton_event.print((fmt) __VA_OPT__(, ) __VA_ARGS__);                         \
                _cotton_logger.append(std::move(_cotton_event));                               \
            }                                                                                   \
        }                                                                                       \
    } while (false)

#define LOG_INFO(logger, fmt, ...) LOG_LEVEL((logger), LogLevel::INFO, (fmt) __VA_OPT__(, ) __VA_ARGS__)
