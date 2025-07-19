// EditorSink.cpp - Editor Integration Implementation
#include "Engine/Core/Logging/EditorSink.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <regex>
#include <iomanip>
#include <thread>
#include <deque>
#include <unordered_map>
#include <condition_variable>
#include <shared_mutex>
#include <cstring>

#ifdef _WIN32
#include <Windows.h>
#undef min
#undef max
#endif

import Akhanda.Core.Logging;

using namespace Akhanda::Logging;

// =============================================================================
// AdvancedEditorSink::Impl - Private Implementation
// =============================================================================

class AdvancedEditorSink::Impl {
public:
    explicit Impl(const AdvancedEditorSink::Config& config)
        : config_(config)
        , nextMessageId_(1)
        , running_(true) {

        messages_.reserve(config_.maxMessages);

        if (config_.asyncProcessing) {
            // Start async processing thread
            processingThread_ = std::thread(&Impl::ProcessingLoop, this);
        }
    }

    ~Impl() {
        Shutdown();
    }

    void Shutdown() {
        running_ = false;

        if (processingThread_.joinable()) {
            bufferCondition_.notify_all();
            processingThread_.join();
        }
    }

    void ProcessMessage(const LogEntry& entry) {
        if (!ShouldProcessMessage(entry)) {
            stats_.filteredMessages.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        auto editorMessage = CreateEditorMessage(entry);

        if (config_.asyncProcessing) {
            // Add to async buffer
            std::unique_lock<std::mutex> lock(bufferMutex_);

            if (messageBuffer_.size() >= config_.bufferSize) {
                // Buffer full - drop oldest message
                messageBuffer_.pop_front();
                stats_.droppedMessages.fetch_add(1, std::memory_order_relaxed);
            }

            messageBuffer_.push_back(std::move(editorMessage));
            bufferCondition_.notify_one();
        }
        else {
            // Process immediately
            AddMessageToStore(std::move(editorMessage));
        }
    }

    void SetConfig(const AdvancedEditorSink::Config& config) {
        std::lock_guard<std::shared_mutex> lock(configMutex_);
        config_ = config;
    }

    const AdvancedEditorSink::Config& GetConfig() const {
        std::shared_lock<std::shared_mutex> lock(configMutex_);
        return config_;
    }

    void Clear() {
        std::lock_guard<std::shared_mutex> lock(messagesMutex_);
        messages_.clear();
        collapsedMessages_.clear();
        NotifyEvent(EditorLogEvent::LogCleared);
    }

    void SetMaxMessages(size_t maxMessages) {
        std::lock_guard<std::shared_mutex> lock(configMutex_);
        config_.maxMessages = maxMessages;

        // Trim if necessary
        std::lock_guard<std::shared_mutex> msgLock(messagesMutex_);
        if (messages_.size() > maxMessages) {
            size_t toRemove = messages_.size() - maxMessages;
            messages_.erase(messages_.begin(), messages_.begin() + toRemove);
        }
    }

    size_t GetMessageCount() const {
        std::shared_lock<std::shared_mutex> lock(messagesMutex_);
        return messages_.size();
    }

    std::vector<EditorLogMessage> GetMessages(size_t startIndex, size_t count) const {
        std::shared_lock<std::shared_mutex> lock(messagesMutex_);

        if (startIndex >= messages_.size()) {
            return {};
        }

        size_t endIndex = std::min(startIndex + count, messages_.size());
        return std::vector<EditorLogMessage>(messages_.begin() + startIndex, messages_.begin() + endIndex);
    }

    std::vector<EditorLogMessage> GetMessagesInRange(
        std::chrono::high_resolution_clock::time_point start,
        std::chrono::high_resolution_clock::time_point end) const {

        std::shared_lock<std::shared_mutex> lock(messagesMutex_);
        std::vector<EditorLogMessage> result;

        for (const auto& msg : messages_) {
            if (msg.timestamp >= start && msg.timestamp <= end) {
                result.push_back(msg);
            }
        }

        return result;
    }

    void SetLevelFilter(LogLevel minLevel) {
        std::lock_guard<std::shared_mutex> lock(configMutex_);
        levelFilter_ = minLevel;
        NotifyEvent(EditorLogEvent::FilterChanged);
    }

    void SetChannelFilter(const std::vector<std::string>& allowedChannels) {
        std::lock_guard<std::shared_mutex> lock(configMutex_);
        channelFilter_ = allowedChannels;
        NotifyEvent(EditorLogEvent::FilterChanged);
    }

    void SetTextFilter(const std::string& searchText) {
        std::lock_guard<std::shared_mutex> lock(configMutex_);
        textFilter_ = searchText;
        NotifyEvent(EditorLogEvent::FilterChanged);
    }

    void ClearFilters() {
        std::lock_guard<std::shared_mutex> lock(configMutex_);
        levelFilter_ = LogLevel::Debug;
        channelFilter_.clear();
        textFilter_.clear();
        NotifyEvent(EditorLogEvent::FilterChanged);
    }

    bool ExportToFile(const std::string& filename, const std::vector<EditorLogMessage>& messages) const {
        try {
            std::ofstream file(filename);
            if (!file.is_open()) {
                return false;
            }

            const auto& exportMessages = messages.empty() ? GetAllMessages() : messages;

            for (const auto& msg : exportMessages) {
                file << FormatMessageForEditor(msg, true) << "\n";
            }

            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    }

    bool ExportToClipboard(const std::vector<EditorLogMessage>& messages) const {
#ifdef _WIN32
        try {
            const auto& exportMessages = messages.empty() ? GetAllMessages() : messages;

            std::ostringstream oss;
            for (const auto& msg : exportMessages) {
                oss << FormatMessageForEditor(msg, true) << "\r\n";
            }

            std::string text = oss.str();

            if (OpenClipboard(nullptr)) {
                EmptyClipboard();

                HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, text.size() + 1);
                if (hClipboardData) {
                    char* pchData = static_cast<char*>(GlobalLock(hClipboardData));
                    strcpy_s(pchData, text.size() + 1, text.c_str());
                    GlobalUnlock(hClipboardData);

                    SetClipboardData(CF_TEXT, hClipboardData);
                }

                CloseClipboard();
                return true;
            }
        }
        catch (const std::exception&) {
            return false;
        }
#endif
        return false;
    }

    void SetEventCallback(EditorLogEventCallback callback) {
        std::lock_guard<std::shared_mutex> lock(callbackMutex_);
        eventCallback_ = std::move(callback);
    }

    void ClearEventCallback() {
        std::lock_guard<std::shared_mutex> lock(callbackMutex_);
        eventCallback_ = nullptr;
    }

    const AdvancedEditorSink::Statistics& GetStatistics() const {
        return stats_;
    }

    void ResetStatistics() {
        stats_.Reset();
    }

private:
    bool ShouldProcessMessage(const LogEntry& entry) const {
        std::shared_lock<std::shared_mutex> lock(configMutex_);

        // Level filter
        if (entry.level < levelFilter_) {
            return false;
        }

        // Channel filter
        if (!channelFilter_.empty()) {
            bool channelAllowed = std::find(channelFilter_.begin(), channelFilter_.end(),
                entry.channel) != channelFilter_.end();
            if (!channelAllowed) {
                return false;
            }
        }

        // Text filter
        if (!textFilter_.empty()) {
            if (entry.message.find(textFilter_) == std::string::npos) {
                return false;
            }
        }

        return true;
    }

    EditorLogMessage CreateEditorMessage(const LogEntry& entry) {
        EditorLogMessage msg;
        msg.level = entry.level;
        msg.channel = std::string(entry.channel);
        msg.message = std::string(entry.message);
        msg.timestamp = entry.timestamp;
        msg.threadId = entry.threadId;
        msg.messageId = nextMessageId_.fetch_add(1, std::memory_order_relaxed);

        // Format timestamp - use current system time for display
        // (The exact timestamp conversion isn't critical for editor display)
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&timeT), "%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        msg.formattedTime = oss.str();

        // Extract source location if available
        msg.fileName = entry.location.file_name();
        msg.functionName = entry.location.function_name();
        msg.lineNumber = entry.location.line();

        // Auto-categorize and apply styling
        if (config_.enableCategorization) {
            AutoCategorize(msg);
        }

        if (config_.enableColorCoding) {
            ApplyColorCoding(msg);
        }

        return msg;
    }

    void AutoCategorize(EditorLogMessage& msg) {
        msg.category = DetectMessageCategory(msg.message, msg.channel);
    }

    void ApplyColorCoding(EditorLogMessage& msg) {
        msg.textColor = GetLevelColor(msg.level);

        // Apply additional styling based on level
        switch (msg.level) {
        case LogLevel::Error:
        case LogLevel::Fatal:
            msg.isBold = true;
            break;
        case LogLevel::Warning:
            msg.isItalic = true;
            break;
        default:
            break;
        }
    }

    void AddMessageToStore(EditorLogMessage&& msg) {
        bool shouldCollapse = config_.enableCollapsing && ShouldCollapseMessage(msg);

        std::lock_guard<std::shared_mutex> lock(messagesMutex_);

        if (shouldCollapse) {
            // Try to find similar message to collapse
            auto it = collapsedMessages_.find(GenerateCollapseKey(msg));
            if (it != collapsedMessages_.end()) {
                it->second.collapseCount++;
                stats_.collapsedMessages.fetch_add(1, std::memory_order_relaxed);
                NotifyEvent(EditorLogEvent::MessageCollapsed, &it->second);
                return;
            }
            else {
                msg.isCollapsible = true;
                msg.collapseCount = 1;
                collapsedMessages_[GenerateCollapseKey(msg)] = msg;
            }
        }

        // Add to message store
        if (messages_.size() >= config_.maxMessages) {
            messages_.erase(messages_.begin());
        }

        messages_.push_back(msg);
        stats_.totalMessages.fetch_add(1, std::memory_order_relaxed);

        NotifyEvent(EditorLogEvent::MessageAdded, &msg);
    }

    bool ShouldCollapseMessage(const EditorLogMessage& msg) {
        // Don't collapse errors or critical messages
        if (msg.level >= LogLevel::Error) {
            return false;
        }

        // Only collapse repetitive debug/info messages
        return msg.level == LogLevel::Debug || msg.level == LogLevel::Info;
    }

    std::string GenerateCollapseKey(const EditorLogMessage& msg) {
        return msg.channel + "|" + msg.message;
    }

    void ProcessingLoop() {
        std::vector<EditorLogMessage> batch;
        batch.reserve(100);

        while (running_) {
            std::unique_lock<std::mutex> lock(bufferMutex_);
            bufferCondition_.wait(lock, [this] { return !messageBuffer_.empty() || !running_; });

            if (!running_) break;

            // Move messages to local batch
            batch.clear();
            while (!messageBuffer_.empty() && batch.size() < 100) {
                batch.push_back(std::move(messageBuffer_.front()));
                messageBuffer_.pop_front();
            }
            lock.unlock();

            // Process batch
            for (auto& msg : batch) {
                AddMessageToStore(std::move(msg));
            }

            // Throttle processing if needed
            if (!batch.empty()) {
                std::this_thread::sleep_for(config_.flushInterval);
            }
        }
    }

    void NotifyEvent(EditorLogEvent event, const EditorLogMessage* message = nullptr) {
        std::shared_lock<std::shared_mutex> lock(callbackMutex_);
        if (eventCallback_) {
            eventCallback_(event, message);
        }
    }

    std::vector<EditorLogMessage> GetAllMessages() const {
        std::shared_lock<std::shared_mutex> lock(messagesMutex_);
        return messages_;
    }

    // Configuration and state
    mutable std::shared_mutex configMutex_;
    AdvancedEditorSink::Config config_;

    // Filtering
    LogLevel levelFilter_ = LogLevel::Debug;
    std::vector<std::string> channelFilter_;
    std::string textFilter_;

    // Message storage
    mutable std::shared_mutex messagesMutex_;
    std::vector<EditorLogMessage> messages_;
    std::unordered_map<std::string, EditorLogMessage> collapsedMessages_;
    std::atomic<uint32_t> nextMessageId_;

    // Async processing
    std::atomic<bool> running_;
    std::thread processingThread_;
    std::mutex bufferMutex_;
    std::condition_variable bufferCondition_;
    std::deque<EditorLogMessage> messageBuffer_;

    // Event callbacks
    mutable std::shared_mutex callbackMutex_;
    EditorLogEventCallback eventCallback_;

    // Statistics
    mutable AdvancedEditorSink::Statistics stats_;
};

// =============================================================================
// AdvancedEditorSink Implementation
// =============================================================================

AdvancedEditorSink::AdvancedEditorSink(const Config& config)
    : config_(config)
    , impl_(std::make_unique<Impl>(config)) {
}

AdvancedEditorSink::~AdvancedEditorSink() = default;

void AdvancedEditorSink::SetConfig(const Config& config) {
    config_ = config;
    impl_->SetConfig(config);
}

void AdvancedEditorSink::SetEventCallback(EditorLogEventCallback callback) {
    eventCallback_ = std::move(callback);
    impl_->SetEventCallback(eventCallback_);
}

void AdvancedEditorSink::ClearEventCallback() {
    eventCallback_ = nullptr;
    impl_->ClearEventCallback();
}

void AdvancedEditorSink::Clear() {
    impl_->Clear();
}

void AdvancedEditorSink::SetMaxMessages(size_t maxMessages) {
    impl_->SetMaxMessages(maxMessages);
}

size_t AdvancedEditorSink::GetMessageCount() const {
    return impl_->GetMessageCount();
}

std::vector<EditorLogMessage> AdvancedEditorSink::GetMessages(size_t startIndex, size_t count) const {
    return impl_->GetMessages(startIndex, count);
}

std::vector<EditorLogMessage> AdvancedEditorSink::GetMessagesInRange(
    std::chrono::high_resolution_clock::time_point start,
    std::chrono::high_resolution_clock::time_point end) const {
    return impl_->GetMessagesInRange(start, end);
}

void AdvancedEditorSink::SetLevelFilter(LogLevel minLevel) {
    impl_->SetLevelFilter(minLevel);
}

void AdvancedEditorSink::SetChannelFilter(const std::vector<std::string>& allowedChannels) {
    impl_->SetChannelFilter(allowedChannels);
}

void AdvancedEditorSink::SetTextFilter(const std::string& searchText) {
    impl_->SetTextFilter(searchText);
}

void AdvancedEditorSink::ClearFilters() {
    impl_->ClearFilters();
}

bool AdvancedEditorSink::ExportToFile(const std::string& filename, const std::vector<EditorLogMessage>& messages) const {
    return impl_->ExportToFile(filename, messages);
}

bool AdvancedEditorSink::ExportToClipboard(const std::vector<EditorLogMessage>& messages) const {
    return impl_->ExportToClipboard(messages);
}

void AdvancedEditorSink::RegisterWithLogManager() {
    // This would integrate with the LogManager's editor callback system
    auto& logManager = LogManager::Instance();
    logManager.SetEditorCallback([this](const LogEntry& entry) {
        impl_->ProcessMessage(entry);
        });
}

void AdvancedEditorSink::UnregisterFromLogManager() {
    auto& logManager = LogManager::Instance();
    logManager.ClearEditorCallback();
}

// For internal use by LogManager
void AdvancedEditorSink::ProcessMessage(const LogEntry& entry) {
    impl_->ProcessMessage(entry);
}

// =============================================================================
// EditorLogIntegration Implementation
// =============================================================================

EditorLogIntegration& EditorLogIntegration::Instance() {
    static EditorLogIntegration instance;
    return instance;
}

EditorLogIntegration::~EditorLogIntegration() {
    Shutdown();
}

void EditorLogIntegration::Initialize(EditorLogEventCallback eventCallback) {
    if (initialized_) return;

    AdvancedEditorSink::Config config;
    // Configure for optimal editor integration
    config.enableCollapsing = true;
    config.enableCategorization = true;
    config.enableColorCoding = true;
    config.asyncProcessing = true;
    config.maxMessages = 10000;

    editorSink_ = std::make_unique<AdvancedEditorSink>(config);
    editorSink_->SetEventCallback(std::move(eventCallback));
    editorSink_->RegisterWithLogManager();

    initialized_ = true;
}

void EditorLogIntegration::Shutdown() {
    if (!initialized_) return;

    if (editorSink_) {
        editorSink_->UnregisterFromLogManager();
        editorSink_.reset();
    }

    initialized_ = false;
}

// =============================================================================
// Utility Functions Implementation
// =============================================================================

std::string FormatMessageForEditor(const EditorLogMessage& message, bool includeTimestamp) {
    std::ostringstream oss;

    if (includeTimestamp) {
        oss << "[" << message.formattedTime << "] ";
    }

    oss << "[" << ToString(message.level) << "] ";

    if (!message.channel.empty()) {
        oss << "[" << message.channel << "] ";
    }

    oss << message.message;

    if (message.collapseCount > 1) {
        oss << " (x" << message.collapseCount << ")";
    }

    return oss.str();
}

uint32_t GetLevelColor(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:   return 0xFF808080;  // Gray
    case LogLevel::Info:    return 0xFFFFFFFF;  // White
    case LogLevel::Warning: return 0xFFFFFF00;  // Yellow
    case LogLevel::Error:   return 0xFFFF4444;  // Red
    case LogLevel::Fatal:   return 0xFFFF0000;  // Bright Red
    default:                return 0xFFFFFFFF;  // White
    }
}

std::string DetectMessageCategory(const std::string& message, const std::string& channel) {
    // Simple pattern matching for categorization
    std::string lowerMessage = message;
    std::transform(message.begin(), message.end(), lowerMessage.begin(), ::tolower);

    if (lowerMessage.find("render") != std::string::npos ||
        lowerMessage.find("draw") != std::string::npos ||
        lowerMessage.find("gpu") != std::string::npos) {
        return "Rendering";
    }

    if (lowerMessage.find("physics") != std::string::npos ||
        lowerMessage.find("collision") != std::string::npos) {
        return "Physics";
    }

    if (lowerMessage.find("audio") != std::string::npos ||
        lowerMessage.find("sound") != std::string::npos) {
        return "Audio";
    }

    if (lowerMessage.find("network") != std::string::npos ||
        lowerMessage.find("tcp") != std::string::npos ||
        lowerMessage.find("udp") != std::string::npos) {
        return "Networking";
    }

    if (lowerMessage.find("memory") != std::string::npos ||
        lowerMessage.find("alloc") != std::string::npos) {
        return "Memory";
    }

    if (lowerMessage.find("file") != std::string::npos ||
        lowerMessage.find("disk") != std::string::npos ||
        lowerMessage.find("load") != std::string::npos) {
        return "I/O";
    }

    // Use channel as fallback category
    if (!channel.empty()) {
        return channel;
    }

    return "General";
}

std::string FormatMessageAsHTML(const EditorLogMessage& message) {
    std::ostringstream oss;

    oss << "<div class=\"log-message log-" << ToString(message.level) << "\">";
    oss << "<span class=\"timestamp\">[" << message.formattedTime << "]</span> ";
    oss << "<span class=\"level\">[" << ToString(message.level) << "]</span> ";
    oss << "<span class=\"channel\">[" << message.channel << "]</span> ";
    oss << "<span class=\"message\">" << message.message << "</span>";

    if (message.collapseCount > 1) {
        oss << " <span class=\"collapse-count\">(x" << message.collapseCount << ")</span>";
    }

    oss << "</div>";

    return oss.str();
}

std::string FormatMessageAsJSON(const EditorLogMessage& message) {
    std::ostringstream oss;

    oss << "{"
        << "\"id\":" << message.messageId << ","
        << "\"level\":\"" << ToString(message.level) << "\","
        << "\"channel\":\"" << message.channel << "\","
        << "\"message\":\"" << message.message << "\","
        << "\"timestamp\":\"" << message.formattedTime << "\","
        << "\"category\":\"" << message.category << "\","
        << "\"collapseCount\":" << message.collapseCount << ","
        << "\"fileName\":\"" << message.fileName << "\","
        << "\"lineNumber\":" << message.lineNumber
        << "}";

    return oss.str();
}