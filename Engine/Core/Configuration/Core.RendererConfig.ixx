// Core.RendererConfig.ixx (Fixed with Traditional Header)
// Akhanda Game Engine - Renderer Configuration Module Interface  
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include "JsonWrapper.hpp"

export module Akhanda.Core.Configuration.Rendering;

import Akhanda.Core.Configuration;
import Akhanda.Core.Configuration.Manager;

export namespace Akhanda::Configuration {

    // ============================================================================
    // Rendering API Enumeration
    // ============================================================================

    enum class RenderingAPI : std::uint32_t {
        D3D12 = 0,
        Vulkan = 1,     // Future support
        D3D11 = 2       // Legacy fallback
    };

    inline std::string RenderingAPIToString(RenderingAPI api) {
        switch (api) {
        case RenderingAPI::D3D12: return "D3D12";
        case RenderingAPI::Vulkan: return "Vulkan";
        case RenderingAPI::D3D11: return "D3D11";
        default: return "Unknown";
        }
    }

    inline ConfigResult_t<RenderingAPI> StringToRenderingAPI(const std::string& str) {
        if (str == "D3D12") return RenderingAPI::D3D12;
        if (str == "Vulkan") return RenderingAPI::Vulkan;
        if (str == "D3D11") return RenderingAPI::D3D11;

        return ConfigError(ConfigResult::ValidationError,
            std::format("Unknown rendering API: '{}'", str));
    }

    // ============================================================================
    // MSAA Sample Count Enumeration
    // ============================================================================

    enum class MSAASamples : std::uint32_t {
        None = 1,
        MSAA2x = 2,
        MSAA4x = 4,
        MSAA8x = 8,
        MSAA16x = 16
    };

    inline std::string MSAASamplesToString(MSAASamples samples) {
        switch (samples) {
        case MSAASamples::None: return "None";
        case MSAASamples::MSAA2x: return "2x";
        case MSAASamples::MSAA4x: return "4x";
        case MSAASamples::MSAA8x: return "8x";
        case MSAASamples::MSAA16x: return "16x";
        default: return "Unknown";
        }
    }

    inline ConfigResult_t<MSAASamples> StringToMSAASamples(const std::string& str) {
        if (str == "None" || str == "1x") return MSAASamples::None;
        if (str == "2x") return MSAASamples::MSAA2x;
        if (str == "4x") return MSAASamples::MSAA4x;
        if (str == "8x") return MSAASamples::MSAA8x;
        if (str == "16x") return MSAASamples::MSAA16x;

        return ConfigError(ConfigResult::ValidationError,
            std::format("Unknown MSAA samples: '{}'", str));
    }

    // ============================================================================
    // NEW: Shader System Configuration Enumerations
    // ============================================================================

    enum class ShaderCompilationMode : std::uint32_t {
        RuntimeOnly = 0,        // Compile shaders at runtime from HLSL
        PrecompiledOnly = 1,    // Use only pre-compiled .cso files
        Hybrid = 2              // Try precompiled first, fallback to runtime
    };

    inline std::string ShaderCompilationModeToString(ShaderCompilationMode mode) {
        switch (mode) {
        case ShaderCompilationMode::RuntimeOnly: return "RuntimeOnly";
        case ShaderCompilationMode::PrecompiledOnly: return "PrecompiledOnly";
        case ShaderCompilationMode::Hybrid: return "Hybrid";
        default: return "Unknown";
        }
    }

    inline ConfigResult_t<ShaderCompilationMode> StringToShaderCompilationMode(const std::string& str) {
        if (str == "RuntimeOnly") return ShaderCompilationMode::RuntimeOnly;
        if (str == "PrecompiledOnly") return ShaderCompilationMode::PrecompiledOnly;
        if (str == "Hybrid") return ShaderCompilationMode::Hybrid;

        return ConfigError(ConfigResult::ValidationError,
            std::format("Unknown shader compilation mode: '{}'", str));
    }

    enum class ShaderOptimization : std::uint32_t {
        Debug = 0,              // No optimization, full debug info (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION)
        Release = 1,            // Full optimization (D3DCOMPILE_OPTIMIZATION_LEVEL3)
        Size = 2,               // Optimize for size (D3DCOMPILE_OPTIMIZATION_LEVEL1)
        Speed = 3               // Optimize for speed (D3DCOMPILE_OPTIMIZATION_LEVEL3)
    };

    inline std::string ShaderOptimizationToString(ShaderOptimization opt) {
        switch (opt) {
        case ShaderOptimization::Debug: return "Debug";
        case ShaderOptimization::Release: return "Release";
        case ShaderOptimization::Size: return "Size";
        case ShaderOptimization::Speed: return "Speed";
        default: return "Unknown";
        }
    }

    inline ConfigResult_t<ShaderOptimization> StringToShaderOptimization(const std::string& str) {
        if (str == "Debug") return ShaderOptimization::Debug;
        if (str == "Release") return ShaderOptimization::Release;
        if (str == "Size") return ShaderOptimization::Size;
        if (str == "Speed") return ShaderOptimization::Speed;

        return ConfigError(ConfigResult::ValidationError,
            std::format("Unknown shader optimization level: '{}'", str));
    }

    enum class ShaderModel : std::uint32_t {
        SM_5_0 = 0,            // Shader Model 5.0 (legacy compatibility)
        SM_5_1 = 1,            // Shader Model 5.1 (D3D12 minimum)
        SM_6_0 = 2,            // Shader Model 6.0 (DXIL)
        SM_6_1 = 3,            // Shader Model 6.1
        SM_6_2 = 4,            // Shader Model 6.2  
        SM_6_3 = 5,            // Shader Model 6.3
        SM_6_4 = 6,            // Shader Model 6.4
        SM_6_5 = 7,            // Shader Model 6.5
        SM_6_6 = 8,            // Shader Model 6.6
        SM_6_7 = 9             // Shader Model 6.7 (latest)
    };

    inline std::string ShaderModelToString(ShaderModel model) {
        switch (model) {
        case ShaderModel::SM_5_0: return "5_0";
        case ShaderModel::SM_5_1: return "5_1";
        case ShaderModel::SM_6_0: return "6_0";
        case ShaderModel::SM_6_1: return "6_1";
        case ShaderModel::SM_6_2: return "6_2";
        case ShaderModel::SM_6_3: return "6_3";
        case ShaderModel::SM_6_4: return "6_4";
        case ShaderModel::SM_6_5: return "6_5";
        case ShaderModel::SM_6_6: return "6_6";
        case ShaderModel::SM_6_7: return "6_7";
        default: return "Unknown";
        }
    }

    inline ConfigResult_t<ShaderModel> StringToShaderModel(const std::string& str) {
        if (str == "5_0" || str == "5.0") return ShaderModel::SM_5_0;
        if (str == "5_1" || str == "5.1") return ShaderModel::SM_5_1;
        if (str == "6_0" || str == "6.0") return ShaderModel::SM_6_0;
        if (str == "6_1" || str == "6.1") return ShaderModel::SM_6_1;
        if (str == "6_2" || str == "6.2") return ShaderModel::SM_6_2;
        if (str == "6_3" || str == "6.3") return ShaderModel::SM_6_3;
        if (str == "6_4" || str == "6.4") return ShaderModel::SM_6_4;
        if (str == "6_5" || str == "6.5") return ShaderModel::SM_6_5;
        if (str == "6_6" || str == "6.6") return ShaderModel::SM_6_6;
        if (str == "6_7" || str == "6.7") return ShaderModel::SM_6_7;

        return ConfigError(ConfigResult::ValidationError,
            std::format("Unknown shader model: '{}'", str));
    }

    // ============================================================================
    // Shader Configuration Structure
    // ============================================================================
    class ShaderConfig : public IConfigSection {
    private:
        // Compilation settings
        ConfigValue<ShaderCompilationMode> compilationMode{ ShaderCompilationMode::Hybrid };
        ConfigValue<ShaderOptimization> optimization{ ShaderOptimization::Debug };
        ConfigValue<ShaderModel> targetShaderModel{ ShaderModel::SM_6_0 };

        // File paths
        ConfigValue<std::string> shaderSourcePath{ "Shaders/" };
        ConfigValue<std::string> precompiledPath{ "Shaders/Compiled/" };
        ConfigValue<std::string> shaderCachePath{ "Cache/Shaders/" };

        // Hot-reload settings
        ConfigValue<bool> enableHotReload{ true };
        ConfigValue<std::uint32_t> hotReloadCheckIntervalMs{ 500, 100, 10000 };  // Check for file changes every 500ms
        ConfigValue<bool> autoRecompileOnChange{ true };

        // Cache settings
        ConfigValue<bool> enableShaderCache{ true };
        ConfigValue<std::uint32_t> maxCacheSize{ 512, 64, 4096 }; // MB
        ConfigValue<bool> validateCacheOnStartup{ true };

        // Compilation options
        ConfigValue<std::vector<std::string>> globalDefines;       // Global shader defines
        ConfigValue<std::vector<std::string>> includePaths;        // Additional include paths
        ConfigValue<bool> generateDebugInfo{ true };                // Include debug symbols
        ConfigValue<bool> enableStrictMode{ true };                 // Treat warnings as errors
        ConfigValue<bool> enableMatrix16ByteAlignment{ true };     // Force 16-byte matrix alignment

        // Advanced settings
        ConfigValue<bool> enableShaderReflection{ true };          // Enable automatic reflection
        ConfigValue<bool> cacheReflectionData{ true };             // Cache reflection results
        ConfigValue<std::uint32_t> maxShaderVariants{ 1024, 32, 8192 };      // Maximum shader variants to cache

        // Development features
        ConfigValue<bool> dumpCompiledShaders{ false };            // Dump compiled bytecode for debugging
        ConfigValue<bool> logCompilationTimes{ true };             // Log shader compilation performance
        ConfigValue<bool> enableShaderProfiling{ false };         // Enable shader profiling markers

        // Error handling
        ConfigValue<bool> treatWarningsAsErrors{ false };          // HLSL compiler warning handling
        ConfigValue<std::uint32_t> maxCompilationRetries{ 3 };    // Retry failed compilations

    public:
        ShaderConfig() = default;
        ~ShaderConfig() override = default;

        // ========================================================================
        // IConfigSection Implementation
        // ========================================================================

        const char* GetSectionName() const noexcept override {
            return "shader";
        }

        ConfigResult_t<bool> Validate() const override {
            // All validation is handled by individual ConfigValue instances
            return true;
        }

        std::vector<std::string> GetValidationErrors() const override {
            // Return any validation errors
            return {};
        }

        void ResetToDefaults() override {
            compilationMode.Reset();
            optimization.Reset();
            targetShaderModel.Reset();
            shaderSourcePath.Reset();
            precompiledPath.Reset();
            shaderCachePath.Reset();
            enableHotReload.Reset();
            hotReloadCheckIntervalMs.Reset();
            autoRecompileOnChange.Reset();
            enableShaderCache.Reset();
            maxCacheSize.Reset();
            validateCacheOnStartup.Reset();
            generateDebugInfo.Reset();
            enableStrictMode.Reset();
            enableMatrix16ByteAlignment.Reset();
            enableShaderReflection.Reset();
            cacheReflectionData.Reset();
            maxShaderVariants.Reset();
            dumpCompiledShaders.Reset();
            logCompilationTimes.Reset();
            enableShaderProfiling.Reset();
            treatWarningsAsErrors.Reset();
            maxCompilationRetries.Reset();
        }

        std::string GetDescription() const override {
            return "Shader configuration settings";
        }

        bool RequiresRestart() const override {
            return true; // Most rendering changes require restart
        }

        JsonValue SaveToJson() const override {

            auto j = ConfigJson::CreateObject();

            // Basic compilation settings
            SetConfigValue(j, "compilationMode", ShaderCompilationModeToString(compilationMode.Get()));
            SetConfigValue(j, "optimization", ShaderOptimizationToString(optimization.Get()));
            SetConfigValue(j, "targetShaderModel", ShaderModelToString(targetShaderModel.Get()));

            // File paths
            SetConfigValue(j, "shaderSourcePath", shaderSourcePath.Get());
            SetConfigValue(j, "precompiledPath", precompiledPath.Get());
            SetConfigValue(j, "shaderCachePath", shaderCachePath.Get());

            // Hot-reload settings
            SetConfigValue(j, "enableHotReload", enableHotReload.Get());
            SetConfigValue(j, "hotReloadCheckIntervalMs", hotReloadCheckIntervalMs.Get());
            SetConfigValue(j, "autoRecompileOnChange", autoRecompileOnChange.Get());

            // Cache settings
            SetConfigValue(j, "enableShaderCache", enableShaderCache.Get());
            SetConfigValue(j, "maxCacheSize", maxCacheSize.Get());
            SetConfigValue(j, "validateCacheOnStartup", validateCacheOnStartup.Get());

            // Compilation options
            SetConfigValue(j, "globalDefines", globalDefines.Get());
            SetConfigValue(j, "includePaths", includePaths.Get());
            SetConfigValue(j, "generateDebugInfo", generateDebugInfo.Get());
            SetConfigValue(j,"enableStrictMode", enableStrictMode.Get());
            SetConfigValue(j, "enableMatrix16ByteAlignment", enableMatrix16ByteAlignment.Get());

            // Advanced settings
            SetConfigValue(j, "enableShaderReflection", enableShaderReflection.Get());
            SetConfigValue(j, "cacheReflectionData", cacheReflectionData.Get());
            SetConfigValue(j, "maxShaderVariants", maxShaderVariants.Get());

            // Development features
            SetConfigValue(j, "dumpCompiledShaders", dumpCompiledShaders.Get());
            SetConfigValue(j, "logCompilationTimes", logCompilationTimes.Get());
            SetConfigValue(j, "enableShaderProfiling", enableShaderProfiling.Get());

            // Error handling
            SetConfigValue(j, "treatWarningsAsErrors", treatWarningsAsErrors.Get());
            SetConfigValue(j, "maxCompilationRetries", maxCompilationRetries.Get());

            return j;
        }

        ConfigResult_t<bool> LoadFromJson(const JsonValue& json) override {
            try {
                // Parse compilation settings
                if (json.Contains("compilationMode")) {
                    auto res_string = GetConfigValue<std::string>(json, "compilationMode");
                    auto result = StringToShaderCompilationMode(res_string);
                    if (!result) return ConfigError(ConfigResult::ValidationError, result.Error().message);
                    compilationMode.Set(result.Value());
                }

                if (json.Contains("optimization")) {
                    auto res_string = GetConfigValue<std::string>(json, "optimization");
                    auto result = StringToShaderOptimization(res_string);
                    if (!result) return ConfigError(ConfigResult::ValidationError, result.Error().message);
                    optimization.Set(result.Value());
                }

                if (json.Contains("targetShaderModel")) {
                    auto res_string = GetConfigValue<std::string>(json, "targetShaderModel");
                    auto result = StringToShaderModel(res_string);
                    if (!result) return ConfigError(ConfigResult::ValidationError, result.Error().message);
                    targetShaderModel.Set(result.Value());
                }

                // Parse file paths
                shaderSourcePath.Set(GetConfigValue<std::string>(json, "shaderSourcePath", "Shaders/"));
                precompiledPath.Set(GetConfigValue<std::string>(json, "precompiledPath", "Shaders/Compiled/"));
                shaderCachePath.Set(GetConfigValue<std::string>(json, "shaderCachePath", "Cache/Shaders/"));

                // Parse hot-reload settings
                enableHotReload.Set(GetConfigValue<bool>(json, "enableHotReload", true));
                hotReloadCheckIntervalMs.Set(GetConfigValue<uint32_t>(json, "hotReloadCheckIntervalMs", 500));
                autoRecompileOnChange.Set(GetConfigValue<bool>(json, "autoRecompileOnChange", true));

                // Parse cache settings
                enableShaderCache.Set(GetConfigValue<bool>(json, "enableShaderCache", true));
                maxCacheSize.Set(GetConfigValue<uint32_t>(json, "maxCacheSize", 512));
                validateCacheOnStartup.Set(GetConfigValue<bool>(json, "validateCacheOnStartup", true));

                // Parse compilation options
                globalDefines.Set(GetConfigValue<std::vector<std::string>>(json, "globalDefines"));
                includePaths.Set(GetConfigValue<std::vector<std::string>>(json, "includePaths"));
                generateDebugInfo.Set(GetConfigValue<bool>(json, "generateDebugInfo", true));
                enableStrictMode.Set(GetConfigValue<bool>(json, "enableStrictMode", true));
                enableMatrix16ByteAlignment.Set(GetConfigValue<bool>(json, "enableMatrix16ByteAlignment", true));

                // Parse advanced settings
                enableShaderReflection.Set(GetConfigValue<bool>(json, "enableShaderReflection", true));
                cacheReflectionData.Set(GetConfigValue<bool>(json, "cacheReflectionData", true));
                maxShaderVariants.Set(GetConfigValue<uint32_t>(json, "maxShaderVariants", 1024));

                // Parse development features
                dumpCompiledShaders.Set(GetConfigValue(json, "dumpCompiledShaders", false));
                logCompilationTimes.Set(GetConfigValue(json, "logCompilationTimes", true));
                enableShaderProfiling.Set(GetConfigValue(json, "enableShaderProfiling", false));

                // Parse error handling
                treatWarningsAsErrors.Set(GetConfigValue(json, "treatWarningsAsErrors", false));
                maxCompilationRetries.Set(GetConfigValue(json, "maxCompilationRetries", 3));

                return Validate();

            }
            catch (const std::exception& e) {
                return ConfigError(ConfigResult::ParseError,
                    std::format("Failed to parse shader configuration: {}", e.what()));
            }
        }

        // Getters
        ShaderCompilationMode GetCompilationMode() const { return compilationMode.Get(); }
        ShaderOptimization GetOptimization() const { return optimization.Get(); }
        ShaderModel GetTargetShaderModel() const { return targetShaderModel.Get(); }
        const std::string& GetShaderSourcePath() const { return shaderSourcePath.Get(); }
        const std::string& GetPrecompiledPath() const { return precompiledPath.Get(); }
        const std::string& GetShaderCachePath() const { return shaderCachePath.Get(); }
        bool GetEnableHotReload() const { return enableHotReload.Get(); }
        std::uint32_t GetHotReloadCheckIntervalMs() const { return hotReloadCheckIntervalMs.Get(); }
        bool GetAutoRecompileOnChange() const { return autoRecompileOnChange.Get(); }
        bool GetEnableShaderCache() const { return enableShaderCache.Get(); }
        std::uint32_t GetMaxCacheSize() const { return maxCacheSize.Get(); }
        bool GetValidateCacheOnStartup() const { return validateCacheOnStartup.Get(); }
        const std::vector<std::string>& GetGlobalDefines() const { return globalDefines.Get(); }
        const std::vector<std::string>& GetIncludePaths() const { return includePaths.Get(); }
        bool GetGenerateDebugInfo() const { return generateDebugInfo.Get(); }
        bool GetEnableStrictMode() const { return enableStrictMode.Get(); }
        bool GetEnableMatrix16ByteAlignment() const { return enableMatrix16ByteAlignment.Get(); }
        bool GetEnableShaderReflection() const { return enableShaderReflection.Get(); }
        bool GetCacheReflectionData() const { return cacheReflectionData.Get(); }
        std::uint32_t GetMaxShaderVariants() const { return maxShaderVariants.Get(); }
        bool GetDumpCompiledShaders() const { return dumpCompiledShaders.Get(); }
        bool GetLogCompilationTimes() const { return logCompilationTimes.Get(); }
        bool GetEnableShaderProfiling() const { return enableShaderProfiling.Get(); }
        bool GetTreatWarningsAsErrors() const { return treatWarningsAsErrors.Get(); }
        std::uint32_t GetMaxCompilationRetries() const { return maxCompilationRetries.Get(); }

        // Setters
        void SetCompilationMode(ShaderCompilationMode mode) { compilationMode.Set(mode); }
        void SetOptimization(ShaderOptimization opt) { optimization.Set(opt); }
        void SetTargetShaderModel(ShaderModel model) { targetShaderModel.Set(model); }
        void SetShaderSourcePath(const std::string& path) { shaderSourcePath.Set(path); }
        void SetPrecompiledPath(const std::string& path) { precompiledPath.Set(path); }
        void SetShaderCachePath(const std::string& path) { shaderCachePath.Set(path); }
        void SetEnableHotReload(bool enable) { enableHotReload.Set(enable); }
        void SetHotReloadCheckIntervalMs(std::uint32_t interval) { hotReloadCheckIntervalMs.Set(interval); }
        void SetAutoRecompileOnChange(bool enable) { autoRecompileOnChange.Set(enable); }
        void SetEnableShaderCache(bool enable) { enableShaderCache.Set(enable); }
        void SetMaxCacheSize(std::uint32_t size) { maxCacheSize.Set(size); }
        void SetValidateCacheOnStartup(bool enable) { validateCacheOnStartup.Set(enable); }
        void SetGlobalDefines(const std::vector<std::string>& defines) { globalDefines.Set(defines); }
        void SetIncludePaths(const std::vector<std::string>& paths) { includePaths.Set(paths); }
        void SetGenerateDebugInfo(bool enable) { generateDebugInfo.Set(enable); }
        void SetEnableStrictMode(bool enable) { enableStrictMode.Set(enable); }
        void SetEnableMatrix16ByteAlignment(bool enable) { enableMatrix16ByteAlignment.Set(enable); }
        void SetEnableShaderReflection(bool enable) { enableShaderReflection.Set(enable); }
        void SetCacheReflectionData(bool enable) { cacheReflectionData.Set(enable); }
        void SetMaxShaderVariants(std::uint32_t count) { maxShaderVariants.Set(count); }
        void SetDumpCompiledShaders(bool enable) { dumpCompiledShaders.Set(enable); }
        void SetLogCompilationTimes(bool enable) { logCompilationTimes.Set(enable); }
        void SetEnableShaderProfiling(bool enable) { enableShaderProfiling.Set(enable); }
        void SetTreatWarningsAsErrors(bool enable) { treatWarningsAsErrors.Set(enable); }
        void SetMaxCompilationRetries(std::uint32_t count) { maxCompilationRetries.Set(count); }

        void AddGlobalDefine(const std::string& define) {
            auto defines = globalDefines.Get();
            defines.push_back(define);
            globalDefines.Set(defines);
        }

        void AddIncludePath(const std::string& path) {
            auto paths = includePaths.Get();
            paths.push_back(path);
            includePaths.Set(paths);
        }
    };

    // ============================================================================
    // Rendering Configuration Section
    // ============================================================================

    class RenderingConfig : public IConfigSection {
    public:
        RenderingConfig() {
            // Initialize with sensible defaults
            renderingAPI_.Set(RenderingAPI::D3D12);
            resolution_.Set(Resolution{ 1280, 720 });
            fullscreen_.Set(false);
            vsync_.Set(true);
            msaaSamples_.Set(MSAASamples::None);
            maxFPS_.Set(0); // 0 = unlimited
            hdrEnabled_.Set(false);
            adapterPreference_.Set(0); // Primary adapter
            debugLayerEnabled_.Set(false);
            gpuValidationEnabled_.Set(false);

            SetupValidation();
        }

        // ========================================================================
        // IConfigSection Implementation
        // ========================================================================

        const char* GetSectionName() const noexcept override {
            return "rendering";
        }

        ConfigResult_t<bool> LoadFromJson(const JsonValue& sectionJson) override {
            try {
                // Load each configuration value
                if (sectionJson.Contains("renderingAPI")) {
                    std::string apiStr = GetConfigValue<std::string>(sectionJson, "renderingAPI");
                    auto apiResult = StringToRenderingAPI(apiStr);
                    if (!apiResult) return ConfigError(ConfigResult::ValidationError, apiResult.Error().message);
                    renderingAPI_.Set(apiResult.Value());
                }

                if (sectionJson.Contains("resolution")) {
                    auto resJson = sectionJson["resolution"];
                    auto resResult = Resolution::FromJson(resJson);
                    if (!resResult) return ConfigError(ConfigResult::ValidationError, resResult.Error().message);
                    resolution_.Set(resResult.Value());
                }

                if (sectionJson.Contains("shaders")) {
                    shaderConfig_.LoadFromJson(sectionJson["shaders"]);
                }

                if (sectionJson.Contains("fullscreen")) {
                    fullscreen_.Set(GetConfigValue<bool>(sectionJson, "fullscreen"));
                }

                if (sectionJson.Contains("vsync")) {
                    vsync_.Set(GetConfigValue<bool>(sectionJson, "vsync"));
                }

                if (sectionJson.Contains("msaaSamples")) {
                    std::string msaaStr = GetConfigValue<std::string>(sectionJson, "msaaSamples");
                    auto msaaResult = StringToMSAASamples(msaaStr);
                    if (!msaaResult) return ConfigError(ConfigResult::ValidationError, msaaResult.Error().message);
                    msaaSamples_.Set(msaaResult.Value());
                }

                if (sectionJson.Contains("maxFPS")) {
                    maxFPS_.Set(GetConfigValue<std::uint32_t>(sectionJson, "maxFPS"));
                }

                if (sectionJson.Contains("hdrEnabled")) {
                    hdrEnabled_.Set(GetConfigValue<bool>(sectionJson, "hdrEnabled"));
                }

                if (sectionJson.Contains("adapterPreference")) {
                    adapterPreference_.Set(GetConfigValue<std::uint32_t>(sectionJson, "adapterPreference"));
                }

                if (sectionJson.Contains("debugLayerEnabled")) {
                    debugLayerEnabled_.Set(GetConfigValue<bool>(sectionJson, "debugLayerEnabled"));
                }

                if (sectionJson.Contains("gpuValidationEnabled")) {
                    gpuValidationEnabled_.Set(GetConfigValue<bool>(sectionJson, "gpuValidationEnabled"));
                }

                return true;
            }
            catch (const std::exception& e) {
                return ConfigError(ConfigResult::ParseError,
                    std::format("Failed to load rendering configuration: {}", e.what()));
            }
        }

        JsonValue SaveToJson() const override {
            auto j = ConfigJson::CreateObject();

            SetConfigValue(j, "renderingAPI", RenderingAPIToString(renderingAPI_.Get()));
            j.SetMember("resolution", resolution_.Get().ToJson());
            j.SetMember("shaders", shaderConfig_.SaveToJson());
            SetConfigValue(j, "fullscreen", fullscreen_.Get());
            SetConfigValue(j, "vsync", vsync_.Get());
            SetConfigValue(j, "msaaSamples", MSAASamplesToString(msaaSamples_.Get()));
            SetConfigValue(j, "maxFPS", maxFPS_.Get());
            SetConfigValue(j, "hdrEnabled", hdrEnabled_.Get());
            SetConfigValue(j, "adapterPreference", adapterPreference_.Get());
            SetConfigValue(j, "debugLayerEnabled", debugLayerEnabled_.Get());
            SetConfigValue(j, "gpuValidationEnabled", gpuValidationEnabled_.Get());

            return j;
        }

        ConfigResult_t<bool> Validate() const override {
            // All validation is handled by individual ConfigValue instances
            return true;
        }

        std::vector<std::string> GetValidationErrors() const override {
            // Return any validation errors
            return {};
        }

        void ResetToDefaults() override {
            renderingAPI_.Reset();
            resolution_.Reset();
            fullscreen_.Reset();
            vsync_.Reset();
            msaaSamples_.Reset();
            maxFPS_.Reset();
            hdrEnabled_.Reset();
            adapterPreference_.Reset();
            debugLayerEnabled_.Reset();
            gpuValidationEnabled_.Reset();
            shaderConfig_.ResetToDefaults();
        }

        std::string GetDescription() const override {
            return "Graphics and rendering configuration settings";
        }

        bool RequiresRestart() const override {
            return true; // Most rendering changes require restart
        }

        // ========================================================================
        // Configuration Access Methods
        // ========================================================================

        // Getters
        RenderingAPI GetRenderingAPI() const { return renderingAPI_.Get(); }
        Resolution GetResolution() const { return resolution_.Get(); }
        bool GetFullscreen() const { return fullscreen_.Get(); }
        bool GetVsync() const { return vsync_.Get(); }
        MSAASamples GetMsaaSamples() const { return msaaSamples_.Get(); }
        std::uint32_t GetMaxFPS() const { return maxFPS_.Get(); }
        bool GetHdrEnabled() const { return hdrEnabled_.Get(); }
        std::uint32_t GetAdapterPreference() const { return adapterPreference_.Get(); }
        bool GetDebugLayerEnabled() const { return debugLayerEnabled_.Get(); }
        bool GetGpuValidationEnabled() const { return gpuValidationEnabled_.Get(); }
        const ShaderConfig& GetShaderConfig() const { return shaderConfig_; }

        // Setters
        ConfigResult_t<bool> SetRenderingAPI(RenderingAPI api) { return renderingAPI_.Set(api); }
        ConfigResult_t<bool> SetResolution(const Resolution& res) { return resolution_.Set(res); }
        ConfigResult_t<bool> SetFullscreen(bool fullscreen) { return fullscreen_.Set(fullscreen); }
        ConfigResult_t<bool> SetVsync(bool vsync) { return vsync_.Set(vsync); }
        ConfigResult_t<bool> SetMsaaSamples(MSAASamples samples) { return msaaSamples_.Set(samples); }
        ConfigResult_t<bool> SetMaxFPS(std::uint32_t fps) { return maxFPS_.Set(fps); }
        ConfigResult_t<bool> SetHdrEnabled(bool enabled) { return hdrEnabled_.Set(enabled); }
        ConfigResult_t<bool> SetAdapterPreference(std::uint32_t adapter) { return adapterPreference_.Set(adapter); }
        ConfigResult_t<bool> SetDebugLayerEnabled(bool enabled) { return debugLayerEnabled_.Set(enabled); }
        ConfigResult_t<bool> SetGpuValidationEnabled(bool enabled) { return gpuValidationEnabled_.Set(enabled); }

    private:
        void SetupValidation() {
            // Set up resolution validation
            resolution_.SetValidator([](const Resolution& res) -> bool {
                // Validate resolution is reasonable
                if (res.width < 320 || res.width > 7680) return false;
                if (res.height < 240 || res.height > 4320) return false;

                // Must be divisible by 2 for GPU compatibility
                if (res.width % 2 != 0 || res.height % 2 != 0) return false;

                return true;
            });

            // Custom validation for max FPS
            maxFPS_.SetValidator([](std::uint32_t fps) -> bool {
                // 0 = unlimited is valid, otherwise must be at least 10
                return fps == 0 || fps >= 10;
            });
        }

        // ========================================================================
        // Configuration Values
        // ========================================================================

        ConfigValue<RenderingAPI> renderingAPI_;
        ConfigValue<Resolution> resolution_;
        ConfigValue<bool> fullscreen_;
        ConfigValue<bool> vsync_;
        ConfigValue<MSAASamples> msaaSamples_;
        ConfigValue<std::uint32_t> maxFPS_;
        ConfigValue<bool> hdrEnabled_;
        ConfigValue<std::uint32_t> adapterPreference_;
        ConfigValue<bool> debugLayerEnabled_;
        ConfigValue<bool> gpuValidationEnabled_;
        ShaderConfig shaderConfig_;
    };

    // ========================================================================
    // Convenience Functions
    // ========================================================================

    inline std::shared_ptr<RenderingConfig> GetRenderingConfig() {
        return GetConfig<RenderingConfig>();
    }

    inline void RegisterRenderingConfig() {
        RegisterConfigSection<RenderingConfig>();
    }

} // namespace Akhanda::Configuration

// ============================================================================
// std::format Support for Configuration Types
// ============================================================================

template<>
struct std::formatter<Akhanda::Configuration::RenderingAPI> : std::formatter<std::string> {
    auto format(Akhanda::Configuration::RenderingAPI api, format_context& ctx) const {
        return std::formatter<std::string>::format(Akhanda::Configuration::RenderingAPIToString(api), ctx);
    }
};

template<>
struct std::formatter<Akhanda::Configuration::MSAASamples> : std::formatter<std::string> {
    auto format(Akhanda::Configuration::MSAASamples samples, format_context& ctx) const {
        return std::formatter<std::string>::format(Akhanda::Configuration::MSAASamplesToString(samples), ctx);
    }
};

template<>
struct std::formatter<Akhanda::Configuration::Resolution> : std::formatter<std::string> {
    auto format(const Akhanda::Configuration::Resolution& res, format_context& ctx) const {
        return std::formatter<std::string>::format(
            std::format("{}x{}", res.width, res.height), ctx);
    }
};