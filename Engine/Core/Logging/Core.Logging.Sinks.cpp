// Core.Logging.Sinks.cpp - Built-in Log Sinks
module;

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

module Akhanda.Core.Logging;

using namespace Akhanda::Logging;

// =============================================================================
// Console Sink - Colored Windows Console Output
// =============================================================================


class ConsoleSink : public ILogSink {
public:
    ConsoleSink() : minLevel_(LogLevel::Debug) {
        // Get console handles
        hStdOut_ = GetStdHandle(STD_OUTPUT_HANDLE);
        hStdErr_ = GetStdHandle(STD_ERROR_HANDLE);

        // Enable virtual terminal processing for color support
        DWORD mode;
        if (GetConsoleMode(hStdOut_, &mode)) {
            SetConsoleMode(hStdOut_, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
        if (GetConsoleMode(hStdErr_, &mode)) {
            SetConsoleMode(hStdErr_, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }

        // Store original console attributes
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(hStdOut_, &csbi)) {
            originalAttributes_ = csbi.wAttributes;
        }
    }

    ~ConsoleSink() override {
        // Restore original console attributes
        SetConsoleTextAttribute(hStdOut_, originalAttributes_);
        SetConsoleTextAttribute(hStdErr_, originalAttributes_);
    }

    void Write(const LogEntry& entry) override {
        if (entry.level < minLevel_.load(std::memory_order_relaxed)) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        // Choose output handle and color
        HANDLE handle = (entry.level >= LogLevel::Error) ? hStdErr_ : hStdOut_;
        WORD color = GetColorForLevel(entry.level);

        // Format timestamp
        auto time = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(time);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            time.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&timeT), "%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

        // Format thread ID
        std::ostringstream threadOss;
        threadOss << entry.threadId;
        std::string threadStr = threadOss.str();
        if (threadStr.length() > 8) {
            threadStr = threadStr.substr(threadStr.length() - 8);
        }

        // Set color
        SetConsoleTextAttribute(handle, color);

        // Write formatted log entry
        std::ostringstream logLine;
        logLine << "[" << oss.str() << "] "
            << "[" << std::setw(5) << ToString(entry.level) << "] "
            << "[" << std::setw(8) << threadStr << "] "
            << "[" << entry.channel << "] "
            << entry.message;

        // Add source location for debug/error levels
        if (entry.level == LogLevel::Debug || entry.level >= LogLevel::Error) {
            logLine << " (" << std::filesystem::path(entry.location.file_name()).filename().string()
                << ":" << entry.location.line() << ")";
        }

        logLine << "\n";

        // Write to console
        std::string logString = logLine.str();
        DWORD written;
        WriteConsoleA(handle, logString.c_str(), static_cast<DWORD>(logString.length()), &written, nullptr);

        // Reset color
        SetConsoleTextAttribute(handle, originalAttributes_);
    }

    void Flush() override {
        // Console output is typically unbuffered, but ensure it's flushed
        std::cout.flush();
        std::cerr.flush();
    }

    void SetLevel(LogLevel minLevel) override {
        minLevel_.store(minLevel, std::memory_order_relaxed);
    }

    LogLevel GetLevel() const override {
        return minLevel_.load(std::memory_order_relaxed);
    }

    bool IsThreadSafe() const override {
        return true;
    }

    bool SupportsAsync() const override {
        return true;
    }

    // Console-specific configuration
    void SetUseColors(bool useColors) {
        useColors_ = useColors;
    }

    void SetShowSourceLocation(bool showLocation) {
        showSourceLocation_ = showLocation;
    }

private:
    WORD GetColorForLevel(LogLevel level) const {
        if (!useColors_) {
            return originalAttributes_;
        }

        switch (level) {
        case LogLevel::Debug:
            return FOREGROUND_BLUE | FOREGROUND_GREEN; // Cyan
        case LogLevel::Info:
            return FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE; // White
        case LogLevel::Warning:
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // Yellow
        case LogLevel::Error:
            return FOREGROUND_RED | FOREGROUND_INTENSITY; // Bright Red
        case LogLevel::Fatal:
            return FOREGROUND_RED | BACKGROUND_RED | FOREGROUND_INTENSITY; // Red on Red
        default:
            return originalAttributes_;
        }
    }

    HANDLE hStdOut_;
    HANDLE hStdErr_;
    WORD originalAttributes_ = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    std::atomic<LogLevel> minLevel_;
    std::mutex mutex_;
    bool useColors_ = true;
    bool showSourceLocation_ = true;
};

// =============================================================================
// File Sink - High-Performance File Logging with Rotation
// =============================================================================

class FileSink : public ILogSink {
public:
    struct Config {
        std::string filename = "engine.log";
        std::string directory = "Logs";
        size_t maxFileSize = 50 * 1024 * 1024; // 50MB
        size_t maxFiles = 10;
        bool asyncWrite = true;
        size_t bufferSize = 64 * 1024; // 64KB buffer
        bool autoFlush = true;
        std::chrono::milliseconds flushInterval{ 1000 }; // 1 second
    };

    explicit FileSink(const Config& config = {})
        : config_(config)
        , minLevel_(LogLevel::Debug)
        , running_(false)
        , buffer_(config_.bufferSize)
        , bufferPos_(0) {

        // Create log directory
        std::filesystem::create_directories(config_.directory);

        // Open initial log file
        OpenLogFile();

        if (config_.asyncWrite) {
            StartAsyncWriter();
        }
    }

    ~FileSink() override {
        StopAsyncWriter();
        if (currentFile_.is_open()) {
            currentFile_.close();
        }
    }

    void Write(const LogEntry& entry) override {
        if (entry.level < minLevel_.load(std::memory_order_relaxed)) {
            return;
        }

        // Format log entry
        std::string formattedEntry = FormatLogEntry(entry);

        if (config_.asyncWrite) {
            WriteAsync(formattedEntry);
        }
        else {
            WriteSync(formattedEntry);
        }
    }

    void Flush() override {
        if (config_.asyncWrite) {
            // Signal flush and wait
            {
                std::lock_guard<std::mutex> lock(asyncMutex_);
                flushRequested_ = true;
            }
            asyncCondition_.notify_one();

            // Wait for flush completion
            std::unique_lock<std::mutex> lock(flushMutex_);
            flushComplete_.wait(lock, [this] { return !flushRequested_; });
        }
        else {
            std::lock_guard<std::mutex> lock(fileMutex_);
            FlushBuffer();
            currentFile_.flush();
        }
    }

    void SetLevel(LogLevel minLevel) override {
        minLevel_.store(minLevel, std::memory_order_relaxed);
    }

    LogLevel GetLevel() const override {
        return minLevel_.load(std::memory_order_relaxed);
    }

    bool IsThreadSafe() const override {
        return true;
    }

    bool SupportsAsync() const override {
        return config_.asyncWrite;
    }

    // File-specific methods
    void RotateNow() {
        std::lock_guard<std::mutex> lock(fileMutex_);
        RotateLogFile();
    }

    size_t GetCurrentFileSize() const {
        std::lock_guard<std::mutex> lock(fileMutex_);
        return currentFileSize_;
    }

    std::string GetCurrentFilePath() const {
        std::lock_guard<std::mutex> lock(fileMutex_);
        return currentFilePath_;
    }

private:
    void OpenLogFile() {
        // Generate filename with timestamp
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << config_.directory << "/"
            << std::put_time(std::localtime(&timeT), "%Y%m%d_%H%M%S_")
            << config_.filename;

        currentFilePath_ = oss.str();
        currentFile_.open(currentFilePath_, std::ios::app);
        currentFileSize_ = 0;

        if (currentFile_.is_open()) {
            // Get current file size
            currentFile_.seekp(0, std::ios::end);
            currentFileSize_ = static_cast<size_t>(currentFile_.tellp());

            // Write header
            currentFile_ << "=== Log session started at "
                << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S")
                << " ===\n";
            currentFile_.flush();
        }
    }

    void RotateLogFile() {
        if (currentFile_.is_open()) {
            currentFile_ << "=== Log session ended ===\n";
            currentFile_.close();
        }

        // Clean up old files
        CleanupOldFiles();

        // Open new file
        OpenLogFile();
    }

    void CleanupOldFiles() {
        try {
            std::vector<std::filesystem::directory_entry> logFiles;

            for (const auto& entry : std::filesystem::directory_iterator(config_.directory)) {
                if (entry.is_regular_file() &&
                    entry.path().filename().string().find(config_.filename) != std::string::npos) {
                    logFiles.push_back(entry);
                }
            }

            // Sort by modification time (newest first)
            std::sort(logFiles.begin(), logFiles.end(),
                [](const auto& a, const auto& b) {
                    return std::filesystem::last_write_time(a) >
                        std::filesystem::last_write_time(b);
                });

            // Remove excess files
            for (size_t i = config_.maxFiles; i < logFiles.size(); ++i) {
                std::filesystem::remove(logFiles[i].path());
            }
        }
        catch (const std::exception&) {
            // Ignore cleanup errors
        }
    }

    std::string FormatLogEntry(const LogEntry& entry) {
        auto time = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(time);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            time.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

        std::ostringstream threadOss;
        threadOss << entry.threadId;

        std::ostringstream result;
        result << "[" << oss.str() << "] "
            << "[" << std::setw(5) << ToString(entry.level) << "] "
            << "[" << threadOss.str() << "] "
            << "[" << entry.channel << "] "
            << entry.message
            << " (" << std::filesystem::path(entry.location.file_name()).filename().string()
            << ":" << entry.location.line()
            << " in " << entry.location.function_name() << ")"
            << "\n";

        return result.str();
    }

    void WriteSync(const std::string& entry) {
        std::lock_guard<std::mutex> lock(fileMutex_);

        if (!currentFile_.is_open()) {
            return;
        }

        // Add to buffer
        if (bufferPos_ + entry.length() > buffer_.size()) {
            FlushBuffer();
        }

        std::memcpy(buffer_.data() + bufferPos_, entry.c_str(), entry.length());
        bufferPos_ += entry.length();
        currentFileSize_ += entry.length();

        // Auto-flush if enabled
        if (config_.autoFlush) {
            FlushBuffer();
        }

        // Check for rotation
        if (currentFileSize_ >= config_.maxFileSize) {
            FlushBuffer();
            RotateLogFile();
        }
    }

    void WriteAsync(const std::string& entry) {
        std::lock_guard<std::mutex> lock(asyncMutex_);
        asyncQueue_.push(entry);
        asyncCondition_.notify_one();
    }

    void FlushBuffer() {
        if (bufferPos_ > 0 && currentFile_.is_open()) {
            currentFile_.write(buffer_.data(), bufferPos_);
            currentFile_.flush();
            bufferPos_ = 0;
        }
    }

    void StartAsyncWriter() {
        running_ = true;
        asyncThread_ = std::thread(&FileSink::AsyncWriterLoop, this);
    }

    void StopAsyncWriter() {
        if (running_) {
            {
                std::lock_guard<std::mutex> lock(asyncMutex_);
                running_ = false;
            }
            asyncCondition_.notify_all();

            if (asyncThread_.joinable()) {
                asyncThread_.join();
            }
        }
    }

    void AsyncWriterLoop() {
        auto lastFlush = std::chrono::steady_clock::now();

        while (running_) {
            std::unique_lock<std::mutex> lock(asyncMutex_);

            // Wait for data or timeout
            asyncCondition_.wait_for(lock, config_.flushInterval, [this] {
                return !asyncQueue_.empty() || !running_ || flushRequested_;
                });

            // Process all queued entries
            std::queue<std::string> localQueue;
            asyncQueue_.swap(localQueue);

            bool shouldFlush = flushRequested_;
            flushRequested_ = false;

            lock.unlock();

            // Write entries
            while (!localQueue.empty()) {
                WriteSync(localQueue.front());
                localQueue.pop();
            }

            // Periodic flush
            auto now = std::chrono::steady_clock::now();
            if (shouldFlush || (now - lastFlush) >= config_.flushInterval) {
                std::lock_guard<std::mutex> fileLock(fileMutex_);
                FlushBuffer();
                lastFlush = now;

                if (shouldFlush) {
                    std::lock_guard<std::mutex> flushLock(flushMutex_);
                    flushComplete_.notify_all();
                }
            }
        }
    }

    Config config_;
    std::atomic<LogLevel> minLevel_;

    // File management
    mutable std::mutex fileMutex_;
    std::ofstream currentFile_;
    std::string currentFilePath_;
    size_t currentFileSize_ = 0;

    // Buffering
    std::vector<char> buffer_;
    size_t bufferPos_ = 0;

    // Async writing
    std::atomic<bool> running_;
    std::thread asyncThread_;
    std::mutex asyncMutex_;
    std::condition_variable asyncCondition_;
    std::queue<std::string> asyncQueue_;

    // Flush synchronization
    std::mutex flushMutex_;
    std::condition_variable flushComplete_;
    std::atomic<bool> flushRequested_{ false };
};


// =============================================================================
// Editor Sink - Real-time Display for Integrated Editor
// =============================================================================

class EditorSink : public ILogSink {
public:
    struct EditorLogEntry {
        LogEntry entry;
        std::string formattedMessage;
        std::chrono::high_resolution_clock::time_point timestamp;
        uint64_t sequenceId;
    };

    explicit EditorSink(size_t maxEntries = 10000)
        : maxEntries_(maxEntries)
        , minLevel_(LogLevel::Debug)
        , sequenceCounter_(0) {
    }

    void Write(const LogEntry& entry) override {
        if (entry.level < minLevel_.load(std::memory_order_relaxed)) {
            return;
        }

        EditorLogEntry editorEntry;
        editorEntry.entry = entry;
        editorEntry.formattedMessage = FormatForEditor(entry);
        editorEntry.timestamp = std::chrono::high_resolution_clock::now();
        editorEntry.sequenceId = sequenceCounter_.fetch_add(1, std::memory_order_relaxed);

        std::lock_guard<std::mutex> lock(mutex_);

        // Add to circular buffer
        logEntries_.push_back(std::move(editorEntry));

        // Remove oldest entries if we exceed max
        if (logEntries_.size() > maxEntries_) {
            logEntries_.erase(logEntries_.begin(),
                logEntries_.begin() + (logEntries_.size() - maxEntries_));
        }

        // Notify listeners
        hasNewEntries_ = true;
    }

    void Flush() override {
        // Editor sink doesn't need traditional flushing
        std::lock_guard<std::mutex> lock(mutex_);
        hasNewEntries_ = true;
    }

    void SetLevel(LogLevel minLevel) override {
        minLevel_.store(minLevel, std::memory_order_relaxed);
    }

    LogLevel GetLevel() const override {
        return minLevel_.load(std::memory_order_relaxed);
    }

    bool IsThreadSafe() const override {
        return true;
    }

    bool SupportsAsync() const override {
        return true;
    }

    // Editor-specific methods
    std::vector<EditorLogEntry> GetRecentEntries(size_t count = 100) const {
        std::lock_guard<std::mutex> lock(mutex_);

        if (logEntries_.size() <= count) {
            return logEntries_;
        }

        return std::vector<EditorLogEntry>(
            logEntries_.end() - count,
            logEntries_.end()
        );
    }

    std::vector<EditorLogEntry> GetEntriesAfter(uint64_t sequenceId) const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<EditorLogEntry> result;
        for (const auto& entry : logEntries_) {
            if (entry.sequenceId > sequenceId) {
                result.push_back(entry);
            }
        }

        return result;
    }

    std::vector<EditorLogEntry> GetFilteredEntries(LogLevel minLevel,
        const std::string& channelFilter = "") const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<EditorLogEntry> result;
        for (const auto& entry : logEntries_) {
            if (entry.entry.level >= minLevel) {
                if (channelFilter.empty() ||
                    entry.entry.channel.find(channelFilter) != std::string::npos) {
                    result.push_back(entry);
                }
            }
        }

        return result;
    }

    void ClearEntries() {
        std::lock_guard<std::mutex> lock(mutex_);
        logEntries_.clear();
        hasNewEntries_ = false;
    }

    bool HasNewEntries() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return hasNewEntries_;
    }

    void MarkEntriesRead() {
        std::lock_guard<std::mutex> lock(mutex_);
        hasNewEntries_ = false;
    }

    size_t GetEntryCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return logEntries_.size();
    }

    uint64_t GetLatestSequenceId() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return logEntries_.empty() ? 0 : logEntries_.back().sequenceId;
    }

    // Statistics for editor display
    struct Statistics {
        size_t totalEntries = 0;
        size_t debugCount = 0;
        size_t infoCount = 0;
        size_t warningCount = 0;
        size_t errorCount = 0;
        size_t fatalCount = 0;
    };

    Statistics GetStatistics() const {
        std::lock_guard<std::mutex> lock(mutex_);

        Statistics stats;
        stats.totalEntries = logEntries_.size();

        for (const auto& entry : logEntries_) {
            switch (entry.entry.level) {
            case LogLevel::Debug:   stats.debugCount++; break;
            case LogLevel::Info:    stats.infoCount++; break;
            case LogLevel::Warning: stats.warningCount++; break;
            case LogLevel::Error:   stats.errorCount++; break;
            case LogLevel::Fatal:   stats.fatalCount++; break;
            }
        }

        return stats;
    }

private:
    std::string FormatForEditor(const LogEntry& entry) {
        auto time = entry.timestamp;
        auto timeT = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()); // Approximate conversion
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            time.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&timeT), "%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        oss << " [" << ToString(entry.level) << "] ";
        oss << "[" << entry.channel << "] ";
        oss << entry.message;

        return oss.str();
    }

    size_t maxEntries_;
    std::atomic<LogLevel> minLevel_;
    std::atomic<uint64_t> sequenceCounter_;

    mutable std::mutex mutex_;
    std::vector<EditorLogEntry> logEntries_;
    bool hasNewEntries_ = false;
};


// =============================================================================
// Sink Factory Functions
// =============================================================================
namespace Sinks {
    inline std::unique_ptr<ConsoleSink> CreateConsoleSink() {
        return std::make_unique<ConsoleSink>();
    }

    inline std::unique_ptr<FileSink> CreateFileSink(const FileSink::Config& config = {}) {
        return std::make_unique<FileSink>(config);
    }

    inline std::unique_ptr<EditorSink> CreateEditorSink(size_t maxEntries = 10000) {
        return std::make_unique<EditorSink>(maxEntries);
    }

    // Convenience function to set up default logging
    inline void SetupDefaultLogging() {
        auto& logManager = LogManager::Instance();

        // Console sink for immediate feedback
        auto consoleSink = CreateConsoleSink();
        consoleSink->SetLevel(LogLevel::Info);
        logManager.AddSink(std::move(consoleSink));

        // File sink for persistent logging
        FileSink::Config fileConfig;
        fileConfig.filename = "Akhanda.log";
        fileConfig.directory = "Logs";
        fileConfig.maxFileSize = 10 * 1024 * 1024; // 10MB
        fileConfig.maxFiles = 5;

        auto fileSink = CreateFileSink(fileConfig);
        fileSink->SetLevel(LogLevel::Debug);
        logManager.AddSink(std::move(fileSink));

        // Editor sink for real-time display
        auto editorSink = CreateEditorSink(5000); // Keep last 5000 entries
        editorSink->SetLevel(LogLevel::Debug);
        logManager.AddSink(std::move(editorSink));
    }
}
