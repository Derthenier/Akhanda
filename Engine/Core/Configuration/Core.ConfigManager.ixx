// Core.ConfigManager.ixx
// Akhanda Game Engine - Configuration Manager Module Interface
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <nlohmann/json.hpp>

export module Akhanda.Core.Configuration.Manager;

import Akhanda.Core.Configuration;
import Akhanda.Core.Configuration.FileHandler;
import Akhanda.Core.Threading;
import std;

export namespace Akhanda::Configuration {

    // ============================================================================
    // Configuration Manager - Central coordinator for all configuration
    // ============================================================================

    class ConfigManager {
    public:
        struct InitializationOptions {
            std::filesystem::path configDirectory = "Config";
            std::string mainConfigFile = "engine.json";
            bool enableHotReload = true;
            bool enableAutoSave = true;
            bool createMissingFiles = true;
            bool enableBackups = true;
            std::chrono::seconds autoSaveInterval{ 30 };
        };

        // ========================================================================
        // Singleton Access
        // ========================================================================

        static ConfigManager& Instance() {
            static ConfigManager instance;
            return instance;
        }

        ~ConfigManager() {
            Shutdown();
        }

        // Non-copyable, non-movable
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;
        ConfigManager(ConfigManager&&) = delete;
        ConfigManager& operator=(ConfigManager&&) = delete;

        // ========================================================================
        // Lifecycle Management
        // ========================================================================

        ConfigResult_t<bool> Initialize(const InitializationOptions& options = {}) {
            Threading::SpinLockGuard lock(initializationLock_);

            if (isInitialized_) {
                return ConfigError(ConfigResult::ReadOnlyError, "ConfigManager already initialized");
            }

            options_ = options;

            // Ensure config directory exists
            auto dirResult = ConfigFileHandler::EnsureConfigDirectory();
            if (!dirResult) {
                return dirResult.Error();
            }

            // Initialize file handler
            fileHandler_ = std::make_unique<ConfigFileHandler>();

            // Load main configuration file
            auto configPath = ConfigFileHandler::ResolveConfigPath(options_.mainConfigFile);
            auto loadResult = LoadMainConfiguration(configPath);
            if (!loadResult) {
                return loadResult.Error();
            }

            // Start auto-save timer if enabled
            if (options_.enableAutoSave) {
                StartAutoSaveTimer();
            }

            // Start hot-reload if enabled
            if (options_.enableHotReload) {
                StartHotReload(configPath);
            }

            isInitialized_ = true;
            return true;
        }

        void Shutdown() {
            Threading::SpinLockGuard lock(initializationLock_);

            if (!isInitialized_) {
                return;
            }

            // Stop auto-save timer
            StopAutoSaveTimer();

            // Stop hot-reload
            if (fileHandler_) {
                fileHandler_->StopFileWatching();
            }

            // Save any pending changes
            SaveAllSections();

            // Clear all sections
            {
                Threading::SpinLockGuard sectionLock(sectionsLock_);
                configSections_.clear();
            }

            // Reset state
            fileHandler_.reset();
            isInitialized_ = false;
        }

        bool IsInitialized() const noexcept {
            Threading::SpinLockGuard lock(initializationLock_);
            return isInitialized_;
        }

        // ========================================================================
        // Section Management
        // ========================================================================

        template<ConfigSection T>
        ConfigResult_t<std::shared_ptr<T>> RegisterSection() {
            if (!isInitialized_) {
                return ConfigError(ConfigResult::ReadOnlyError, "ConfigManager not initialized");
            }

            const std::string sectionName = T::SECTION_NAME;

            Threading::SpinLockGuard lock(sectionsLock_);

            // Check if section already exists
            if (configSections_.contains(sectionName)) {
                return ConfigError(ConfigResult::ReadOnlyError,
                    std::format("Section '{}' already registered", sectionName));
            }

            // Create new section instance
            auto section = std::make_shared<T>();

            // Load section data from main config if available
            if (mainConfig_.contains(sectionName)) {
                auto loadResult = section->LoadFromJson(mainConfig_[sectionName]);
                if (!loadResult) {
                    return loadResult.Error();
                }
            }

            // Register change notification
            section->OnSectionChange([this](const IConfigSection& changedSection) {
                OnSectionChanged(changedSection);
                });

            // Store section
            configSections_[sectionName] = section;

            return section;
        }

        template<ConfigSection T>
        std::shared_ptr<T> GetSection() {
            const std::string sectionName = T::SECTION_NAME;

            Threading::SpinLockGuard lock(sectionsLock_);
            auto it = configSections_.find(sectionName);

            if (it != configSections_.end()) {
                return std::dynamic_pointer_cast<T>(it->second);
            }

            return nullptr;
        }

        template<ConfigSection T>
        std::shared_ptr<T> GetOrCreateSection() {
            auto existing = GetSection<T>();
            if (existing) {
                return existing;
            }

            auto result = RegisterSection<T>();
            return result ? result.Value() : nullptr;
        }

        ConfigResult_t<bool> UnregisterSection(const std::string& sectionName) {
            Threading::SpinLockGuard lock(sectionsLock_);

            auto it = configSections_.find(sectionName);
            if (it == configSections_.end()) {
                return ConfigError(ConfigResult::KeyNotFound,
                    std::format("Section '{}' not found", sectionName));
            }

            // Save section before removing
            auto saveResult = SaveSection(*it->second);
            if (!saveResult) {
                // Log warning but continue with removal
                // LOG_WARNING("Failed to save section before removal: {}", saveResult.Error().message);
            }

            configSections_.erase(it);
            return true;
        }

        std::vector<std::string> GetRegisteredSections() const {
            Threading::SpinLockGuard lock(sectionsLock_);

            std::vector<std::string> sections;
            sections.reserve(configSections_.size());

            for (const auto& [name, _] : configSections_) {
                sections.emplace_back(name);
            }

            return sections;
        }

        // ========================================================================
        // Configuration Operations
        // ========================================================================

        ConfigResult_t<bool> LoadAllSections() {
            if (!isInitialized_) {
                return ConfigError(ConfigResult::ReadOnlyError, "ConfigManager not initialized");
            }

            auto configPath = ConfigFileHandler::ResolveConfigPath(options_.mainConfigFile);
            auto loadResult = LoadMainConfiguration(configPath);
            if (!loadResult) {
                return loadResult.Error();
            }

            Threading::SpinLockGuard lock(sectionsLock_);

            // Reload all registered sections
            for (auto& [sectionName, section] : configSections_) {
                if (mainConfig_.contains(sectionName)) {
                    auto sectionLoadResult = section->LoadFromJson(mainConfig_[sectionName]);
                    if (!sectionLoadResult) {
                        return ConfigError(ConfigResult::ParseError,
                            std::format("Failed to reload section '{}': {}",
                                sectionName, sectionLoadResult.Error().message));
                    }
                }
            }

            return true;
        }

        ConfigResult_t<bool> SaveAllSections() {
            if (!isInitialized_) {
                return ConfigError(ConfigResult::ReadOnlyError, "ConfigManager not initialized");
            }

            json updatedConfig = mainConfig_;

            {
                Threading::SpinLockGuard lock(sectionsLock_);

                // Update config with current section data
                for (const auto& [sectionName, section] : configSections_) {
                    updatedConfig[sectionName] = section->SaveToJson();
                }
            }

            // Save to file
            auto configPath = ConfigFileHandler::ResolveConfigPath(options_.mainConfigFile);

            ConfigFileHandler::SaveOptions saveOptions;
            saveOptions.createBackup = options_.enableBackups;
            saveOptions.prettyPrint = true;
            saveOptions.atomicWrite = true;

            auto saveResult = fileHandler_->SaveJsonFile(configPath, updatedConfig, saveOptions);
            if (!saveResult) {
                return saveResult.Error();
            }

            // Update our cached config
            mainConfig_ = std::move(updatedConfig);
            lastSaveTime_ = std::chrono::steady_clock::now();

            return true;
        }

        ConfigResult_t<bool> ResetAllSections() {
            Threading::SpinLockGuard lock(sectionsLock_);

            for (auto& [_, section] : configSections_) {
                section->ResetToDefaults();
            }

            return SaveAllSections();
        }

        ConfigResult_t<bool> ValidateAllSections() const {
            Threading::SpinLockGuard lock(sectionsLock_);

            std::vector<std::string> allErrors;

            for (const auto& [sectionName, section] : configSections_) {
                auto validationResult = section->Validate();
                if (!validationResult) {
                    allErrors.emplace_back(std::format("[{}] {}",
                        sectionName, validationResult.Error().message));
                }

                auto sectionErrors = section->GetValidationErrors();
                for (const auto& error : sectionErrors) {
                    allErrors.emplace_back(std::format("[{}] {}", sectionName, error));
                }
            }

            if (!allErrors.empty()) {
                std::string combinedError = "Validation errors:\n";
                for (const auto& error : allErrors) {
                    combinedError += "  " + error + "\n";
                }
                return ConfigError(ConfigResult::ValidationError, combinedError);
            }

            return true;
        }

        // ========================================================================
        // Hot-Reload and Change Notification
        // ========================================================================

        using ConfigChangeCallback = std::function<void(const std::string& sectionName)>;

        void OnConfigChange(ConfigChangeCallback callback) {
            Threading::SpinLockGuard lock(callbacksLock_);
            changeCallbacks_.emplace_back(std::move(callback));
        }

        void OnGlobalConfigChange(std::function<void()> callback) {
            Threading::SpinLockGuard lock(callbacksLock_);
            globalChangeCallbacks_.emplace_back(std::move(callback));
        }

        // ========================================================================
        // Utility and Debug
        // ========================================================================

        json GetFullConfiguration() const {
            Threading::SpinLockGuard lock(sectionsLock_);

            json fullConfig = mainConfig_;

            // Ensure all sections are included with latest data
            for (const auto& [sectionName, section] : configSections_) {
                fullConfig[sectionName] = section->SaveToJson();
            }

            return fullConfig;
        }

        std::chrono::steady_clock::time_point GetLastSaveTime() const noexcept {
            return lastSaveTime_;
        }

        bool HasUnsavedChanges() const noexcept {
            return lastChangeTime_ > lastSaveTime_;
        }

        size_t GetSectionCount() const noexcept {
            Threading::SpinLockGuard lock(sectionsLock_);
            return configSections_.size();
        }

    private:
        // ========================================================================
        // Private Constructor
        // ========================================================================

        ConfigManager() = default;

        // ========================================================================
        // Private Implementation
        // ========================================================================

        ConfigResult_t<bool> LoadMainConfiguration(const std::filesystem::path& configPath) {
            ConfigFileHandler::LoadOptions loadOptions;
            loadOptions.createIfMissing = options_.createMissingFiles;
            loadOptions.createBackup = options_.enableBackups;
            loadOptions.enableWatching = false; // We handle watching separately

            auto loadResult = fileHandler_->LoadJsonFile(configPath, loadOptions);
            if (!loadResult) {
                return loadResult.Error();
            }

            mainConfig_ = loadResult.Value();
            return true;
        }

        ConfigResult_t<bool> SaveSection(const IConfigSection& section) {
            const std::string sectionName = section.GetSectionName();
            mainConfig_[sectionName] = section.SaveToJson();

            lastChangeTime_ = std::chrono::steady_clock::now();
            return true;
        }

        void OnSectionChanged(const IConfigSection& section) {
            // Update main config
            SaveSection(section);

            // Notify listeners
            const std::string sectionName = section.GetSectionName();
            NotifyConfigChange(sectionName);

            // Trigger auto-save if enough time has passed
            if (options_.enableAutoSave) {
                auto now = std::chrono::steady_clock::now();
                if (now - lastSaveTime_ >= options_.autoSaveInterval) {
                    SaveAllSections();
                }
            }
        }

        void NotifyConfigChange(const std::string& sectionName) {
            Threading::SpinLockGuard lock(callbacksLock_);

            for (auto& callback : changeCallbacks_) {
                callback(sectionName);
            }

            for (auto& callback : globalChangeCallbacks_) {
                callback();
            }
        }

        void StartHotReload(const std::filesystem::path& configPath) {
            auto reloadCallback = [this]([[maybe_unused]] const std::filesystem::path& path) {
                auto reloadResult = LoadAllSections();
                if (!reloadResult) {
                    // LOG_ERROR("Hot-reload failed: {}", reloadResult.Error().message);
                }
                else {
                    // LOG_INFO("Configuration hot-reloaded from: {}", path.string());
                    NotifyConfigChange("*"); // Notify all sections changed
                }
                };

            fileHandler_->StartFileWatching(configPath, reloadCallback);
        }

        void StartAutoSaveTimer() {
            shouldStopAutoSave_.store(false);
            autoSaveThread_ = std::jthread([this](std::stop_token token) {
                while (!token.stop_requested() && !shouldStopAutoSave_.load()) {
                    std::this_thread::sleep_for(options_.autoSaveInterval);

                    if (HasUnsavedChanges() && !shouldStopAutoSave_.load()) {
                        SaveAllSections();
                    }
                }
                });
        }

        void StopAutoSaveTimer() {
            shouldStopAutoSave_.store(true);
            if (autoSaveThread_.joinable()) {
                autoSaveThread_.request_stop();
                autoSaveThread_.join();
            }
        }

        // ========================================================================
        // Member Variables
        // ========================================================================

        // Initialization
        mutable Threading::SpinLock initializationLock_;
        bool isInitialized_ = false;
        InitializationOptions options_;

        // File handling
        std::unique_ptr<ConfigFileHandler> fileHandler_;
        json mainConfig_;

        // Section management
        mutable Threading::SpinLock sectionsLock_;
        std::unordered_map<std::string, std::shared_ptr<IConfigSection>> configSections_;

        // Change notification
        mutable Threading::SpinLock callbacksLock_;
        std::vector<ConfigChangeCallback> changeCallbacks_;
        std::vector<std::function<void()>> globalChangeCallbacks_;

        // Timing
        std::chrono::steady_clock::time_point lastSaveTime_{};
        std::chrono::steady_clock::time_point lastChangeTime_{};

        // Auto-save
        std::jthread autoSaveThread_;
        std::atomic<bool> shouldStopAutoSave_{ false };
    };

    // ============================================================================
    // Global Configuration Access Helpers
    // ============================================================================

    // Convenience functions for common operations
    template<ConfigSection T>
    std::shared_ptr<T> GetConfig() {
        return ConfigManager::Instance().GetOrCreateSection<T>();
    }

    template<ConfigSection T>
    ConfigResult_t<std::shared_ptr<T>> RegisterConfig() {
        return ConfigManager::Instance().RegisterSection<T>();
    }

    inline ConfigResult_t<bool> InitializeConfig(const ConfigManager::InitializationOptions& options = {}) {
        return ConfigManager::Instance().Initialize(options);
    }

    inline void ShutdownConfig() {
        ConfigManager::Instance().Shutdown();
    }

    inline ConfigResult_t<bool> SaveConfig() {
        return ConfigManager::Instance().SaveAllSections();
    }

    inline ConfigResult_t<bool> ReloadConfig() {
        return ConfigManager::Instance().LoadAllSections();
    }

    // Helper for easy section registration
    template<ConfigSection T>
    inline void RegisterConfigSection() {
        ConfigSectionRegistry::RegisterSection<T>();
    }

} // namespace Akhanda::Configuration