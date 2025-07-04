module;

#include <atomic>
#include <string_view>
#include <source_location>
#include <format>
#include <chrono>
#include <thread>
#include <string>

export module Akhanda.Core.Logging;

// =============================================================================
// Forward Declarations
// =============================================================================

export namespace Akhanda::Logging {
    class Logger;
    class LogChannel;
    class LogMessage;
    struct LogEntry;
    class ILogSink;
    class LogManager;
}

// =============================================================================
// Log Levels
// =============================================================================

export namespace Akhanda::Logging {
    enum class LogLevel : uint8_t {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Fatal = 4,
        Count
    };

    constexpr std::string_view ToString(LogLevel level) noexcept {
        switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
        }
    }

    constexpr bool IsValidLevel(LogLevel level) noexcept {
        return level >= LogLevel::Debug && level < LogLevel::Count;
    }
}

// =============================================================================
// Log Entry Structure
// =============================================================================

export namespace Akhanda::Logging {
    struct LogEntry {
        LogLevel level;
        std::string_view channel;
        std::string_view message;
        std::chrono::high_resolution_clock::time_point timestamp;
        std::thread::id threadId;
        std::source_location location;

        LogEntry() = default;

        LogEntry(LogLevel lvl, std::string_view ch, std::string_view msg,
            const std::source_location& loc = std::source_location::current()) noexcept
            : level(lvl)
            , channel(ch)
            , message(msg)
            , timestamp(std::chrono::high_resolution_clock::now())
            , threadId(std::this_thread::get_id())
            , location(loc) {
        }
    };
}

// =============================================================================
// Log Sink Interface
// =============================================================================

export namespace Akhanda::Logging {
    class ILogSink {
    public:
        virtual ~ILogSink() = default;

        // Core sink interface
        virtual void Write(const LogEntry& entry) = 0;
        virtual void Flush() = 0;

        // Configuration
        virtual void SetLevel(LogLevel minLevel) = 0;
        virtual LogLevel GetLevel() const = 0;

        // Thread safety
        virtual bool IsThreadSafe() const = 0;

        // Performance hints
        virtual bool SupportsAsync() const = 0;
        virtual size_t GetPreferredBatchSize() const { return 1; }
    };
}

// =============================================================================
// Log Channel
// =============================================================================

export namespace Akhanda::Logging {
    class LogChannel {
    public:
        explicit LogChannel(std::string_view name) noexcept
            : name_(name)
            , level_(LogLevel::Debug) {
        }

        // Non-copyable, movable
        LogChannel(const LogChannel&) = delete;
        LogChannel& operator=(const LogChannel&) = delete;
        LogChannel(LogChannel&&) = default;
        LogChannel& operator=(LogChannel&&) = default;

        // Core logging methods
        void Log(LogLevel level, std::string_view message,
            const std::source_location& location = std::source_location::current()) const;

        // Formatted logging
        template<typename... Args>
        void LogFormat(LogLevel level, std::format_string<Args...> fmt, Args&&... args,
            const std::source_location& location = std::source_location::current()) const;

        // Convenience methods
        void Debug(std::string_view message, const std::source_location& location = std::source_location::current()) const {
            Log(LogLevel::Debug, message, location);
        }

        void Info(std::string_view message, const std::source_location& location = std::source_location::current()) const {
            Log(LogLevel::Info, message, location);
        }

        void Warning(std::string_view message, const std::source_location& location = std::source_location::current()) const {
            Log(LogLevel::Warning, message, location);
        }

        void Error(std::string_view message, const std::source_location& location = std::source_location::current()) const {
            Log(LogLevel::Error, message, location);
        }

        void Fatal(std::string_view message, const std::source_location& location = std::source_location::current()) const {
            Log(LogLevel::Fatal, message, location);
        }

        // Formatted convenience methods
        template<typename... Args>
        void DebugFormat(std::format_string<Args...> fmt, Args&&... args,
            const std::source_location& location = std::source_location::current()) const {
            LogFormat(LogLevel::Debug, fmt, std::forward<Args>(args)..., location);
        }

        template<typename... Args>
        void InfoFormat(std::format_string<Args...> fmt, Args&&... args,
            const std::source_location& location = std::source_location::current()) const {
            LogFormat(LogLevel::Info, fmt, std::forward<Args>(args)..., location);
        }

        template<typename... Args>
        void WarningFormat(std::format_string<Args...> fmt, Args&&... args,
            const std::source_location& location = std::source_location::current()) const {
            LogFormat(LogLevel::Warning, fmt, std::forward<Args>(args)..., location);
        }

        template<typename... Args>
        void ErrorFormat(std::format_string<Args...> fmt, Args&&... args,
            const std::source_location& location = std::source_location::current()) const {
            LogFormat(LogLevel::Error, fmt, std::forward<Args>(args)..., location);
        }

        template<typename... Args>
        void FatalFormat(std::format_string<Args...> fmt, Args&&... args,
            const std::source_location& location = std::source_location::current()) const {
            LogFormat(LogLevel::Fatal, fmt, std::forward<Args>(args)..., location);
        }

        // Configuration
        void SetLevel(LogLevel level) noexcept { level_.store(level, std::memory_order_relaxed); }
        LogLevel GetLevel() const noexcept { return level_.load(std::memory_order_relaxed); }

        // Query
        std::string_view GetName() const noexcept { return name_; }
        bool ShouldLog(LogLevel level) const noexcept {
            return level >= level_.load(std::memory_order_relaxed);
        }

    private:
        std::string_view name_;
        std::atomic<LogLevel> level_;
    };
}

// =============================================================================
// Log Manager
// =============================================================================

export namespace Akhanda::Logging {
    class LogManager {
    public:
        static LogManager& Instance() noexcept;

        // Non-copyable, non-movable singleton
        LogManager(const LogManager&) = delete;
        LogManager& operator=(const LogManager&) = delete;
        LogManager(LogManager&&) = delete;
        LogManager& operator=(LogManager&&) = delete;

        // Lifecycle
        void Initialize();
        void Shutdown();

        // Channel management
        LogChannel& GetChannel(std::string_view name);
        void RemoveChannel(std::string_view name);

        // Sink management
        void AddSink(std::unique_ptr<ILogSink> sink);
        void RemoveSink(ILogSink* sink);
        void ClearSinks();

        // Global configuration
        void SetGlobalLevel(LogLevel level);
        LogLevel GetGlobalLevel() const;

        // Core logging (called by channels)
        void Log(const LogEntry& entry);

        // Performance monitoring
        struct Statistics {
            std::atomic<uint64_t> messagesLogged{ 0 };
            std::atomic<uint64_t> messagesDropped{ 0 };
            std::atomic<uint64_t> bytesWritten{ 0 };
            std::atomic<uint64_t> flushCount{ 0 };
        };

        const Statistics& GetStatistics() const noexcept;
        void ResetStatistics() noexcept;

        // Thread safety and performance
        void Flush();
        void SetAsyncMode(bool enabled);
        bool IsAsyncMode() const;

    private:
        LogManager() = default;
        ~LogManager() = default;

        std::atomic<LogLevel> globalLevel_{ LogLevel::Debug };
        std::atomic<bool> asyncMode_{ true };
    };
}

// =============================================================================
// Scoped Logging Utilities
// =============================================================================

export namespace Akhanda::Logging {
    // RAII scope logger for timing and call tracing
    class ScopeLogger {
    public:
        ScopeLogger(LogChannel& channel, std::string_view scopeName,
            const std::source_location& location = std::source_location::current()) noexcept
            : channel_(channel)
            , scopeName_(scopeName)
            , startTime_(std::chrono::high_resolution_clock::now())
            , location_(location) {

            if (channel_.ShouldLog(LogLevel::Debug)) {
                std::string message = "Entering scope: ";
                message += scopeName_;
                channel_.Log(LogLevel::Debug, message, location_);
            }
        }

        ~ScopeLogger() noexcept {
            if (channel_.ShouldLog(LogLevel::Debug)) {
                const auto endTime = std::chrono::high_resolution_clock::now();
                const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_);

                std::string message = "Exiting scope: ";
                message += scopeName_;
                message += " (";
                message += std::to_string(duration.count());
                message += "μs)";

                channel_.Log(LogLevel::Debug, message, location_);
            }
        }

        // Non-copyable, non-movable
        ScopeLogger(const ScopeLogger&) = delete;
        ScopeLogger& operator=(const ScopeLogger&) = delete;
        ScopeLogger(ScopeLogger&&) = delete;
        ScopeLogger& operator=(ScopeLogger&&) = delete;

    private:
        LogChannel& channel_;
        std::string_view scopeName_;
        std::chrono::high_resolution_clock::time_point startTime_;
        std::source_location location_;
    };
}

// =============================================================================
// Compile-Time Configuration
// =============================================================================

export namespace Akhanda::Logging {
    // Compile-time log level filtering
#ifdef AKH_DEBUG
    constexpr LogLevel COMPILE_TIME_LOG_LEVEL = LogLevel::Debug;
#elif defined(AKH_PROFILE)
    constexpr LogLevel COMPILE_TIME_LOG_LEVEL = LogLevel::Info;
#else
    constexpr LogLevel COMPILE_TIME_LOG_LEVEL = LogLevel::Warning;
#endif

    // Compile-time check for log level
    constexpr bool ShouldCompileLog(LogLevel level) noexcept {
        return level >= COMPILE_TIME_LOG_LEVEL;
    }
}

// =============================================================================
// Global Channel Access
// =============================================================================

export namespace Akhanda::Logging {
    // Predefined engine channels
    namespace Channels {
        inline LogChannel& Engine() { return LogManager::Instance().GetChannel("Engine"); }
        inline LogChannel& Renderer() { return LogManager::Instance().GetChannel("Renderer"); }
        inline LogChannel& Physics() { return LogManager::Instance().GetChannel("Physics"); }
        inline LogChannel& Audio() { return LogManager::Instance().GetChannel("Audio"); }
        inline LogChannel& AI() { return LogManager::Instance().GetChannel("AI"); }
        inline LogChannel& Networking() { return LogManager::Instance().GetChannel("Networking"); }
        inline LogChannel& Editor() { return LogManager::Instance().GetChannel("Editor"); }
        inline LogChannel& Game() { return LogManager::Instance().GetChannel("Game"); }
        inline LogChannel& Plugin() { return LogManager::Instance().GetChannel("Plugin"); }
    }
}

// Note: Macros will be provided in Core.Logging.hpp header file
// since macros cannot be exported from modules