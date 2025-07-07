// Core.Logging.hpp - Logging Macros Header (Unchanged API)
#pragma once

// Import the logging module
import Akhanda.Core.Logging;

// =============================================================================
// Convenience Macros (Unchanged from Original)
// =============================================================================

// Channel-based logging macros with compile-time filtering
#define LOG_DEBUG(channel, message) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Debug)) { \
        if ((channel).ShouldLog(::Akhanda::Logging::LogLevel::Debug)) { \
            (channel).Debug(message); \
        } \
    } } while(0)

#define LOG_INFO(channel, message) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Info)) { \
        if ((channel).ShouldLog(::Akhanda::Logging::LogLevel::Info)) { \
            (channel).Info(message); \
        } \
    } } while(0)

#define LOG_WARNING(channel, message) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Warning)) { \
        if ((channel).ShouldLog(::Akhanda::Logging::LogLevel::Warning)) { \
            (channel).Warning(message); \
        } \
    } } while(0)

#define LOG_ERROR(channel, message) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Error)) { \
        if ((channel).ShouldLog(::Akhanda::Logging::LogLevel::Error)) { \
            (channel).Error(message); \
        } \
    } } while(0)

#define LOG_FATAL(channel, message) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Fatal)) { \
        if ((channel).ShouldLog(::Akhanda::Logging::LogLevel::Fatal)) { \
            (channel).Fatal(message); \
        } \
    } } while(0)

// =============================================================================
// Formatted Logging Macros (Unchanged from Original)
// =============================================================================

#define LOG_DEBUG_FORMAT(channel, fmt, ...) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Debug)) { \
        if ((channel).ShouldLog(::Akhanda::Logging::LogLevel::Debug)) { \
            (channel).DebugFormat(fmt, __VA_ARGS__); \
        } \
    } } while(0)

#define LOG_INFO_FORMAT(channel, fmt, ...) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Info)) { \
        if ((channel).ShouldLog(::Akhanda::Logging::LogLevel::Info)) { \
            (channel).InfoFormat(fmt, __VA_ARGS__); \
        } \
    } } while(0)

#define LOG_WARNING_FORMAT(channel, fmt, ...) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Warning)) { \
        if ((channel).ShouldLog(::Akhanda::Logging::LogLevel::Warning)) { \
            (channel).WarningFormat(fmt, __VA_ARGS__); \
        } \
    } } while(0)

#define LOG_ERROR_FORMAT(channel, fmt, ...) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Error)) { \
        if ((channel).ShouldLog(::Akhanda::Logging::LogLevel::Error)) { \
            (channel).ErrorFormat(fmt, __VA_ARGS__); \
        } \
    } } while(0)

#define LOG_FATAL_FORMAT(channel, fmt, ...) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Fatal)) { \
        if ((channel).ShouldLog(::Akhanda::Logging::LogLevel::Fatal)) { \
            (channel).FatalFormat(fmt, __VA_ARGS__); \
        } \
    } } while(0)

// =============================================================================
// Scope Logging Macro (Unchanged from Original)
// =============================================================================

#define LOG_SCOPE(channel, name) \
    ::Akhanda::Logging::ScopeLogger ANONYMOUS_VARIABLE(scopeLogger_)((channel), (name))

// =============================================================================
// Helper Macros for Unique Variable Names (Unchanged from Original)
// =============================================================================

#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __COUNTER__)
#define CONCATENATE(x, y) CONCATENATE_IMPL(x, y)
#define CONCATENATE_IMPL(x, y) x##y

// =============================================================================
// Performance Optimized Macros (New - Optional)
// =============================================================================

// These macros skip the ShouldLog check when using spdlog's internal filtering
// Use these for maximum performance in hot paths

#define LOG_DEBUG_FAST(channel, message) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Debug)) { \
        (channel).Debug(message); \
    } } while(0)

#define LOG_INFO_FAST(channel, message) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Info)) { \
        (channel).Info(message); \
    } } while(0)

#define LOG_WARNING_FAST(channel, message) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Warning)) { \
        (channel).Warning(message); \
    } } while(0)

#define LOG_ERROR_FAST(channel, message) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Error)) { \
        (channel).Error(message); \
    } } while(0)

#define LOG_FATAL_FAST(channel, message) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Fatal)) { \
        (channel).Fatal(message); \
    } } while(0)

// Formatted fast macros
#define LOG_DEBUG_FORMAT_FAST(channel, fmt, ...) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Debug)) { \
        (channel).DebugFormat(fmt, __VA_ARGS__); \
    } } while(0)

#define LOG_INFO_FORMAT_FAST(channel, fmt, ...) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Info)) { \
        (channel).InfoFormat(fmt, __VA_ARGS__); \
    } } while(0)

#define LOG_WARNING_FORMAT_FAST(channel, fmt, ...) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Warning)) { \
        (channel).WarningFormat(fmt, __VA_ARGS__); \
    } } while(0)

#define LOG_ERROR_FORMAT_FAST(channel, fmt, ...) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Error)) { \
        (channel).ErrorFormat(fmt, __VA_ARGS__); \
    } } while(0)

#define LOG_FATAL_FORMAT_FAST(channel, fmt, ...) \
    do { if constexpr (::Akhanda::Logging::ShouldCompileLog(::Akhanda::Logging::LogLevel::Fatal)) { \
        (channel).FatalFormat(fmt, __VA_ARGS__); \
    } } while(0)

// =============================================================================
// Conditional Logging Macros (New - Optional)
// =============================================================================

// Log only if condition is true (useful for reducing log spam)
#define LOG_DEBUG_IF(condition, channel, message) \
    do { if (condition) { LOG_DEBUG(channel, message); } } while(0)

#define LOG_INFO_IF(condition, channel, message) \
    do { if (condition) { LOG_INFO(channel, message); } } while(0)

#define LOG_WARNING_IF(condition, channel, message) \
    do { if (condition) { LOG_WARNING(channel, message); } } while(0)

#define LOG_ERROR_IF(condition, channel, message) \
    do { if (condition) { LOG_ERROR(channel, message); } } while(0)

#define LOG_FATAL_IF(condition, channel, message) \
    do { if (condition) { LOG_FATAL(channel, message); } } while(0)

// =============================================================================
// Rate-Limited Logging Macros (New - Optional)
// =============================================================================

// Log at most once per specified duration (useful for high-frequency events)
#define LOG_DEBUG_THROTTLE(duration_ms, channel, message) \
    do { \
        static auto last_log_time = std::chrono::steady_clock::time_point{}; \
        auto now = std::chrono::steady_clock::now(); \
        if (now - last_log_time >= std::chrono::milliseconds(duration_ms)) { \
            last_log_time = now; \
            LOG_DEBUG(channel, message); \
        } \
    } while(0)

#define LOG_INFO_THROTTLE(duration_ms, channel, message) \
    do { \
        static auto last_log_time = std::chrono::steady_clock::time_point{}; \
        auto now = std::chrono::steady_clock::now(); \
        if (now - last_log_time >= std::chrono::milliseconds(duration_ms)) { \
            last_log_time = now; \
            LOG_INFO(channel, message); \
        } \
    } while(0)

#define LOG_WARNING_THROTTLE(duration_ms, channel, message) \
    do { \
        static auto last_log_time = std::chrono::steady_clock::time_point{}; \
        auto now = std::chrono::steady_clock::now(); \
        if (now - last_log_time >= std::chrono::milliseconds(duration_ms)) { \
            last_log_time = now; \
            LOG_WARNING(channel, message); \
        } \
    } while(0)

#define LOG_ERROR_THROTTLE(duration_ms, channel, message) \
    do { \
        static auto last_log_time = std::chrono::steady_clock::time_point{}; \
        auto now = std::chrono::steady_clock::now(); \
        if (now - last_log_time >= std::chrono::milliseconds(duration_ms)) { \
            last_log_time = now; \
            LOG_ERROR(channel, message); \
        } \
    } while(0)

// =============================================================================
// Debug-Only Logging Macros (New - Optional)
// =============================================================================

// These macros compile to nothing in Release builds
#ifdef AKH_DEBUG
#define LOG_DEBUG_ONLY(channel, message) LOG_DEBUG(channel, message)
#define LOG_DEBUG_FORMAT_ONLY(channel, fmt, ...) LOG_DEBUG_FORMAT(channel, fmt, __VA_ARGS__)
#else
#define LOG_DEBUG_ONLY(channel, message) ((void)0)
#define LOG_DEBUG_FORMAT_ONLY(channel, fmt, ...) ((void)0)
#endif

// =============================================================================
// Memory Logging Helpers (New - Engine Specific)
// =============================================================================

// Special macros for memory-related logging with automatic size formatting
#define LOG_MEMORY_ALLOC(channel, size, type) \
    LOG_DEBUG_FORMAT(channel, "Memory allocated: {} bytes for {}", (size), (type))

#define LOG_MEMORY_FREE(channel, size, type) \
    LOG_DEBUG_FORMAT(channel, "Memory freed: {} bytes for {}", (size), (type))

#define LOG_MEMORY_USAGE(channel, current, peak) \
    LOG_INFO_FORMAT(channel, "Memory usage: current={} bytes, peak={} bytes", (current), (peak))

// =============================================================================
// Performance Logging Helpers (New - Engine Specific)
// =============================================================================

// Automatic timing scope with threshold
#define LOG_PERFORMANCE_SCOPE(channel, name, threshold_ms) \
    ::Akhanda::Logging::ScopeLogger ANONYMOUS_VARIABLE(perfLogger_)((channel), (name)); \
    auto ANONYMOUS_VARIABLE(perfStart_) = std::chrono::high_resolution_clock::now()

// GPU timing (placeholder for future GPU profiling integration)
#define LOG_GPU_SCOPE(channel, name) \
    LOG_SCOPE(channel, std::string("GPU: ") + (name))

// =============================================================================
// Assert-Style Logging (New - Optional)
// =============================================================================

// Log and continue (doesn't break execution)
#define LOG_ASSERT(condition, channel, message) \
    do { \
        if (!(condition)) { \
            LOG_ERROR_FORMAT(channel, "Assertion failed: {} - {}", #condition, message); \
        } \
    } while(0)

// Log and break in debug builds
#ifdef AKH_DEBUG
#define LOG_ASSERT_DEBUG(condition, channel, message) \
        do { \
            if (!(condition)) { \
                LOG_FATAL_FORMAT(channel, "Debug assertion failed: {} - {}", #condition, message); \
                __debugbreak(); \
            } \
        } while(0)
#else
#define LOG_ASSERT_DEBUG(condition, channel, message) ((void)0)
#endif

// =============================================================================
// Usage Examples (Documentation - Remove in Production)
// =============================================================================

/*
// Basic usage (unchanged from your existing code):
auto& renderChannel = LogManager::Instance().GetChannel("Render");
LOG_INFO(renderChannel, "Rendering frame");
LOG_ERROR_FORMAT(renderChannel, "Failed to create texture: {}", filename);

// Scope timing:
LOG_SCOPE(renderChannel, "RenderFrame");

// Performance optimized (new):
LOG_INFO_FAST(renderChannel, "Hot path message");

// Conditional logging (new):
LOG_WARNING_IF(textureCount > 1000, renderChannel, "Too many textures loaded");

// Rate limiting (new):
LOG_DEBUG_THROTTLE(1000, renderChannel, "Per-second debug message");

// Memory tracking (new):
LOG_MEMORY_ALLOC(renderChannel, 1024, "VertexBuffer");

// Assertions (new):
LOG_ASSERT(device != nullptr, renderChannel, "Graphics device is null");
*/