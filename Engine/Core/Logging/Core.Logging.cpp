// Core.Logging.cpp - Simplified spdlog-based Implementation
module;

#define SPDLOG_WCHAR_TO_UTF8_SUPPORT

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/fmt/fmt.h>

#include <unordered_map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <functional>
#include <cstdio>

module Akhanda.Core.Logging;

// =============================================================================
// Forward Declarations
// =============================================================================

class LogManagerImpl;

using namespace Akhanda::Logging;

// =============================================================================
// spdlog Integration Utilities
// =============================================================================

namespace {
    // Convert our LogLevel to spdlog level
    constexpr spdlog::level::level_enum ToSpdlogLevel(LogLevel level) noexcept {
        switch (level) {
        case LogLevel::Debug:   return spdlog::level::debug;
        case LogLevel::Info:    return spdlog::level::info;
        case LogLevel::Warning: return spdlog::level::warn;
        case LogLevel::Error:   return spdlog::level::err;
        case LogLevel::Fatal:   return spdlog::level::critical;
        default:                return spdlog::level::info;
        }
    }

    // Convert spdlog level to our LogLevel
    constexpr LogLevel FromSpdlogLevel(spdlog::level::level_enum level) noexcept {
        switch (level) {
        case spdlog::level::debug:    return LogLevel::Debug;
        case spdlog::level::info:     return LogLevel::Info;
        case spdlog::level::warn:     return LogLevel::Warning;
        case spdlog::level::err:      return LogLevel::Error;
        case spdlog::level::critical: return LogLevel::Fatal;
        default:                      return LogLevel::Info;
        }
    }

    // Global async thread pool for all loggers
    std::shared_ptr<spdlog::details::thread_pool> g_threadPool = nullptr;
    std::atomic<bool> g_asyncMode{ true };

    // Optional memory tracking callback - can be set by external code
    std::function<void(size_t, const char*)> g_memoryAllocationCallback = nullptr;
    std::function<void(size_t, const char*)> g_memoryDeallocationCallback = nullptr;
}

// =============================================================================
// Editor Sink - Custom spdlog sink for editor integration
// =============================================================================

class EditorSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    using EditorCallback = std::function<void(const LogEntry&)>;

    void SetCallback(EditorCallback callback) {
        std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
        callback_ = std::move(callback);
    }

    void ClearCallback() {
        std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
        callback_ = nullptr;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (!callback_) return;

        // Convert spdlog message to our LogEntry format
        LogEntry entry;
        entry.level = FromSpdlogLevel(msg.level);
        entry.channel = msg.logger_name;

        // Extract message
        spdlog::memory_buf_t formatted;
        base_sink<std::mutex>::formatter_->format(msg, formatted);
        messageBuffer_ = std::string(formatted.data(), formatted.size());
        entry.message = messageBuffer_;

        entry.timestamp = std::chrono::high_resolution_clock::time_point(
            std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(
                msg.time.time_since_epoch()));
        entry.threadId = std::this_thread::get_id();

        // Source location (if available in future spdlog versions)
        entry.location = std::source_location::current();

        callback_(entry);
    }

    void flush_() override {
        // Editor callback is immediate, no buffering
    }

private:
    EditorCallback callback_;
    std::string messageBuffer_; // Temporary storage for message
};

// =============================================================================
// Optional Memory Tracking Sink - Uses callbacks instead of direct access
// =============================================================================

class OptionalMemoryTrackingSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    OptionalMemoryTrackingSink() = default;

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (!g_memoryAllocationCallback) return;

        // Track memory allocation for log message
        spdlog::memory_buf_t formatted;
        base_sink<std::mutex>::formatter_->format(msg, formatted);

        size_t messageSize = formatted.size();
        g_memoryAllocationCallback(messageSize, "LogMessage");

        // Immediately track deallocation (message is temporary)
        if (g_memoryDeallocationCallback) {
            g_memoryDeallocationCallback(messageSize, "LogMessage");
        }
    }

    void flush_() override {
        // No persistent storage, nothing to flush
    }
};

// =============================================================================
// Akhanda Sink Wrapper - Adapts our ILogSink to spdlog
// =============================================================================

class AkhandaSinkWrapper : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit AkhandaSinkWrapper(std::unique_ptr<ILogSink> akhandaSink)
        : akhandaSink_(std::move(akhandaSink)) {
        set_level(ToSpdlogLevel(akhandaSink_->GetLevel()));
    }

    ILogSink* GetAkhandaSink() const { return akhandaSink_.get(); }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (!akhandaSink_) return;

        // Convert to LogEntry
        LogEntry entry;
        entry.level = FromSpdlogLevel(msg.level);
        entry.channel = msg.logger_name;

        spdlog::memory_buf_t formatted;
        base_sink<std::mutex>::formatter_->format(msg, formatted);
        messageBuffer_ = std::string(formatted.data(), formatted.size());
        entry.message = messageBuffer_;

        entry.timestamp = std::chrono::high_resolution_clock::time_point(
            std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(
                msg.time.time_since_epoch()));
        entry.threadId = std::this_thread::get_id();
        entry.location = std::source_location::current();

        akhandaSink_->Write(entry);
    }

    void flush_() override {
        if (akhandaSink_) {
            akhandaSink_->Flush();
        }
    }

private:
    std::unique_ptr<ILogSink> akhandaSink_;
    std::string messageBuffer_;
};

// =============================================================================
// LogChannel Implementation
// =============================================================================

class LogChannel::Impl {
public:
    explicit Impl(std::string_view name)
        : spdlogLogger_(nullptr) {

        try {
            // Create appropriate logger based on async mode
            if (g_asyncMode.load(std::memory_order_relaxed) && g_threadPool) {
                // Async logger for performance
                spdlogLogger_ = std::make_shared<spdlog::async_logger>(
                    std::string(name),
                    spdlog::sinks_init_list{},
                    g_threadPool,
                    spdlog::async_overflow_policy::block
                );
            }
            else {
                // Synchronous logger
                spdlogLogger_ = std::make_shared<spdlog::logger>(
                    std::string(name),
                    spdlog::sinks_init_list{}
                );
            }

            // Set default level and pattern
            spdlogLogger_->set_level(spdlog::level::debug);
            spdlogLogger_->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%n] [%l] [thread %t] %v");

            // Register with spdlog
            spdlog::register_logger(spdlogLogger_);

#ifdef AKH_DEBUG
            // Debug output
            std::printf("Created logger '%.*s' with %zu sinks\n",
                static_cast<int>(name.size()), name.data(),
                spdlogLogger_->sinks().size());
#endif
        }
        catch (const std::exception& e) {
            // Fallback: create a basic logger or handle gracefully
#ifdef AKH_DEBUG
            std::printf("Failed to create logger '%.*s': %s\n",
                static_cast<int>(name.size()), name.data(), e.what());
#endif
        }
    }

    ~Impl() {
        if (spdlogLogger_) {
            spdlog::drop(spdlogLogger_->name());
        }
    }

    void Log(LogLevel level, std::string_view message, const std::source_location& location) {
        if (!spdlogLogger_) {
            // Fallback: direct console output if spdlog logger not available
            std::printf("[%s] %.*s\n", ToString(level).data(), static_cast<int>(message.size()), message.data());
            return;
        }

        auto spdLevel = ToSpdlogLevel(level);

        // Use spdlog's efficient logging with source location
        spdlogLogger_->log(spdlog::source_loc{ location.file_name(),
                                             static_cast<int>(location.line()),
                                             location.function_name() },
            spdLevel, "{}", message);

        // Update statistics through public interface
        LogManager::Instance().UpdateStatistics(message.length());
    }

    void Log(LogLevel level, std::wstring_view message, const std::source_location& location) {
        if (!spdlogLogger_) {
            return;
        }

        auto spdLevel = ToSpdlogLevel(level);

        // Use spdlog's efficient logging with source location
        spdlog::source_loc loc{ location.file_name(), static_cast<int>(location.line()), location.function_name() };
        spdlogLogger_->log(loc, spdLevel, L"{}", message);

        // Update statistics through public interface
        LogManager::Instance().UpdateStatistics(message.length());
    }

    void SetLevel(LogLevel level) {
        if (spdlogLogger_) {
            spdlogLogger_->set_level(ToSpdlogLevel(level));
        }
    }

    LogLevel GetLevel() const {
        if (!spdlogLogger_) return LogLevel::Info;
        return FromSpdlogLevel(spdlogLogger_->level());
    }

    bool ShouldLog(LogLevel level) const {
        if (!spdlogLogger_) return false;
        return spdlogLogger_->should_log(ToSpdlogLevel(level));
    }

    void AddSink(std::shared_ptr<spdlog::sinks::sink> sink) {
        if (spdlogLogger_ && sink) {
            spdlogLogger_->sinks().push_back(sink);
#ifdef AKH_DEBUG
            std::printf("Added sink to logger '%s' (now has %zu sinks)\n",
                spdlogLogger_->name().c_str(), spdlogLogger_->sinks().size());
#endif
        }
#ifdef AKH_DEBUG
        else {
            std::printf("Failed to add sink: logger=%p, sink=%p\n",
                spdlogLogger_.get(), sink.get());
        }
#endif
    }

    void RemoveSink(spdlog::sinks::sink* sink) {
        if (!spdlogLogger_) return;

        auto& sinks = spdlogLogger_->sinks();
        sinks.erase(std::remove_if(sinks.begin(), sinks.end(),
            [sink](const std::shared_ptr<spdlog::sinks::sink>& s) {
                return s.get() == sink;
            }), sinks.end());
    }

private:
    std::shared_ptr<spdlog::logger> spdlogLogger_;
};

// =============================================================================
// LogManager Implementation
// =============================================================================

class LogManager::Impl {
public:
    Impl() : initialized_(false) {
        statistics_.Reset();
    }

    ~Impl() {
        Shutdown();
    }

    void Initialize() {
        std::lock_guard<std::mutex> lock(initMutex_);
        if (initialized_) return;

        // Initialize spdlog thread pool for async logging
        if (g_asyncMode.load(std::memory_order_relaxed)) {
            g_threadPool = std::make_shared<spdlog::details::thread_pool>(
                8192,  // Queue size
                1      // Number of threads
            );
        }

        // Set global memory tracker reference (if callbacks are set)
        // Note: Memory tracking is optional and must be configured externally

        // Create default sinks
        CreateDefaultSinks();

        initialized_ = true;
    }

    void Shutdown() {
        std::lock_guard<std::mutex> lock(initMutex_);
        if (!initialized_) return;

        // Clear all channels
        {
            std::lock_guard<std::shared_mutex> channelLock(channelsMutex_);
            channels_.clear();
        }

        // Clear all sinks
        {
            std::lock_guard<std::shared_mutex> sinkLock(sinksMutex_);
            editorSink_.reset();
            memoryTrackingSink_.reset();
            akhandaSinks_.clear();
        }

        // Shutdown spdlog
        spdlog::shutdown();
        g_threadPool.reset();

        initialized_ = false;
    }

    LogChannel& GetChannel(std::string_view name) {
        std::shared_lock<std::shared_mutex> lock(channelsMutex_);

        auto it = channels_.find(std::string(name));
        if (it != channels_.end()) {
            return *it->second;
        }

        lock.unlock();

        // Create new channel
        std::lock_guard<std::shared_mutex> writeLock(channelsMutex_);

        // Double-check after acquiring write lock
        auto it2 = channels_.find(std::string(name));
        if (it2 != channels_.end()) {
            return *it2->second;
        }

        auto channel = std::make_unique<LogChannel>(name);

        // Add existing sinks to new channel
        {
            std::shared_lock<std::shared_mutex> sinkLock(sinksMutex_);

            // Add default console and MSVC sinks
            if (defaultConsoleSink_) {
                channel->AddSink(std::static_pointer_cast<void>(defaultConsoleSink_));
            }
            if (defaultMsvcSink_) {
                channel->AddSink(std::static_pointer_cast<void>(defaultMsvcSink_));
            }

            // Add other sinks
            if (editorSink_) {
                channel->AddSink(std::static_pointer_cast<void>(editorSink_));
            }
            if (memoryTrackingSink_) {
                channel->AddSink(std::static_pointer_cast<void>(memoryTrackingSink_));
            }
            for (const auto& wrapper : akhandaSinks_) {
                channel->AddSink(std::static_pointer_cast<void>(wrapper));
            }
        }

        auto& channelRef = *channel;
        channels_[std::string(name)] = std::move(channel);
        return channelRef;
    }

    void RemoveChannel(std::string_view name) {
        std::lock_guard<std::shared_mutex> lock(channelsMutex_);
        channels_.erase(std::string(name));
    }

    void AddSink(std::unique_ptr<ILogSink> sink) {
        std::lock_guard<std::shared_mutex> lock(sinksMutex_);

        auto wrapper = std::make_shared<AkhandaSinkWrapper>(std::move(sink));
        akhandaSinks_.push_back(wrapper);

        // Add to all existing channels
        std::shared_lock<std::shared_mutex> channelLock(channelsMutex_);
        for (const auto& [name, channel] : channels_) {
            channel->AddSink(std::static_pointer_cast<void>(wrapper));
        }
    }

    void RemoveSink(ILogSink* sink) {
        std::lock_guard<std::shared_mutex> lock(sinksMutex_);

        // Find and remove wrapper
        auto it = std::find_if(akhandaSinks_.begin(), akhandaSinks_.end(),
            [sink](const std::shared_ptr<AkhandaSinkWrapper>& wrapper) {
                return wrapper->GetAkhandaSink() == sink;
            });

        if (it != akhandaSinks_.end()) {
            auto& wrapper = *it;
            akhandaSinks_.erase(it);

            // Remove from all channels
            std::shared_lock<std::shared_mutex> channelLock(channelsMutex_);
            for (const auto& [name, channel] : channels_) {
                channel->RemoveSink(wrapper.get());
            }
        }
    }

    void ClearSinks() {
        std::lock_guard<std::shared_mutex> lock(sinksMutex_);
        akhandaSinks_.clear();

        // Remove from all channels
        std::shared_lock<std::shared_mutex> channelLock(channelsMutex_);
        for (const auto& [name, channel] : channels_) {
            // Keep editor and memory tracking sinks, remove only Akhanda sinks
            // This would require more complex logic - simplified for now
        }
    }

    void SetGlobalLevel(LogLevel level) {
        globalLevel_.store(level, std::memory_order_relaxed);
        spdlog::set_level(ToSpdlogLevel(level));
    }

    LogLevel GetGlobalLevel() const {
        return globalLevel_.load(std::memory_order_relaxed);
    }

    void SetEditorCallback(LogManager::EditorLogCallback callback) {
        std::lock_guard<std::shared_mutex> lock(sinksMutex_);

        if (!editorSink_) {
            editorSink_ = std::make_shared<EditorSink>();

            // Add to all existing channels
            std::shared_lock<std::shared_mutex> channelLock(channelsMutex_);
            for (const auto& [name, channel] : channels_) {
                channel->AddSink(std::static_pointer_cast<void>(editorSink_));
            }
        }

        editorSink_->SetCallback(std::move(callback));
    }

    void ClearEditorCallback() {
        std::shared_lock<std::shared_mutex> lock(sinksMutex_);
        if (editorSink_) {
            editorSink_->ClearCallback();
        }
    }

    void Flush() {
        spdlog::apply_all([](std::shared_ptr<spdlog::logger> logger) {
            logger->flush();
            });

        statistics_.flushCount.fetch_add(1, std::memory_order_relaxed);
    }

    void SetAsyncMode(bool enabled) {
        g_asyncMode.store(enabled, std::memory_order_relaxed);
    }

    bool IsAsyncMode() const {
        return g_asyncMode.load(std::memory_order_relaxed);
    }

    void UpdateStatistics(size_t messageBytes) {
        statistics_.messagesLogged.fetch_add(1, std::memory_order_relaxed);
        statistics_.bytesWritten.fetch_add(messageBytes, std::memory_order_relaxed);
    }

    const LogManager::Statistics& GetStatistics() const {
        return statistics_;
    }

    void ResetStatistics() {
        statistics_.Reset();
    }

    void SetMemoryTrackingCallbacks(LogManager::MemoryAllocationCallback allocCallback,
        LogManager::MemoryDeallocationCallback deallocCallback) {
        g_memoryAllocationCallback = std::move(allocCallback);
        g_memoryDeallocationCallback = std::move(deallocCallback);

        // Create memory tracking sink if it doesn't exist
        std::lock_guard<std::shared_mutex> lock(sinksMutex_);
        if (!memoryTrackingSink_ && g_memoryAllocationCallback) {
            memoryTrackingSink_ = std::make_shared<OptionalMemoryTrackingSink>();

            // Add to all existing channels
            std::shared_lock<std::shared_mutex> channelLock(channelsMutex_);
            for (const auto& [name, channel] : channels_) {
                channel->AddSink(std::static_pointer_cast<void>(memoryTrackingSink_));
            }
        }
    }

    void ClearMemoryTrackingCallbacks() {
        g_memoryAllocationCallback = nullptr;
        g_memoryDeallocationCallback = nullptr;

        // Remove memory tracking sink
        std::lock_guard<std::shared_mutex> lock(sinksMutex_);
        if (memoryTrackingSink_) {
            // Remove from all channels
            std::shared_lock<std::shared_mutex> channelLock(channelsMutex_);
            for (const auto& [name, channel] : channels_) {
                channel->RemoveSink(memoryTrackingSink_.get());
            }
            memoryTrackingSink_.reset();
        }
    }

private:
    void CreateDefaultSinks() {
        // Console sink for debug output
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(spdlog::level::debug);

        // MSVC output window sink for Visual Studio
        auto msvcSink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        msvcSink->set_level(spdlog::level::debug);

        // Store default sinks for new channels
        std::lock_guard<std::shared_mutex> lock(sinksMutex_);
        defaultConsoleSink_ = consoleSink;
        defaultMsvcSink_ = msvcSink;

        // Optional memory tracking sink
        if (g_memoryAllocationCallback) {
            memoryTrackingSink_ = std::make_shared<OptionalMemoryTrackingSink>();
        }

        // Apply to all existing channels
        std::shared_lock<std::shared_mutex> channelLock(channelsMutex_);
        for (const auto& [name, channel] : channels_) {
            channel->AddSink(std::static_pointer_cast<void>(consoleSink));
            channel->AddSink(std::static_pointer_cast<void>(msvcSink));
            if (memoryTrackingSink_) {
                channel->AddSink(std::static_pointer_cast<void>(memoryTrackingSink_));
            }
        }
    }

    std::atomic<bool> initialized_;
    std::atomic<LogLevel> globalLevel_{ LogLevel::Debug };

    mutable std::shared_mutex channelsMutex_;
    mutable std::shared_mutex sinksMutex_;
    std::mutex initMutex_;

    std::unordered_map<std::string, std::unique_ptr<LogChannel>> channels_;

    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> defaultConsoleSink_;
    std::shared_ptr<spdlog::sinks::msvc_sink_mt> defaultMsvcSink_;
    
    std::shared_ptr<EditorSink> editorSink_;
    std::shared_ptr<OptionalMemoryTrackingSink> memoryTrackingSink_;
    std::vector<std::shared_ptr<AkhandaSinkWrapper>> akhandaSinks_;

    LogManager::Statistics statistics_;

public:
    friend class LogManager;
    friend class LogManagerImpl;
};

// =============================================================================
// LogChannel Implementation
// =============================================================================

LogChannel::LogChannel(std::string_view name) noexcept
    : name_(name)
    , level_(LogLevel::Debug)
    , impl_(std::make_unique<Impl>(name)) {
}

void LogChannel::Log(LogLevel level, std::string_view message, const std::source_location& location) const {
    if (!ShouldLog(level)) return;
    impl_->Log(level, message, location);
}

void LogChannel::Log(LogLevel level, std::wstring_view message, const std::source_location& location) const {
    if (!ShouldLog(level)) return;
    impl_->Log(level, message, location);
}

void LogChannel::SetLevel(LogLevel level) noexcept {
    level_.store(level, std::memory_order_relaxed);
    impl_->SetLevel(level);
}

LogLevel LogChannel::GetLevel() const noexcept {
    return level_.load(std::memory_order_relaxed);
}

std::string_view LogChannel::GetName() const noexcept {
    return name_;
}

bool LogChannel::ShouldLog(LogLevel level) const noexcept {
    return level >= level_.load(std::memory_order_relaxed) && impl_->ShouldLog(level);
}

void LogChannel::AddSink(std::shared_ptr<void> sink) {
    auto spdlogSink = std::static_pointer_cast<spdlog::sinks::sink>(sink);
    impl_->AddSink(spdlogSink);
}

void LogChannel::RemoveSink(void* sink) {
    impl_->RemoveSink(static_cast<spdlog::sinks::sink*>(sink));
}

// =============================================================================
// LogManager Implementation
// =============================================================================

LogManager::LogManager() : impl_(std::make_unique<Impl>()) {}
LogManager::~LogManager() = default;

LogManager& LogManager::Instance() noexcept {
    static LogManager instance;
    return instance;
}

void LogManager::Initialize() { impl_->Initialize(); }
void LogManager::Shutdown() { impl_->Shutdown(); }

LogChannel& LogManager::GetChannel(std::string_view name) {
    return impl_->GetChannel(name);
}

void LogManager::RemoveChannel(std::string_view name) {
    impl_->RemoveChannel(name);
}

void LogManager::AddSink(std::unique_ptr<ILogSink> sink) {
    impl_->AddSink(std::move(sink));
}

void LogManager::RemoveSink(ILogSink* sink) {
    impl_->RemoveSink(sink);
}

void LogManager::ClearSinks() {
    impl_->ClearSinks();
}

void LogManager::SetGlobalLevel(LogLevel level) {
    impl_->SetGlobalLevel(level);
}

LogLevel LogManager::GetGlobalLevel() const {
    return impl_->GetGlobalLevel();
}

void LogManager::Log(const LogEntry& entry) {
    // This method is now mostly unused as channels log directly through spdlog
    // Keep for API compatibility
    impl_->UpdateStatistics(entry.message.length());
}

const LogManager::Statistics& LogManager::GetStatistics() const noexcept {
    return impl_->GetStatistics();
}

void LogManager::ResetStatistics() noexcept {
    impl_->ResetStatistics();
}

void LogManager::UpdateStatistics(size_t messageBytes) {
    // This method is now mostly unused as channels log directly through spdlog
    // Keep for API compatibility
    impl_->UpdateStatistics(messageBytes);
}

void LogManager::Flush() {
    impl_->Flush();
}

void LogManager::SetAsyncMode(bool enabled) {
    impl_->SetAsyncMode(enabled);
}

bool LogManager::IsAsyncMode() const {
    return impl_->IsAsyncMode();
}

void LogManager::SetEditorCallback(EditorLogCallback callback) {
    impl_->SetEditorCallback(std::move(callback));
}


void LogManager::SetMemoryTrackingCallbacks(LogManager::MemoryAllocationCallback allocCallback, LogManager::MemoryDeallocationCallback deallocCallback) {
    impl_->SetMemoryTrackingCallbacks(allocCallback, deallocCallback);
}

void LogManager::ClearMemoryTrackingCallbacks() {
    impl_->ClearMemoryTrackingCallbacks();
}

// =============================================================================
// ScopeLogger Implementation
// =============================================================================

ScopeLogger::ScopeLogger(LogChannel& channel, std::string_view scopeName,
    const std::source_location& location) noexcept
    : channel_(channel)
    , scopeName_(scopeName)
    , startTime_(std::chrono::high_resolution_clock::now())
    , location_(location) {

    if (channel_.ShouldLog(LogLevel::Debug)) {
        std::string message = std::format("Entering scope: {}", scopeName_);
        channel_.Log(LogLevel::Debug, message, location_);
    }
}

ScopeLogger::~ScopeLogger() noexcept {
    if (channel_.ShouldLog(LogLevel::Debug)) {
        const auto endTime = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_);

        std::string message = std::format("Exiting scope: {} ({}μs)", scopeName_, duration.count());
        channel_.Log(LogLevel::Debug, message, location_);
    }
}