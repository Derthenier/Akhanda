// Core.Configuration.Manager.ixx (Fixed with Traditional Header)
// Akhanda Game Engine - Simplified Configuration Manager
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include "JsonWrapper.hpp"
#include <unordered_map>

export module Akhanda.Core.Configuration.Manager;

import Akhanda.Core.Configuration;

export namespace Akhanda::Configuration {

    // ============================================================================
    // Simplified Configuration Manager
    // ============================================================================

    class ConfigManager {
    public:
        struct InitializationOptions {
            std::filesystem::path configDirectory = "Config";
            std::string mainConfigFile = "engine.json";
            bool createMissingFiles = true;
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
        // Core Operations
        // ========================================================================

        static bool Initialize(const InitializationOptions& options = {}) {
            return Instance().InitializeImpl(options);
        }

        static void Shutdown() {
            Instance().ShutdownImpl();
        }

        static bool IsInitialized() {
            return Instance().isInitialized_;
        }

        // ========================================================================
        // Section Management
        // ========================================================================

        template<typename T>
        static std::shared_ptr<T> GetOrCreateSection() {
            static_assert(is_config_section_v<T>, "T must be a configuration section");
            return Instance().GetOrCreateSectionImpl<T>();
        }

        template<typename T>
        static ConfigResult_t<std::shared_ptr<T>> RegisterSection() {
            static_assert(is_config_section_v<T>, "T must be a configuration section");
            return Instance().RegisterSectionImpl<T>();
        }

        static ConfigResult_t<bool> SaveAllSections() {
            return Instance().SaveAllSectionsImpl();
        }

        static ConfigResult_t<bool> LoadAllSections() {
            return Instance().LoadAllSectionsImpl();
        }

    private:
        ConfigManager() = default;

        bool InitializeImpl(const InitializationOptions& options) {
            if (isInitialized_) {
                return true;
            }

            options_ = options;

            // Create config directory if it doesn't exist
            if (options_.createMissingFiles) {
                try {
                    std::filesystem::create_directories(options_.configDirectory);
                }
                catch (...) {
                    // Continue even if directory creation fails
                }
            }

            // Load main configuration file
            auto configPath = options_.configDirectory / options_.mainConfigFile;
            if (std::filesystem::exists(configPath)) {
                try {
                    mainConfig_ = JsonValue::FromFile(configPath);
                }
                catch (...) {
                    // If loading fails, start with empty config
                    mainConfig_ = ConfigJson::CreateObject();
                }
            }
            else {
                mainConfig_ = ConfigJson::CreateObject();
            }

            isInitialized_ = true;
            return true;
        }

        void ShutdownImpl() {
            if (!isInitialized_) {
                return;
            }

            // Save all sections before shutdown
            SaveAllSectionsImpl();

            configSections_.clear();
            isInitialized_ = false;
        }

        template<typename T>
        std::shared_ptr<T> GetOrCreateSectionImpl() {
            static_assert(is_config_section_v<T>, "T must be a configuration section");
            const std::string sectionName = T{}.GetSectionName();

            // Check if section already exists
            auto it = configSections_.find(sectionName);
            if (it != configSections_.end()) {
                return std::static_pointer_cast<T>(it->second);
            }

            // Create new section
            auto section = std::make_shared<T>();
            configSections_[sectionName] = section;

            // Load from JSON if available
            if (mainConfig_.Contains(sectionName)) {
                auto sectionJson = mainConfig_[sectionName];
                section->LoadFromJson(sectionJson);
            }

            return section;
        }

        template<typename T>
        ConfigResult_t<std::shared_ptr<T>> RegisterSectionImpl() {
            static_assert(is_config_section_v<T>, "T must be a configuration section");
            if (!isInitialized_) {
                return ConfigError(ConfigResult::ValidationError, "ConfigManager not initialized");
            }

            const std::string sectionName = T{}.GetSectionName();

            // Check if section already registered
            if (configSections_.find(sectionName) != configSections_.end()) {
                return ConfigError(ConfigResult::ValidationError,
                    std::format("Section '{}' already registered", sectionName));
            }

            // Create and register section
            auto section = std::make_shared<T>();
            configSections_[sectionName] = section;

            // Load from JSON if available
            if (mainConfig_.Contains(sectionName)) {
                auto sectionJson = mainConfig_[sectionName];
                auto loadResult = section->LoadFromJson(sectionJson);
                if (!loadResult) {
                    return ConfigError(loadResult.Error().result,
                        std::format("Failed to load section '{}': {}", sectionName, loadResult.Error().message));
                }
            }

            return section;
        }

        ConfigResult_t<bool> SaveAllSectionsImpl() {
            if (!isInitialized_) {
                return ConfigError(ConfigResult::ValidationError, "ConfigManager not initialized");
            }

            try {
                // Update main config with all sections
                for (const auto& [sectionName, section] : configSections_) {
                    auto sectionJson = section->SaveToJson();
                    ConfigJson::SetSection(mainConfig_, sectionName, sectionJson);
                }

                // Save to file
                auto configPath = options_.configDirectory / options_.mainConfigFile;
                if (!mainConfig_.ToFile(configPath)) {
                    return ConfigError(ConfigResult::FileNotFound,
                        std::format("Failed to save configuration to: {}", configPath.string()));
                }

                return true;
            }
            catch (const std::exception& e) {
                return ConfigError(ConfigResult::MemoryError,
                    std::format("Failed to save configuration: {}", e.what()));
            }
        }

        ConfigResult_t<bool> LoadAllSectionsImpl() {
            if (!isInitialized_) {
                return ConfigError(ConfigResult::ValidationError, "ConfigManager not initialized");
            }

            try {
                // Reload main config file
                auto configPath = options_.configDirectory / options_.mainConfigFile;
                if (std::filesystem::exists(configPath)) {
                    mainConfig_ = JsonValue::FromFile(configPath);
                }
                else {
                    return ConfigError(ConfigResult::FileNotFound,
                        std::format("Configuration file not found: {}", configPath.string()));
                }

                // Reload all sections
                for (const auto& [sectionName, section] : configSections_) {
                    if (mainConfig_.Contains(sectionName)) {
                        auto sectionJson = mainConfig_[sectionName];
                        auto loadResult = section->LoadFromJson(sectionJson);
                        if (!loadResult) {
                            return ConfigError(loadResult.Error().result,
                                std::format("Failed to reload section '{}': {}", sectionName, loadResult.Error().message));
                        }
                    }
                }

                return true;
            }
            catch (const std::exception& e) {
                return ConfigError(ConfigResult::ParseError,
                    std::format("Failed to load configuration: {}", e.what()));
            }
        }

        // ========================================================================
        // Private Members
        // ========================================================================

        bool isInitialized_ = false;
        InitializationOptions options_;
        JsonValue mainConfig_;
        std::unordered_map<std::string, std::shared_ptr<IConfigSection>> configSections_;
    };

    // ============================================================================
    // Global Configuration Access Helpers
    // ============================================================================

    // Convenience functions for common operations
    template<typename T>
    std::shared_ptr<T> GetConfig() {
        static_assert(is_config_section_v<T>, "T must be a configuration section");
        return ConfigManager::GetOrCreateSection<T>();
    }

    template<typename T>
    ConfigResult_t<std::shared_ptr<T>> RegisterConfig() {
        static_assert(is_config_section_v<T>, "T must be a configuration section");
        return ConfigManager::RegisterSection<T>();
    }

    inline ConfigResult_t<bool> InitializeConfig(const ConfigManager::InitializationOptions& options = {}) {
        bool result = ConfigManager::Initialize(options);
        return result ? ConfigResult_t<bool>(true) : ConfigError(ConfigResult::MemoryError, "Failed to initialize configuration manager");
    }

    inline void ShutdownConfig() {
        ConfigManager::Shutdown();
    }

    inline ConfigResult_t<bool> SaveConfig() {
        return ConfigManager::SaveAllSections();
    }

    inline ConfigResult_t<bool> ReloadConfig() {
        return ConfigManager::LoadAllSections();
    }

    // Helper for easy section registration
    template<typename T>
    inline void RegisterConfigSection() {
        static_assert(is_config_section_v<T>, "T must be a configuration section");
        // For the simplified version, just ensure the section exists
        GetConfig<T>();
    }

} // namespace Akhanda::Configuration