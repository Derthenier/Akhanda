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