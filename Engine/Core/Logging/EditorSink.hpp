// EditorSink.hpp - Editor Integration for Logging System
#pragma once

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>

// Forward declare to avoid including full spdlog headers
namespace spdlog {
    namespace sinks {
        template<typename Mutex> class base_sink;
    }
    namespace details {
        struct log_msg;
    }
}

import Akhanda.Core.Logging;

// =============================================================================
// Editor Log Message Structure
// =============================================================================

namespace Akhanda::Logging {

    // Enhanced log message for editor display
    struct EditorLogMessage {
        LogLevel level;
        std::string channel;
        std::string message;
        std::string formattedTime;
        std::chrono::high_resolution_clock::time_point timestamp;
        std::thread::id threadId;

        // Editor-specific metadata
        uint32_t messageId;        // Unique ID for filtering/searching
        std::string category;      // Auto-categorized (Render, AI, Physics, etc.)
        bool isCollapsible;        // Can this message be collapsed with similar ones?
        uint32_t collapseCount;    // How many similar messages were collapsed

        // Source location (when available)
        std::string fileName;
        std::string functionName;
        uint32_t lineNumber;

        // Color/styling hints for editor
        uint32_t textColor;        // RGBA color for text
        uint32_t backgroundColor;  // RGBA color for background
        bool isBold;
        bool isItalic;

        EditorLogMessage()
            : level(LogLevel::Info)
            , timestamp(std::chrono::high_resolution_clock::now())
            , threadId(std::this_thread::get_id())
            , messageId(0)
            , isCollapsible(false)
            , collapseCount(0)
            , lineNumber(0)
            , textColor(0xFFFFFFFF)  // White
            , backgroundColor(0x00000000)  // Transparent
            , isBold(false)
            , isItalic(false) {
        }
    };

    // Editor log event types
    enum class EditorLogEvent {
        MessageAdded,
        MessageCollapsed,
        ChannelAdded,
        ChannelRemoved,
        LogCleared,
        FilterChanged
    };

    // Callback for editor events
    using EditorLogEventCallback = std::function<void(EditorLogEvent, const EditorLogMessage*)>;
}

// =============================================================================
// Advanced Editor Sink
// =============================================================================

namespace Akhanda::Logging {

    class AdvancedEditorSink {
    public:
        // Configuration options
        struct Config {
            size_t maxMessages = 10000;           // Maximum messages to keep in memory
            size_t maxCollapsedMessages = 100;    // Maximum collapsed messages per type
            bool enableCollapsing = true;         // Enable message collapsing
            bool enableCategorization = true;     // Auto-categorize messages
            bool enableColorCoding = true;        // Apply color coding based on level
            bool enableTimestamps = true;         // Include timestamps
            bool enableThreadInfo = true;         // Include thread information
            bool enableSourceLocation = true;     // Include source file/line info

            // Performance settings
            bool asyncProcessing = true;           // Process messages asynchronously
            size_t bufferSize = 1000;             // Buffer size for async processing
            std::chrono::milliseconds flushInterval{ 100 }; // How often to flush buffer
        };

        explicit AdvancedEditorSink(const Config& config = Config{});
        ~AdvancedEditorSink();

        // Configuration
        void SetConfig(const Config& config);
        const Config& GetConfig() const { return config_; }

        // Event handling
        void SetEventCallback(EditorLogEventCallback callback);
        void ClearEventCallback();

        // Message management
        void Clear();
        void SetMaxMessages(size_t maxMessages);
        size_t GetMessageCount() const;

        // Message retrieval
        std::vector<EditorLogMessage> GetMessages(size_t startIndex = 0, size_t count = SIZE_MAX) const;
        std::vector<EditorLogMessage> GetMessagesInRange(
            std::chrono::high_resolution_clock::time_point start,
            std::chrono::high_resolution_clock::time_point end) const;

        // Filtering
        void SetLevelFilter(LogLevel minLevel);
        void SetChannelFilter(const std::vector<std::string>& allowedChannels);
        void SetTextFilter(const std::string& searchText);
        void ClearFilters();

        // Statistics
        struct Statistics {
            std::atomic<uint64_t> totalMessages{ 0 };
            std::atomic<uint64_t> droppedMessages{ 0 };
            std::atomic<uint64_t> collapsedMessages{ 0 };
            std::atomic<uint64_t> filteredMessages{ 0 };

            struct LevelCounts {
                std::atomic<uint64_t> debug{ 0 };
                std::atomic<uint64_t> info{ 0 };
                std::atomic<uint64_t> warning{ 0 };
                std::atomic<uint64_t> error{ 0 };
                std::atomic<uint64_t> fatal{ 0 };
            } levelCounts;

            void Reset() {
                totalMessages = 0;
                droppedMessages = 0;
                collapsedMessages = 0;
                filteredMessages = 0;
                levelCounts.debug = 0;
                levelCounts.info = 0;
                levelCounts.warning = 0;
                levelCounts.error = 0;
                levelCounts.fatal = 0;
            }
        };

        const Statistics& GetStatistics() const { return stats_; }
        void ResetStatistics() { stats_.Reset(); }

        // Export functionality
        bool ExportToFile(const std::string& filename,
            const std::vector<EditorLogMessage>& messages = {}) const;
        bool ExportToClipboard(const std::vector<EditorLogMessage>& messages = {}) const;

        // Integration with LogManager
        void RegisterWithLogManager();
        void UnregisterFromLogManager();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;

        Config config_;
        EditorLogEventCallback eventCallback_;
        mutable std::shared_mutex callbackMutex_;

        mutable Statistics stats_;

        // Internal message processing
        void ProcessMessage(const LogEntry& entry);
        EditorLogMessage CreateEditorMessage(const LogEntry& entry);
        void ApplyFilters();
        void AutoCategorize(EditorLogMessage& message);
        void ApplyColorCoding(EditorLogMessage& message);
        bool ShouldCollapseMessage(const EditorLogMessage& message);
        void NotifyEvent(EditorLogEvent event, const EditorLogMessage* message = nullptr);
    };

    // =============================================================================
    // Utility Functions for Editor Integration
    // =============================================================================

    // Create formatted string for display in editor
    std::string FormatMessageForEditor(const EditorLogMessage& message, bool includeTimestamp = true);

    // Convert log level to color
    uint32_t GetLevelColor(LogLevel level);

    // Auto-detect message category based on content
    std::string DetectMessageCategory(const std::string& message, const std::string& channel);

    // HTML formatting for rich text editors
    std::string FormatMessageAsHTML(const EditorLogMessage& message);

    // JSON formatting for web-based editors
    std::string FormatMessageAsJSON(const EditorLogMessage& message);

    // =============================================================================
    // Editor Integration Helper Class
    // =============================================================================

    class EditorLogIntegration {
    public:
        static EditorLogIntegration& Instance();

        // Initialize with editor callbacks
        void Initialize(EditorLogEventCallback eventCallback);
        void Shutdown();

        // Quick access to editor sink
        AdvancedEditorSink& GetSink() { return *editorSink_; }
        const AdvancedEditorSink& GetSink() const { return *editorSink_; }

        // Convenience methods for editor UI
        void ClearLogs() { editorSink_->Clear(); }
        void SetLogLevel(LogLevel level) { editorSink_->SetLevelFilter(level); }
        void SearchLogs(const std::string& searchText) { editorSink_->SetTextFilter(searchText); }

        // Export functionality
        bool ExportLogsToFile(const std::string& filename) {
            return editorSink_->ExportToFile(filename);
        }

        // Real-time log count for editor status bar
        size_t GetTotalLogCount() const {
            return editorSink_->GetStatistics().totalMessages.load();
        }

        size_t GetErrorCount() const {
            return editorSink_->GetStatistics().levelCounts.error.load();
        }

        size_t GetWarningCount() const {
            return editorSink_->GetStatistics().levelCounts.warning.load();
        }

    private:
        EditorLogIntegration() = default;
        ~EditorLogIntegration();

        std::unique_ptr<AdvancedEditorSink> editorSink_;
        bool initialized_ = false;
    };

    // =============================================================================
    // Macros for Editor-Specific Logging
    // =============================================================================

    // Log directly to editor (bypasses console/file sinks)
#define LOG_EDITOR_DEBUG(message) \
        do { \
            EditorLogIntegration::Instance().GetSink().ProcessMessage( \
                LogEntry(LogLevel::Debug, "Editor", message)); \
        } while(0)

#define LOG_EDITOR_INFO(message) \
        do { \
            EditorLogIntegration::Instance().GetSink().ProcessMessage( \
                LogEntry(LogLevel::Info, "Editor", message)); \
        } while(0)

#define LOG_EDITOR_WARNING(message) \
        do { \
            EditorLogIntegration::Instance().GetSink().ProcessMessage( \
                LogEntry(LogLevel::Warning, "Editor", message)); \
        } while(0)

#define LOG_EDITOR_ERROR(message) \
        do { \
            EditorLogIntegration::Instance().GetSink().ProcessMessage( \
                LogEntry(LogLevel::Error, "Editor", message)); \
        } while(0)

    // Editor notifications for important events
#define LOG_EDITOR_NOTIFY(level, message) \
        do { \
            EditorLogIntegration::Instance().GetSink().ProcessMessage( \
                LogEntry(level, "Notification", message)); \
        } while(0)

    // Progress logging for long operations
#define LOG_EDITOR_PROGRESS(operation, percent) \
        LOG_EDITOR_INFO(std::format("{}: {}% complete", operation, percent))

} // namespace Akhanda::Logging