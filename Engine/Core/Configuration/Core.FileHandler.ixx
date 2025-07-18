// Core.FileHandler.ixx (Fixed)
// Akhanda Game Engine - Configuration File Handler Module Interface
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include "JsonWrapper.hpp"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

export module Akhanda.Core.Configuration.FileHandler;

import Akhanda.Core.Configuration;
import std;

export namespace Akhanda::Configuration {

    // ============================================================================
    // File Operation Results
    // ============================================================================

    struct FileInfo {
        std::filesystem::path filepath;
        std::filesystem::file_time_type lastModified;
        uintmax_t fileSize = 0;
        bool exists = false;
        bool isValid = false;

        FileInfo() = default;
        explicit FileInfo(const std::filesystem::path& path) : filepath(path) {
            Refresh();
        }

        void Refresh() {
            std::error_code ec;
            exists = std::filesystem::exists(filepath, ec);
            if (exists) {
                lastModified = std::filesystem::last_write_time(filepath, ec);
                fileSize = std::filesystem::file_size(filepath, ec);
                isValid = !ec;
            }
        }

        bool HasChanged(const FileInfo& other) const noexcept {
            return lastModified != other.lastModified || fileSize != other.fileSize;
        }
    };

    // ============================================================================
    // File Watcher for Hot-Reload Support
    // ============================================================================

    class ConfigFileWatcher {
    public:
        using ChangeCallback = std::function<void(const std::filesystem::path&)>;

        ConfigFileWatcher() = default;
        ~ConfigFileWatcher() { Stop(); }

        // Non-copyable, non-movable
        ConfigFileWatcher(const ConfigFileWatcher&) = delete;
        ConfigFileWatcher& operator=(const ConfigFileWatcher&) = delete;
        ConfigFileWatcher(ConfigFileWatcher&&) = delete;
        ConfigFileWatcher& operator=(ConfigFileWatcher&&) = delete;

        bool Start(const std::filesystem::path& watchPath, ChangeCallback callback) {
            if (isWatching_) {
                Stop();
            }

            watchPath_ = watchPath;
            callback_ = std::move(callback);

            if (!std::filesystem::exists(watchPath_)) {
                return false;
            }

            lastFileInfo_ = FileInfo(watchPath_);
            shouldStop_ = false;

            // Start watching thread
            watchThread_ = std::jthread([this](std::stop_token stop_token) {
                WatchLoop(stop_token);
                });

            isWatching_ = true;
            return true;
        }

        void Stop() {
            if (isWatching_) {
                shouldStop_ = true;
                if (watchThread_.joinable()) {
                    watchThread_.request_stop();
                    watchThread_.join();
                }
                isWatching_ = false;
            }
        }

        bool IsWatching() const noexcept { return isWatching_; }

    private:
        void WatchLoop(std::stop_token stop_token) {
            while (!stop_token.stop_requested() && !shouldStop_) {
                if (!std::filesystem::exists(watchPath_)) {
                    break;
                }

                FileInfo currentInfo(watchPath_);
                if (currentInfo.isValid && currentInfo.HasChanged(lastFileInfo_)) {
                    if (callback_) {
                        callback_(watchPath_);
                    }
                    lastFileInfo_ = currentInfo;
                }

                // Check every 500ms
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }

        std::filesystem::path watchPath_;
        ChangeCallback callback_;
        FileInfo lastFileInfo_;
        std::jthread watchThread_;
        std::atomic<bool> shouldStop_{ false };
        bool isWatching_ = false;
    };

    // ============================================================================
    // Configuration File Handler
    // ============================================================================

    class ConfigFileHandler {
    public:
        struct LoadOptions {
            bool createIfMissing = true;
            bool createBackup = true;
            bool validateJson = true;
            bool enableWatching = false;
            std::chrono::milliseconds parseTimeout{ 5000 };
        };

        struct SaveOptions {
            bool createBackup = true;
            bool prettyPrint = true;
            bool atomicWrite = true;
            int indentSize = 2;
        };

        ConfigFileHandler() = default;
        ~ConfigFileHandler() = default;

        // ========================================================================
        // File Loading Operations
        // ========================================================================

        ConfigResult_t<JsonValue> LoadJsonFile(const std::filesystem::path& filePath,
            const LoadOptions& options = {}) {

            // Validate path
            auto pathResult = ValidateFilePath(filePath);
            if (!pathResult) {
                return pathResult.Error();
            }

            // Check if file exists
            if (!std::filesystem::exists(filePath)) {
                if (options.createIfMissing) {
                    // Create empty JSON file
                    auto createResult = CreateEmptyJsonFile(filePath);
                    if (!createResult) {
                        return createResult.Error();
                    }
                    return ConfigJson::CreateObject();
                }
                return ConfigError(ConfigResult::FileNotFound,
                    std::format("Configuration file not found: {}", filePath.string()));
            }

            // Create backup if requested
            if (options.createBackup) {
                auto backupResult = CreateBackup(filePath);
                if (!backupResult) {
                    // Log warning but continue
                    // LOG_WARNING("Failed to create backup: {}", backupResult.Error().message);
                }
            }

            // Read file content
            auto contentResult = ReadFileContent(filePath);
            if (!contentResult) {
                return contentResult.Error();
            }

            // Parse JSON
            auto jsonResult = ParseJsonString(contentResult.Value(), filePath.string());
            if (!jsonResult) {
                return jsonResult.Error();
            }

            // Start file watching if requested
            if (options.enableWatching && !fileWatcher_.IsWatching()) {
                StartFileWatching(filePath);
            }

            lastLoadedFile_ = FileInfo(filePath);
            return jsonResult;
        }

        ConfigResult_t<bool> SaveJsonFile(const std::filesystem::path& filePath,
            const JsonValue& jsonData,
            const SaveOptions& options = {}) {

            // Validate path
            auto pathResult = ValidateFilePath(filePath);
            if (!pathResult) {
                return pathResult.Error();
            }

            // Create backup if file exists and backup is requested
            if (options.createBackup && std::filesystem::exists(filePath)) {
                auto backupResult = CreateBackup(filePath);
                if (!backupResult) {
                    return backupResult.Error();
                }
            }

            // Serialize JSON
            std::string jsonString;
            try {
                if (options.prettyPrint) {
                    jsonString = jsonData.ToString(options.indentSize);
                }
                else {
                    jsonString = jsonData.ToString(-1);
                }
            }
            catch (const std::exception& e) {
                return ConfigError(ConfigResult::ParseError,
                    std::format("JSON serialization error: {}", e.what()));
            }

            // Write to file
            if (options.atomicWrite) {
                return WriteFileAtomic(filePath, jsonString);
            }
            else {
                return WriteFileContent(filePath, jsonString);
            }
        }

        // ========================================================================
        // File Watching Operations
        // ========================================================================

        using FileChangeCallback = std::function<void(const std::filesystem::path&)>;

        ConfigResult_t<bool> StartFileWatching(const std::filesystem::path& filePath,
            FileChangeCallback callback = nullptr) {

            auto watchCallback = callback ? callback : [this](const std::filesystem::path& path) {
                OnFileChanged(path);
                };

            if (!fileWatcher_.Start(filePath, watchCallback)) {
                return ConfigError(ConfigResult::ValidationError,
                    std::format("Failed to start file watching: {}", filePath.string()));
            }

            return true;
        }

        void StopFileWatching() {
            fileWatcher_.Stop();
        }

        bool IsWatching() const { return fileWatcher_.IsWatching(); }

        // ========================================================================
        // Backup Management
        // ========================================================================

        ConfigResult_t<bool> CreateBackup(const std::filesystem::path& filePath) {
            if (!std::filesystem::exists(filePath)) {
                return ConfigError(ConfigResult::FileNotFound, "Cannot backup non-existent file");
            }

            auto backupPath = GenerateBackupPath(filePath);

            // Ensure backup directory exists
            auto backupDir = backupPath.parent_path();
            std::error_code ec;
            if (!std::filesystem::exists(backupDir, ec)) {
                if (!std::filesystem::create_directories(backupDir, ec)) {
                    return ConfigError(ConfigResult::MemoryError,
                        std::format("Failed to create backup directory: {}", ec.message()));
                }
            }

            // Copy file to backup location
            if (!std::filesystem::copy_file(filePath, backupPath, ec)) {
                return ConfigError(ConfigResult::MemoryError,
                    std::format("Failed to create backup: {}", ec.message()));
            }

            return true;
        }

        std::vector<std::filesystem::path> GetBackupFiles(const std::filesystem::path& filePath) {
            auto backupDir = filePath.parent_path() / "Backups";
            auto baseName = filePath.stem().string();

            std::vector<std::filesystem::path> backups;
            std::error_code ec;

            if (!std::filesystem::exists(backupDir, ec)) {
                return backups;
            }

            for (const auto& entry : std::filesystem::directory_iterator(backupDir, ec)) {
                if (entry.is_regular_file() && entry.path().stem().string().starts_with(baseName)) {
                    backups.push_back(entry.path());
                }
            }

            // Sort by modification time (newest first)
            std::sort(backups.begin(), backups.end(), [](const auto& a, const auto& b) {
                return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
                });

            return backups;
        }

        // ========================================================================
        // Static Utility Methods
        // ========================================================================

        static ConfigResult_t<bool> EnsureConfigDirectory() {
            auto configDir = std::filesystem::current_path() / "Config";
            std::error_code ec;

            if (!std::filesystem::exists(configDir, ec)) {
                if (!std::filesystem::create_directories(configDir, ec)) {
                    return ConfigError(ConfigResult::FileNotFound,
                        std::format("Failed to create config directory: {}", ec.message()));
                }
            }

            return true;
        }

        static std::filesystem::path ResolveConfigPath(const std::string& filename) {
            auto configDir = std::filesystem::current_path() / "Config";
            return configDir / filename;
        }

    private:
        // ========================================================================
        // Private Implementation
        // ========================================================================

        ConfigResult_t<bool> ValidateFilePath(const std::filesystem::path& filePath) {
            if (filePath.empty()) {
                return ConfigError(ConfigResult::ValidationError, "File path cannot be empty");
            }

            if (filePath.extension() != ".json") {
                return ConfigError(ConfigResult::ValidationError,
                    "Configuration files must have .json extension");
            }

            // Ensure parent directory exists
            auto parentDir = filePath.parent_path();
            if (!parentDir.empty()) {
                std::error_code ec;
                if (!std::filesystem::exists(parentDir, ec)) {
                    if (!std::filesystem::create_directories(parentDir, ec)) {
                        return ConfigError(ConfigResult::FileNotFound,
                            std::format("Failed to create directory: {}", ec.message()));
                    }
                }
            }

            return true;
        }

        ConfigResult_t<std::string> ReadFileContent(const std::filesystem::path& filePath) {
            std::ifstream file(filePath, std::ios::binary);
            if (!file.is_open()) {
                return ConfigError(ConfigResult::FileNotFound,
                    std::format("Failed to open file: {}", filePath.string()));
            }

            // Get file size
            file.seekg(0, std::ios::end);
            auto fileSize = file.tellg();
            file.seekg(0, std::ios::beg);

            if (fileSize <= 0) {
                return ConfigError(ConfigResult::ValidationError, "File is empty or invalid");
            }

            // Read content
            std::string content;
            content.resize(static_cast<size_t>(fileSize));
            file.read(content.data(), fileSize);

            if (!file) {
                return ConfigError(ConfigResult::MemoryError, "Failed to read file content");
            }

            return content;
        }

        ConfigResult_t<JsonValue> ParseJsonString(const std::string& jsonString, const std::string& context) {
            try {
                return JsonValue::Parse(jsonString);
            }
            catch (const std::exception& e) {
                return ConfigError(ConfigResult::ParseError,
                    std::format("JSON parse error in {}: {}", context, e.what()));
            }
        }

        ConfigResult_t<bool> WriteFileContent(const std::filesystem::path& filePath,
            const std::string& content) {
            std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
            if (!file.is_open()) {
                return ConfigError(ConfigResult::MemoryError,
                    std::format("Failed to open file for writing: {}", filePath.string()));
            }

            file << content;

            if (!file) {
                return ConfigError(ConfigResult::MemoryError, "Failed to write file content");
            }

            return true;
        }

        ConfigResult_t<bool> WriteFileAtomic(const std::filesystem::path& filePath,
            const std::string& content) {
            // Write to temporary file first
            auto tempPath = filePath;
            tempPath += ".tmp";

            auto writeResult = WriteFileContent(tempPath, content);
            if (!writeResult) {
                return writeResult;
            }

            // Atomically move temp file to final location
            std::error_code ec;
            std::filesystem::rename(tempPath, filePath, ec);

            if (ec) {
                std::filesystem::remove(tempPath, ec); // Cleanup temp file
                return ConfigError(ConfigResult::MemoryError,
                    std::format("Failed to move temp file: {}", ec.message()));
            }

            return true;
        }

        ConfigResult_t<bool> CreateEmptyJsonFile(const std::filesystem::path& filePath) {
            JsonValue emptyObject = ConfigJson::CreateObject();
            return WriteFileContent(filePath, emptyObject.ToString(2));
        }

        std::filesystem::path GenerateBackupPath(const std::filesystem::path& filePath) {
            auto backupDir = filePath.parent_path() / "Backups";
            auto timestamp = std::chrono::system_clock::now();
            auto timeT = std::chrono::system_clock::to_time_t(timestamp);

            std::ostringstream oss;
            oss << filePath.stem().string() << "_"
                << std::put_time(std::localtime(&timeT), "%Y%m%d_%H%M%S")
                << filePath.extension().string();

            return backupDir / oss.str();
        }

        void OnFileChanged(const std::filesystem::path& filePath) {
            // Default file change handler - can be overridden
            // For now, just update our cached file info
            lastLoadedFile_ = FileInfo(filePath);
        }

        // ========================================================================
        // Member Variables
        // ========================================================================

        ConfigFileWatcher fileWatcher_;
        FileInfo lastLoadedFile_;
    };

} // namespace Akhanda::Configuration