// Core.Logging.cpp - Core Implementation
module;

#include <array>
#include <atomic>
#include <condition_variable>
#include <format>
#include <memory>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

module Akhanda.Core.Logging;

using namespace Akhanda::Logging;

// =============================================================================
// Internal Implementation Details
// =============================================================================

namespace {
    // Pre-allocated string pools for zero-allocation logging
    class StringPool {
    public:
        static constexpr size_t SMALL_STRING_SIZE = 256;
        static constexpr size_t LARGE_STRING_SIZE = 1024;
        static constexpr size_t POOL_SIZE = 1000;

        struct PooledString {
            std::array<char, SMALL_STRING_SIZE> data;
            size_t length = 0;
            std::atomic<bool> inUse{ false };

            // Make movable for std::vector
            PooledString() = default;

            PooledString(PooledString&& other) noexcept
                : data(std::move(other.data))
                , length(other.length)
                , inUse(other.inUse.load()) {
                other.length = 0;
                other.inUse.store(false);
            }

            PooledString& operator=(PooledString&& other) noexcept {
                if (this != &other) {
                    data = std::move(other.data);
                    length = other.length;
                    inUse.store(other.inUse.load());
                    other.length = 0;
                    other.inUse.store(false);
                }
                return *this;
            }

            // Delete copy operations
            PooledString(const PooledString&) = delete;
            PooledString& operator=(const PooledString&) = delete;
        };

        struct LargePooledString {
            std::array<char, LARGE_STRING_SIZE> data;
            size_t length = 0;
            std::atomic<bool> inUse{ false };

            // Make movable for std::vector
            LargePooledString() = default;

            LargePooledString(LargePooledString&& other) noexcept
                : data(std::move(other.data))
                , length(other.length)
                , inUse(other.inUse.load()) {
                other.length = 0;
                other.inUse.store(false);
            }

            LargePooledString& operator=(LargePooledString&& other) noexcept {
                if (this != &other) {
                    data = std::move(other.data);
                    length = other.length;
                    inUse.store(other.inUse.load());
                    other.length = 0;
                    other.inUse.store(false);
                }
                return *this;
            }

            // Delete copy operations
            LargePooledString(const LargePooledString&) = delete;
            LargePooledString& operator=(const LargePooledString&) = delete;
        };

        StringPool() {
            // Pre-allocate vectors to avoid reallocation
            smallPool_.reserve(POOL_SIZE);
            largePool_.reserve(POOL_SIZE / 4);

            // Use resize instead of emplace_back to avoid moves during construction
            smallPool_.resize(POOL_SIZE);
            largePool_.resize(POOL_SIZE / 4);
        }

        PooledString* AcquireSmall() {
            for (auto& str : smallPool_) {
                bool expected = false;
                if (str.inUse.compare_exchange_weak(expected, true, std::memory_order_acquire)) {
                    str.length = 0;
                    return &str;
                }
            }
            return nullptr; // Pool exhausted
        }

        LargePooledString* AcquireLarge() {
            for (auto& str : largePool_) {
                bool expected = false;
                if (str.inUse.compare_exchange_weak(expected, true, std::memory_order_acquire)) {
                    str.length = 0;
                    return &str;
                }
            }
            return nullptr; // Pool exhausted
        }

        void Release(PooledString* str) {
            if (str) {
                str->inUse.store(false, std::memory_order_release);
            }
        }

        void Release(LargePooledString* str) {
            if (str) {
                str->inUse.store(false, std::memory_order_release);
            }
        }

    private:
        std::vector<PooledString> smallPool_;
        std::vector<LargePooledString> largePool_;
    };

    // Global string pool instance
    StringPool& GetStringPool() {
        static StringPool instance;
        return instance;
    }

    // Lock-free ring buffer for async logging
    template<typename T, size_t Sized>
    class LockFreeRingBuffer {
        static_assert((Sized& (Sized - 1)) == 0, "Size must be power of 2");

    public:
        bool Push(T&& item) {
            const size_t currentTail = tail_.load(std::memory_order_relaxed);
            const size_t nextTail = (currentTail + 1) & (Sized - 1);

            if (nextTail == head_.load(std::memory_order_acquire)) {
                return false; // Buffer full
            }

            buffer_[currentTail] = std::move(item);
            tail_.store(nextTail, std::memory_order_release);
            return true;
        }

        bool Pop(T& item) {
            const size_t currentHead = head_.load(std::memory_order_relaxed);

            if (currentHead == tail_.load(std::memory_order_acquire)) {
                return false; // Buffer empty
            }

            item = std::move(buffer_[currentHead]);
            head_.store((currentHead + 1) & (Sized - 1), std::memory_order_release);
            return true;
        }

        bool IsEmpty() const {
            return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
        }

        size_t Size() const {
            const size_t h = head_.load(std::memory_order_acquire);
            const size_t t = tail_.load(std::memory_order_acquire);
            return (t - h) & (Sized - 1);
        }

    private:
        alignas(64) std::atomic<size_t> head_{ 0 };
        alignas(64) std::atomic<size_t> tail_{ 0 };
        std::array<T, Sized> buffer_;
    };

    // Async log entry with storage management
    struct AsyncLogEntry {
        LogEntry entry;
        std::unique_ptr<std::string> formattedMessage; // For long messages
        StringPool::PooledString* pooledMessage = nullptr; // For short messages
        StringPool::LargePooledString* largePooledMessage = nullptr; // For medium messages

        AsyncLogEntry() = default;

        AsyncLogEntry(AsyncLogEntry&& other) noexcept
            : entry(std::move(other.entry))
            , formattedMessage(std::move(other.formattedMessage))
            , pooledMessage(other.pooledMessage)
            , largePooledMessage(other.largePooledMessage) {
            other.pooledMessage = nullptr;
            other.largePooledMessage = nullptr;
        }

        AsyncLogEntry& operator=(AsyncLogEntry&& other) noexcept {
            if (this != &other) {
                Release();
                entry = std::move(other.entry);
                formattedMessage = std::move(other.formattedMessage);
                pooledMessage = other.pooledMessage;
                largePooledMessage = other.largePooledMessage;
                other.pooledMessage = nullptr;
                other.largePooledMessage = nullptr;
            }
            return *this;
        }

        ~AsyncLogEntry() {
            Release();
        }

        void Release() {
            if (pooledMessage) {
                GetStringPool().Release(pooledMessage);
                pooledMessage = nullptr;
            }
            if (largePooledMessage) {
                GetStringPool().Release(largePooledMessage);
                largePooledMessage = nullptr;
            }
            formattedMessage.reset();
        }

        std::string_view GetMessage() const {
            if (formattedMessage) {
                return *formattedMessage;
            }
            if (largePooledMessage) {
                return std::string_view(largePooledMessage->data.data(), largePooledMessage->length);
            }
            if (pooledMessage) {
                return std::string_view(pooledMessage->data.data(), pooledMessage->length);
            }
            return entry.message;
        }
    };
}

// =============================================================================
// LogChannel Implementation
// =============================================================================

void LogChannel::Log(LogLevel level, std::string_view message, const std::source_location& location) const {
    if (!ShouldLog(level)) {
        return;
    }

    LogEntry entry(level, name_, message, location);
    LogManager::Instance().Log(entry);
}

template<typename... Args>
void LogChannel::LogFormat(LogLevel level, std::format_string<Args...> fmt, Args&&... args,
    const std::source_location& location) const {
    if (!ShouldLog(level)) {
        return;
    }

    // Try to format into pooled string first
    auto& pool = GetStringPool();
    auto* pooledStr = pool.AcquireSmall();

    if (pooledStr) {
        try {
            auto result = std::format_to_n(pooledStr->data.data(), StringPool::SMALL_STRING_SIZE - 1,
                fmt, std::forward<Args>(args)...);
            if (result.out < pooledStr->data.data() + StringPool::SMALL_STRING_SIZE) {
                // Success - fits in small pool
                pooledStr->length = result.out - pooledStr->data.data();
                pooledStr->data[pooledStr->length] = '\0';

                LogEntry entry(level, name_,
                    std::string_view(pooledStr->data.data(), pooledStr->length), location);

                // Create async entry with pooled string
                AsyncLogEntry asyncEntry;
                asyncEntry.entry = entry;
                asyncEntry.pooledMessage = pooledStr;

                LogManager::Instance().Log(entry);
                return;
            }
        }
        catch (...) {
            // Format failed, release and try large pool
        }
        pool.Release(pooledStr);
    }

    // Try large pool
    auto* largePooledStr = pool.AcquireLarge();
    if (largePooledStr) {
        try {
            auto result = std::format_to_n(largePooledStr->data.data(), StringPool::LARGE_STRING_SIZE - 1,
                fmt, std::forward<Args>(args)...);
            if (result.out < largePooledStr->data.data() + StringPool::LARGE_STRING_SIZE) {
                // Success - fits in large pool
                largePooledStr->length = result.out - largePooledStr->data.data();
                largePooledStr->data[largePooledStr->length] = '\0';

                LogEntry entry(level, name_,
                    std::string_view(largePooledStr->data.data(), largePooledStr->length), location);

                LogManager::Instance().Log(entry);
                return;
            }
        }
        catch (...) {
            // Format failed, release and allocate
        }
        pool.Release(largePooledStr);
    }

    // Fall back to heap allocation
    try {
        auto formatted = std::format(fmt, std::forward<Args>(args)...);
        LogEntry entry(level, name_, formatted, location);
        LogManager::Instance().Log(entry);
    }
    catch (const std::exception&) {
        // Format failed completely - log error
        LogEntry entry(LogLevel::Error, name_, "LOG FORMAT ERROR: Invalid format string", location);
        LogManager::Instance().Log(entry);
    }
}

// =============================================================================
// LogManager Implementation
// =============================================================================

class LogManagerImpl {
public:
    LogManagerImpl() : asyncBuffer_{}, workerRunning_{ false } {}

    ~LogManagerImpl() {
        Shutdown();
    }

    void Initialize() {
        std::lock_guard<std::mutex> lock(initMutex_);
        if (initialized_) {
            return;
        }

        // Start async worker thread
        workerRunning_.store(true, std::memory_order_release);
        workerThread_ = std::thread(&LogManagerImpl::AsyncWorker, this);

        initialized_ = true;
    }

    void Shutdown() {
        std::lock_guard<std::mutex> lock(initMutex_);
        if (!initialized_) {
            return;
        }

        // Stop async worker
        workerRunning_.store(false, std::memory_order_release);
        workerCondition_.notify_all();

        if (workerThread_.joinable()) {
            workerThread_.join();
        }

        // Flush remaining messages
        ProcessRemainingMessages();

        // Clear all sinks
        {
            std::lock_guard<std::shared_mutex> sinksLock(sinksMutex_);
            sinks_.clear();
        }

        // Clear all channels
        {
            std::lock_guard<std::shared_mutex> channelsLock(channelsMutex_);
            channels_.clear();
        }

        initialized_ = false;
    }

    LogChannel& GetChannel(std::string_view name) {
        // Try read-only access first
        {
            std::shared_lock<std::shared_mutex> lock(channelsMutex_);
            auto it = channels_.find(std::string(name));
            if (it != channels_.end()) {
                return *it->second;
            }
        }

        // Need to create new channel
        std::lock_guard<std::shared_mutex> lock(channelsMutex_);

        // Double-check in case another thread created it
        auto it = channels_.find(std::string(name));
        if (it != channels_.end()) {
            return *it->second;
        }

        // Create new channel
        auto channel = std::make_unique<LogChannel>(name);
        auto* channelPtr = channel.get();
        channels_[std::string(name)] = std::move(channel);

        return *channelPtr;
    }

    void RemoveChannel(std::string_view name) {
        std::lock_guard<std::shared_mutex> lock(channelsMutex_);
        channels_.erase(std::string(name));
    }

    void AddSink(std::unique_ptr<ILogSink> sink) {
        std::lock_guard<std::shared_mutex> lock(sinksMutex_);
        sinks_.push_back(std::move(sink));
    }

    void RemoveSink(ILogSink* sink) {
        std::lock_guard<std::shared_mutex> lock(sinksMutex_);
        sinks_.erase(
            std::remove_if(sinks_.begin(), sinks_.end(),
                [sink](const std::unique_ptr<ILogSink>& ptr) {
                    return ptr.get() == sink;
                }),
            sinks_.end());
    }

    void ClearSinks() {
        std::lock_guard<std::shared_mutex> lock(sinksMutex_);
        sinks_.clear();
    }

    void SetGlobalLevel(LogLevel level) {
        globalLevel_.store(level, std::memory_order_relaxed);
    }

    LogLevel GetGlobalLevel() const {
        return globalLevel_.load(std::memory_order_relaxed);
    }

    void Log(const LogEntry& entry) {
        // Check global level first
        if (entry.level < globalLevel_.load(std::memory_order_relaxed)) {
            return;
        }

        stats_.messagesLogged.fetch_add(1, std::memory_order_relaxed);

        if (asyncMode_.load(std::memory_order_relaxed)) {
            LogAsync(entry);
        }
        else {
            LogSync(entry);
        }
    }

    const LogManager::Statistics& GetStatistics() const {
        return stats_;
    }

    void ResetStatistics() {
        stats_.messagesLogged.store(0, std::memory_order_relaxed);
        stats_.messagesDropped.store(0, std::memory_order_relaxed);
        stats_.bytesWritten.store(0, std::memory_order_relaxed);
        stats_.flushCount.store(0, std::memory_order_relaxed);
    }

    void Flush() {
        if (asyncMode_.load(std::memory_order_relaxed)) {
            // Wait for async queue to empty
            while (!asyncBuffer_.IsEmpty()) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }

        // Flush all sinks
        std::shared_lock<std::shared_mutex> lock(sinksMutex_);
        for (auto& sink : sinks_) {
            sink->Flush();
        }

        stats_.flushCount.fetch_add(1, std::memory_order_relaxed);
    }

    void SetAsyncMode(bool enabled) {
        asyncMode_.store(enabled, std::memory_order_relaxed);
    }

    bool IsAsyncMode() const {
        return asyncMode_.load(std::memory_order_relaxed);
    }

private:
    void LogAsync(const LogEntry& entry) {
        AsyncLogEntry asyncEntry;
        asyncEntry.entry = entry;

        if (!asyncBuffer_.Push(std::move(asyncEntry))) {
            // Buffer full - drop message and increment counter
            stats_.messagesDropped.fetch_add(1, std::memory_order_relaxed);
        }
        else {
            // Notify worker thread
            workerCondition_.notify_one();
        }
    }

    void LogSync(const LogEntry& entry) {
        std::shared_lock<std::shared_mutex> lock(sinksMutex_);
        for (auto& sink : sinks_) {
            if (entry.level >= sink->GetLevel()) {
                sink->Write(entry);
                stats_.bytesWritten.fetch_add(entry.message.length(), std::memory_order_relaxed);
            }
        }
    }

    void AsyncWorker() {
        AsyncLogEntry entry;
        std::vector<AsyncLogEntry> batch;
        batch.reserve(64); // Process in batches for efficiency

        while (workerRunning_.load(std::memory_order_acquire)) {
            // Wait for messages or shutdown signal
            std::unique_lock<std::mutex> lock(workerMutex_);
            workerCondition_.wait(lock, [this] {
                return !asyncBuffer_.IsEmpty() || !workerRunning_.load(std::memory_order_acquire);
                });
            lock.unlock();

            // Process all available messages
            batch.clear();
            while (asyncBuffer_.Pop(entry)) {
                batch.push_back(std::move(entry));

                // Process in reasonable batches
                if (batch.size() >= 64) {
                    ProcessBatch(batch);
                    batch.clear();
                }
            }

            // Process remaining messages
            if (!batch.empty()) {
                ProcessBatch(batch);
            }
        }
    }

    void ProcessBatch(const std::vector<AsyncLogEntry>& batch) {
        std::shared_lock<std::shared_mutex> lock(sinksMutex_);

        for (const auto& asyncEntry : batch) {
            for (auto& sink : sinks_) {
                if (asyncEntry.entry.level >= sink->GetLevel()) {
                    sink->Write(asyncEntry.entry);
                    stats_.bytesWritten.fetch_add(asyncEntry.GetMessage().length(), std::memory_order_relaxed);
                }
            }
        }
    }

    void ProcessRemainingMessages() {
        AsyncLogEntry entry;
        while (asyncBuffer_.Pop(entry)) {
            std::shared_lock<std::shared_mutex> lock(sinksMutex_);
            for (auto& sink : sinks_) {
                if (entry.entry.level >= sink->GetLevel()) {
                    sink->Write(entry.entry);
                }
            }
        }
    }

    // Thread safety
    mutable std::shared_mutex channelsMutex_;
    mutable std::shared_mutex sinksMutex_;
    std::mutex initMutex_;
    std::mutex workerMutex_;
    std::condition_variable workerCondition_;

    // Storage
    std::unordered_map<std::string, std::unique_ptr<LogChannel>> channels_;
    std::vector<std::unique_ptr<ILogSink>> sinks_;

    // Async logging
    LockFreeRingBuffer<AsyncLogEntry, 8192> asyncBuffer_; // 8K message buffer
    std::thread workerThread_;
    std::atomic<bool> workerRunning_;
    std::atomic<bool> initialized_{ false };

    // Configuration
    std::atomic<LogLevel> globalLevel_{ LogLevel::Debug };
    std::atomic<bool> asyncMode_{ true };

    // Statistics
    LogManager::Statistics stats_;
};

// =============================================================================
// LogManager Public Interface
// =============================================================================

LogManager& LogManager::Instance() noexcept {
    static LogManager instance;
    return instance;
}

static LogManagerImpl& GetImpl() {
    static LogManagerImpl impl;
    return impl;
}

void LogManager::Initialize() {
    GetImpl().Initialize();
}

void LogManager::Shutdown() {
    GetImpl().Shutdown();
}

LogChannel& LogManager::GetChannel(std::string_view name) {
    return GetImpl().GetChannel(name);
}

void LogManager::RemoveChannel(std::string_view name) {
    GetImpl().RemoveChannel(name);
}

void LogManager::AddSink(std::unique_ptr<ILogSink> sink) {
    GetImpl().AddSink(std::move(sink));
}

void LogManager::RemoveSink(ILogSink* sink) {
    GetImpl().RemoveSink(sink);
}

void LogManager::ClearSinks() {
    GetImpl().ClearSinks();
}

void LogManager::SetGlobalLevel(LogLevel level) {
    GetImpl().SetGlobalLevel(level);
}

LogLevel LogManager::GetGlobalLevel() const {
    return GetImpl().GetGlobalLevel();
}

void LogManager::Log(const LogEntry& entry) {
    GetImpl().Log(entry);
}

const LogManager::Statistics& LogManager::GetStatistics() const noexcept {
    return GetImpl().GetStatistics();
}

void LogManager::ResetStatistics() noexcept {
    GetImpl().ResetStatistics();
}

void LogManager::Flush() {
    GetImpl().Flush();
}

void LogManager::SetAsyncMode(bool enabled) {
    GetImpl().SetAsyncMode(enabled);
}

bool LogManager::IsAsyncMode() const {
    return GetImpl().IsAsyncMode();
}

// =============================================================================
// Template Instantiations
// =============================================================================

// Explicit template instantiations for common format patterns
template void LogChannel::LogFormat<int>(LogLevel, std::format_string<int>, int&&, const std::source_location&) const;
template void LogChannel::LogFormat<float>(LogLevel, std::format_string<float>, float&&, const std::source_location&) const;
template void LogChannel::LogFormat<const char*>(LogLevel, std::format_string<const char*>, const char*&&, const std::source_location&) const;
template void LogChannel::LogFormat<std::string>(LogLevel, std::format_string<std::string>, std::string&&, const std::source_location&) const;
template void LogChannel::LogFormat<int, float>(LogLevel, std::format_string<int, float>, int&&, float&&, const std::source_location&) const;