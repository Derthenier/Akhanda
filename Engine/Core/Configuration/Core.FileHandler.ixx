// Core.FileHandler.ixx
// Akhanda Game Engine - Configuration File Handler Module Interface
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

export module Akhanda.Core.Configuration.FileHandler;

import Akhanda.Core.Configuration;
import Akhanda.Core.Threading;
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

        // Non-copyable, movable
        ConfigFileWatcher(const ConfigFileWatcher&) = delete;
        ConfigFileWatcher& operator=(const ConfigFileWatcher&) = delete;
        ConfigFileWatcher(ConfigFileWatcher&&) = default;
        ConfigFileWatcher& operator=(ConfigFileWatcher&&) = default;

        ConfigResult_t<bool> StartWatching(const std::filesystem::path& filePath, ChangeCallback callback) {
            if (isWatching_) {
                return ConfigError(ConfigResult::ReadOnlyError, "Already watching a file");
            }

            if (!std::filesystem::exists(filePath)) {
                return ConfigError(ConfigResult::FileNotFound,
                    std::format("File not found: {}", filePath.string()));
            }

            watchPath_ = filePath;
            callback_ = std::move(callback);
            lastFileInfo_ = FileInfo(filePath);

            // Start watching thread
            shouldStop_.store(false);
            watchThread_ = std::jthread([this](std::stop_token token) { WatchLoop(token); });
            isWatching_ = true;

            return true;
        }

        void Stop() {
            if (isWatching_) {
                shouldStop_.store(true);
                if (watchThread_.joinable()) {
                    watchThread_.request_stop();
                    watchThread_.join();
                }
                isWatching_ = false;
            }
        }

        bool IsWatching() const noexcept { return isWatching_; }
        const std::filesystem::path& GetWatchPath() const noexcept { return watchPath_; }

    private:
        void WatchLoop(std::stop_token token) {
            while (!token.stop_requested() && !shouldStop_.load()) {
                FileInfo currentInfo(watchPath_);

                if (currentInfo.exists && currentInfo.HasChanged(lastFileInfo_)) {
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

        ConfigResult_t<json> LoadJsonFile(const std::filesystem::path& filePath,
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
                    return ConfigResult_t<json>(json::object());
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
            const json& jsonData,
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
                    jsonString = jsonData.dump(options.indentSize);
                }
                else {
                    jsonString = jsonData.dump();
                }
            }
            catch (const json::exception& e) {
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

            return fileWatcher_.StartWatching(filePath, watchCallback);
        }

        void StopFileWatching() {
            fileWatcher_.Stop();
        }

        bool IsWatchingFile() const noexcept {
            return fileWatcher_.IsWatching();
        }

        // ========================================================================
        // Path Utilities
        // ========================================================================

        static std::filesystem::path GetConfigDirectory() {
            // Default to Config/ directory relative to executable
            static std::filesystem::path configDir = []() {
                auto exePath = std::filesystem::current_path();
                return exePath / "Config";
                }();
            return configDir;
        }

        static std::filesystem::path ResolveConfigPath(const std::string& filename) {
            std::filesystem::path path(filename);

            // If it's already absolute, use as-is
            if (path.is_absolute()) {
                return path;
            }

            // Resolve relative to config directory
            return GetConfigDirectory() / path;
        }

        static ConfigResult_t<bool> EnsureConfigDirectory() {
            auto configDir = GetConfigDirectory();
            std::error_code ec;

            if (!std::filesystem::exists(configDir, ec)) {
                if (!std::filesystem::create_directories(configDir, ec)) {
                    return ConfigError(ConfigResult::FileNotFound,
                        std::format("Failed to create config directory: {}", ec.message()));
                }
            }

            return true;
        }

        // ========================================================================
        // Backup and Recovery
        // ========================================================================

        ConfigResult_t<bool> CreateBackup(const std::filesystem::path& filePath) {
            if (!std::filesystem::exists(filePath)) {
                return ConfigError(ConfigResult::FileNotFound, "Source file does not exist");
            }

            auto backupPath = GenerateBackupPath(filePath);
            std::error_code ec;

            std::filesystem::copy_file(filePath, backupPath,
                std::filesystem::copy_options::overwrite_existing, ec);

            if (ec) {
                return ConfigError(ConfigResult::MemoryError,
                    std::format("Failed to create backup: {}", ec.message()));
            }

            return true;
        }

        ConfigResult_t<bool> RestoreFromBackup(const std::filesystem::path& filePath) {
            auto backupPath = GenerateBackupPath(filePath);

            if (!std::filesystem::exists(backupPath)) {
                return ConfigError(ConfigResult::FileNotFound, "Backup file does not exist");
            }

            std::error_code ec;
            std::filesystem::copy_file(backupPath, filePath,
                std::filesystem::copy_options::overwrite_existing, ec);

            if (ec) {
                return ConfigError(ConfigResult::MemoryError,
                    std::format("Failed to restore from backup: {}", ec.message()));
            }

            return true;
        }

        std::vector<std::filesystem::path> GetAvailableBackups(const std::filesystem::path& filePath) {
            std::vector<std::filesystem::path> backups;
            auto backupDir = filePath.parent_path() / "Backups";

            if (!std::filesystem::exists(backupDir)) {
                return backups;
            }

            auto filename = filePath.filename().string();
            for (const auto& entry : std::filesystem::directory_iterator(backupDir)) {
                if (entry.path().filename().string().starts_with(filename)) {
                    backups.push_back(entry.path());
                }
            }

            // Sort by modification time (newest first)
            std::sort(backups.begin(), backups.end(), [](const auto& a, const auto& b) {
                return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
                });

            return backups;
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

        ConfigResult_t<json> ParseJsonString(const std::string& jsonString, const std::string& context) {
            try {
                return json::parse(jsonString);
            }
            catch (const json::parse_error& e) {
                return ConfigError(ConfigResult::ParseError,
                    std::format("JSON parse error in {}: {} at byte {}", context, e.what(), e.byte));
            }
            catch (const json::exception& e) {
                return ConfigError(ConfigResult::ParseError,
                    std::format("JSON error in {}: {}", context, e.what()));
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
            json emptyObject = json::object();
            return WriteFileContent(filePath, emptyObject.dump(2));
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