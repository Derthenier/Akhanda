
module;

#include <atomic>
#include <string_view>
#include <source_location>
#include <format>
#include <chrono>
#include <thread>
#include <string>
#include <memory>
#include <functional>

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
// Log Levels (unchanged from original)
// =============================================================================

export namespace Akhanda::Logging {
    enum class LogLevel : uint8_t {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Fatal = 5,
        Count
    };

    constexpr std::string_view ToString(LogLevel level) noexcept {
        switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
        }
    }

    constexpr bool IsValidLevel(LogLevel level) noexcept {
        return level >= LogLevel::Trace && level < LogLevel::Count;
    }

    // Compile-time level filtering for zero-cost logging
    constexpr bool ShouldCompileLog([[maybe_unused]] LogLevel level) noexcept {
#ifdef AKH_DEBUG
        return true; // All levels in debug
#elif defined(AKH_PROFILE)
        return level >= LogLevel::Info; // Info and above in profile
#else
        return level >= LogLevel::Warning; // Warning and above in release
#endif
    }
}

// =============================================================================
// Log Entry Structure (simplified)
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
// Log Sink Interface (simplified)
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

        // Identification
        virtual std::string_view GetName() const = 0;
    };
}

// =============================================================================
// Log Channel (same public API)
// =============================================================================

export namespace Akhanda::Logging {
    class LogChannel {
    public:
        explicit LogChannel(std::string_view name) noexcept;

        // Non-copyable, movable
        LogChannel(const LogChannel&) = delete;
        LogChannel& operator=(const LogChannel&) = delete;
        LogChannel(LogChannel&&) = default;
        LogChannel& operator=(LogChannel&&) = default;

        // Core logging methods (unchanged API)
        void Log(LogLevel level, std::string_view message, const std::source_location& location = std::source_location::current()) const;
        void Log(LogLevel level, std::wstring_view message, const std::source_location& location = std::source_location::current()) const;

        // Formatted logging (unchanged API)
        template<typename... Args>
        void LogFormat(LogLevel level, std::format_string<Args...> fmt, Args&&... args) const;
        template<typename... Args>
        void LogFormat(LogLevel level, std::wformat_string<Args...> fmt, Args&&... args) const;

        // Convenience methods (unchanged API)
        void Debug(std::string_view message,
            const std::source_location& location = std::source_location::current()) const {
            Log(LogLevel::Debug, message, location);
        }

        void Info(std::string_view message,
            const std::source_location& location = std::source_location::current()) const {
            Log(LogLevel::Info, message, location);
        }

        void Warning(std::string_view message,
            const std::source_location& location = std::source_location::current()) const {
            Log(LogLevel::Warning, message, location);
        }

        void Error(std::string_view message,
            const std::source_location& location = std::source_location::current()) const {
            Log(LogLevel::Error, message, location);
        }

        void Fatal(std::string_view message,
            const std::source_location& location = std::source_location::current()) const {
            Log(LogLevel::Fatal, message, location);
        }

        // Formatted convenience methods (unchanged API)
        template<typename... Args>
        void DebugFormat(std::format_string<Args...> fmt, Args&&... args) const {
            LogFormat(LogLevel::Debug, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void InfoFormat(std::format_string<Args...> fmt, Args&&... args) const {
            LogFormat(LogLevel::Info, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void WarningFormat(std::format_string<Args...> fmt, Args&&... args) const {
            LogFormat(LogLevel::Warning, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void ErrorFormat(std::format_string<Args...> fmt, Args&&... args) const {
            LogFormat(LogLevel::Error, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void FatalFormat(std::format_string<Args...> fmt, Args&&... args) const {
            LogFormat(LogLevel::Fatal, fmt, std::forward<Args>(args)...);
        }

        // Configuration (unchanged API)
        void SetLevel(LogLevel level) noexcept;
        LogLevel GetLevel() const noexcept;

        // Query (unchanged API)
        std::string_view GetName() const noexcept;
        bool ShouldLog(LogLevel level) const noexcept;

        // Internal interface for LogManager (public but not intended for general use)
        void AddSink(std::shared_ptr<void> sink);  // Using void* to avoid spdlog dependency in header
        void RemoveSink(void* sink);

    private:
        std::string name_;
        std::atomic<LogLevel> level_;

        // spdlog logger instance (implementation detail)
        class Impl;
        std::unique_ptr<Impl> impl_;

        // Friend declarations for internal access
        friend class LogManager;
    };
}

// =============================================================================
// Log Manager (same public API)
// =============================================================================

export namespace Akhanda::Logging {
    class LogManager {
        using MemoryAllocationCallback = std::function<void(size_t, const char*)>;
        using MemoryDeallocationCallback = std::function<void(size_t, const char*)>;

    public:
        static LogManager& Instance() noexcept;

        // Non-copyable, non-movable singleton
        LogManager(const LogManager&) = delete;
        LogManager& operator=(const LogManager&) = delete;
        LogManager(LogManager&&) = delete;
        LogManager& operator=(LogManager&&) = delete;

        // Lifecycle (unchanged API)
        void Initialize();
        void Shutdown();

        // Channel management (unchanged API)
        LogChannel& GetChannel(std::string_view name);
        void RemoveChannel(std::string_view name);

        // Sink management (unchanged API) 
        void AddSink(std::unique_ptr<ILogSink> sink);
        void RemoveSink(ILogSink* sink);
        void ClearSinks();

        // Global configuration (unchanged API)
        void SetGlobalLevel(LogLevel level);
        LogLevel GetGlobalLevel() const;

        // Core logging (called by channels - unchanged API)
        void Log(const LogEntry& entry);

        // Performance monitoring (unchanged API)
        struct Statistics {
            std::atomic<uint64_t> messagesLogged{ 0 };
            std::atomic<uint64_t> messagesDropped{ 0 };
            std::atomic<uint64_t> bytesWritten{ 0 };
            std::atomic<uint64_t> flushCount{ 0 };

            void Reset() noexcept {
                messagesLogged.store(0, std::memory_order_relaxed);
                messagesDropped.store(0, std::memory_order_relaxed);
                bytesWritten.store(0, std::memory_order_relaxed);
                flushCount.store(0, std::memory_order_relaxed);
            }
        };

        const Statistics& GetStatistics() const noexcept;
        void ResetStatistics() noexcept;
        void UpdateStatistics(size_t messageBytes);

        // Thread safety and performance (unchanged API)
        void Flush();
        void SetAsyncMode(bool enabled);
        bool IsAsyncMode() const;

        // Editor integration
        using EditorLogCallback = std::function<void(const LogEntry&)>;
        void SetEditorCallback(EditorLogCallback callback);
        void ClearEditorCallback();
        void SetMemoryTrackingCallbacks(MemoryAllocationCallback alloc, MemoryDeallocationCallback dealloc);
        void ClearMemoryTrackingCallbacks();

    private:
        LogManager();
        ~LogManager();

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}

// =============================================================================
// Scoped Logging Utilities (unchanged API)
// =============================================================================

export namespace Akhanda::Logging {
    // RAII scope logger for timing and call tracing
    class ScopeLogger {
    public:
        ScopeLogger(LogChannel& channel, std::string_view scopeName,
            const std::source_location& location = std::source_location::current()) noexcept;

        ~ScopeLogger() noexcept;

        // Non-copyable, non-movable
        ScopeLogger(const ScopeLogger&) = delete;
        ScopeLogger& operator=(const ScopeLogger&) = delete;
        ScopeLogger(ScopeLogger&&) = delete;
        ScopeLogger& operator=(ScopeLogger&&) = delete;

    private:
        LogChannel& channel_;
        std::string scopeName_;
        std::chrono::high_resolution_clock::time_point startTime_;
        std::source_location location_;
    };
}

// =============================================================================
// Template Implementations
// =============================================================================

export namespace Akhanda::Logging {
    template<typename... Args>
    void LogChannel::LogFormat(LogLevel level, std::format_string<Args...> fmt, Args&&... args) const {

        // Early exit if this level won't be logged
        if (!ShouldLog(level)) {
            return;
        }

        try {
            // Use std::format for safe formatting
            auto formatted = std::format(fmt, std::forward<Args>(args)...);
            Log(level, formatted, std::source_location::current());
        }
        catch (const std::exception&) {
            // Format error - log the error instead
            Log(LogLevel::Error, "LOG FORMAT ERROR: Invalid format string", std::source_location::current());
        }
    }
    
    template<typename... Args>
    void LogChannel::LogFormat(LogLevel level, std::wformat_string<Args...> fmt, Args&&... args) const {

        // Early exit if this level won't be logged
        if (!ShouldLog(level)) {
            return;
        }

        try {
            // Use std::format for safe formatting
            auto formatted = std::format(fmt, std::forward<Args>(args)...);
            Log(level, formatted, std::source_location::current());
        }
        catch (const std::exception&) {
            // Format error - log the error instead
            Log(LogLevel::Error, "LOG FORMAT ERROR: Invalid format string", std::source_location::current());
        }
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