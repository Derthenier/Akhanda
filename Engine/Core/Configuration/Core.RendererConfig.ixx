// Core.RendererConfig.ixx
// Akhanda Game Engine - Renderer Configuration Module Interface
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <nlohmann/json.hpp>

export module Akhanda.Core.Configuration.Rendering;

import Akhanda.Core.Configuration;
import std;

export namespace Akhanda::Configuration {

    // ============================================================================
    // Rendering API Enumeration
    // ============================================================================

    enum class RenderingAPI : uint32_t {
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

    // JSON serialization for RenderingAPI
    inline void to_json(json& j, const RenderingAPI& api) {
        j = RenderingAPIToString(api);
    }

    inline void from_json(const json& j, RenderingAPI& api) {
        auto result = StringToRenderingAPI(j.get<std::string>());
        if (!result) {
            throw json::other_error::create(0, result.Error().message, nullptr);
        }
        api = result.Value();
    }

    // ============================================================================
    // MSAA Sample Count Enumeration
    // ============================================================================

    enum class MSAASamples : uint32_t {
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
        default: return "None";
        }
    }

    inline ConfigResult_t<MSAASamples> StringToMSAASamples(const std::string& str) {
        if (str == "None" || str == "1") return MSAASamples::None;
        if (str == "2x" || str == "2") return MSAASamples::MSAA2x;
        if (str == "4x" || str == "4") return MSAASamples::MSAA4x;
        if (str == "8x" || str == "8") return MSAASamples::MSAA8x;
        if (str == "16x" || str == "16") return MSAASamples::MSAA16x;

        return ConfigError(ConfigResult::ValidationError,
            std::format("Invalid MSAA sample count: '{}'", str));
    }

    // JSON serialization for MSAASamples
    inline void to_json(json& j, const MSAASamples& samples) {
        j = static_cast<uint32_t>(samples);
    }

    inline void from_json(const json& j, MSAASamples& samples) {
        uint32_t value = j.get<uint32_t>();
        switch (value) {
        case 1: samples = MSAASamples::None; break;
        case 2: samples = MSAASamples::MSAA2x; break;
        case 4: samples = MSAASamples::MSAA4x; break;
        case 8: samples = MSAASamples::MSAA8x; break;
        case 16: samples = MSAASamples::MSAA16x; break;
        default:
            throw json::other_error::create(0,
                std::format("Invalid MSAA sample count: {}", value), nullptr);
        }
    }

    // ============================================================================
    // Rendering Configuration Section
    // ============================================================================

    class RenderingConfig : public IConfigSection {
        AKH_DECLARE_CONFIG_SECTION(RenderingConfig, "rendering");

    public:
        RenderingConfig() {
            InitializeDefaults();
            SetupValidation();
        }

        // ========================================================================
        // IConfigSection Implementation
        // ========================================================================

        ConfigResult_t<bool> LoadFromJson(const json& sectionJson) override {
            try {
                // Load basic settings
                if (sectionJson.contains("api")) {
                    auto apiResult = renderingAPI_.Set(sectionJson["api"].get<RenderingAPI>());
                    if (!apiResult) return apiResult.Error();
                }

                if (sectionJson.contains("resolution")) {
                    auto resResult = resolution_.Set(sectionJson["resolution"].get<Resolution>());
                    if (!resResult) return resResult.Error();
                }

                if (sectionJson.contains("fullscreen")) {
                    auto fsResult = fullscreen_.Set(sectionJson["fullscreen"].get<bool>());
                    if (!fsResult) return fsResult.Error();
                }

                if (sectionJson.contains("vsync")) {
                    auto vsyncResult = vsync_.Set(sectionJson["vsync"].get<bool>());
                    if (!vsyncResult) return vsyncResult.Error();
                }

                if (sectionJson.contains("msaa_samples")) {
                    auto msaaResult = msaaSamples_.Set(sectionJson["msaa_samples"].get<MSAASamples>());
                    if (!msaaResult) return msaaResult.Error();
                }

                if (sectionJson.contains("max_fps")) {
                    auto fpsResult = maxFPS_.Set(sectionJson["max_fps"].get<uint32_t>());
                    if (!fpsResult) return fpsResult.Error();
                }

                if (sectionJson.contains("hdr_enabled")) {
                    auto hdrResult = hdrEnabled_.Set(sectionJson["hdr_enabled"].get<bool>());
                    if (!hdrResult) return hdrResult.Error();
                }

                if (sectionJson.contains("adapter_preference")) {
                    auto adapterResult = adapterPreference_.Set(sectionJson["adapter_preference"].get<uint32_t>());
                    if (!adapterResult) return adapterResult.Error();
                }

                if (sectionJson.contains("debug_layer")) {
                    auto debugResult = debugLayerEnabled_.Set(sectionJson["debug_layer"].get<bool>());
                    if (!debugResult) return debugResult.Error();
                }

                if (sectionJson.contains("gpu_validation")) {
                    auto validationResult = gpuValidationEnabled_.Set(sectionJson["gpu_validation"].get<bool>());
                    if (!validationResult) return validationResult.Error();
                }

                // Notify that section has changed
                NotifySectionChanged();
                return true;
            }
            catch (const json::exception& e) {
                return ConfigError(ConfigResult::ParseError,
                    std::format("Failed to parse rendering configuration: {}", e.what()));
            }
        }

        json SaveToJson() const override {
            json j;
            j["api"] = *renderingAPI_;
            j["resolution"] = *resolution_;
            j["fullscreen"] = *fullscreen_;
            j["vsync"] = *vsync_;
            j["msaa_samples"] = *msaaSamples_;
            j["max_fps"] = *maxFPS_;
            j["hdr_enabled"] = *hdrEnabled_;
            j["adapter_preference"] = *adapterPreference_;
            j["debug_layer"] = *debugLayerEnabled_;
            j["gpu_validation"] = *gpuValidationEnabled_;
            return j;
        }

        void ResetToDefaults() override {
            renderingAPI_.ResetToDefault();
            resolution_.ResetToDefault();
            fullscreen_.ResetToDefault();
            vsync_.ResetToDefault();
            msaaSamples_.ResetToDefault();
            maxFPS_.ResetToDefault();
            hdrEnabled_.ResetToDefault();
            adapterPreference_.ResetToDefault();
            debugLayerEnabled_.ResetToDefault();
            gpuValidationEnabled_.ResetToDefault();

            NotifySectionChanged();
        }

        ConfigResult_t<bool> Validate() const override {
            // Check resolution constraints
            const auto& res = *resolution_;
            if (res.width < 640 || res.height < 480) {
                return ConfigError(ConfigResult::ValidationError,
                    "Resolution must be at least 640x480");
            }

            if (res.width > 7680 || res.height > 4320) {
                return ConfigError(ConfigResult::ValidationError,
                    "Resolution cannot exceed 7680x4320");
            }

            // Check aspect ratio sanity
            float aspectRatio = static_cast<float>(res.width) / static_cast<float>(res.height);
            if (aspectRatio < 0.5f || aspectRatio > 5.0f) {
                return ConfigError(ConfigResult::ValidationError,
                    "Invalid aspect ratio - resolution seems incorrect");
            }

            // Check FPS limits
            if (*maxFPS_ > 0 && *maxFPS_ < 10) {
                return ConfigError(ConfigResult::ValidationError,
                    "Maximum FPS must be at least 10 or unlimited (0)");
            }

            // Validate API compatibility
            if (*renderingAPI_ == RenderingAPI::Vulkan) {
                return ConfigError(ConfigResult::ValidationError,
                    "Vulkan API is not yet supported");
            }

            return true;
        }

        std::vector<std::string> GetValidationErrors() const override {
            std::vector<std::string> errors;

            auto validationResult = Validate();
            if (!validationResult) {
                errors.emplace_back(validationResult.Error().message);
            }

            return errors;
        }

        uint32_t GetVersion() const noexcept override {
            return 2; // Increment when adding new fields
        }

        std::string GetDescription() const override {
            return "Graphics and rendering system configuration";
        }

        // ========================================================================
        // Public Access Methods
        // ========================================================================

        // Rendering API
        RenderingAPI GetRenderingAPI() const noexcept { return *renderingAPI_; }
        ConfigResult_t<RenderingAPI> SetRenderingAPI(RenderingAPI api) { return renderingAPI_.Set(api); }

        // Resolution
        const Resolution& GetResolution() const noexcept { return *resolution_; }
        ConfigResult_t<Resolution> SetResolution(const Resolution& res) { return resolution_.Set(res); }
        ConfigResult_t<Resolution> SetResolution(uint32_t width, uint32_t height) {
            return resolution_.Set(Resolution{ width, height });
        }

        // Display mode
        bool IsFullscreen() const noexcept { return *fullscreen_; }
        ConfigResult_t<bool> SetFullscreen(bool enabled) { return fullscreen_.Set(enabled); }

        // VSync
        bool IsVSyncEnabled() const noexcept { return *vsync_; }
        ConfigResult_t<bool> SetVSync(bool enabled) { return vsync_.Set(enabled); }

        // Anti-aliasing
        MSAASamples GetMSAASamples() const noexcept { return *msaaSamples_; }
        ConfigResult_t<MSAASamples> SetMSAASamples(MSAASamples samples) { return msaaSamples_.Set(samples); }

        // Frame rate
        uint32_t GetMaxFPS() const noexcept { return *maxFPS_; }
        ConfigResult_t<uint32_t> SetMaxFPS(uint32_t fps) { return maxFPS_.Set(fps); }
        bool IsFrameRateUnlimited() const noexcept { return *maxFPS_ == 0; }

        // HDR
        bool IsHDREnabled() const noexcept { return *hdrEnabled_; }
        ConfigResult_t<bool> SetHDREnabled(bool enabled) { return hdrEnabled_.Set(enabled); }

        // Adapter selection
        uint32_t GetAdapterPreference() const noexcept { return *adapterPreference_; }
        ConfigResult_t<uint32_t> SetAdapterPreference(uint32_t preference) { return adapterPreference_.Set(preference); }

        // Debug features
        bool IsDebugLayerEnabled() const noexcept { return *debugLayerEnabled_; }
        ConfigResult_t<bool> SetDebugLayerEnabled(bool enabled) { return debugLayerEnabled_.Set(enabled); }

        bool IsGPUValidationEnabled() const noexcept { return *gpuValidationEnabled_; }
        ConfigResult_t<bool> SetGPUValidationEnabled(bool enabled) { return gpuValidationEnabled_.Set(enabled); }

        // ========================================================================
        // Convenience Methods
        // ========================================================================

        bool IsHighResolution() const noexcept {
            const auto& res = *resolution_;
            return res.width >= 2560 || res.height >= 1440;
        }

        bool Is4K() const noexcept {
            const auto& res = *resolution_;
            return res.width >= 3840 && res.height >= 2160;
        }

        float GetAspectRatio() const noexcept {
            const auto& res = *resolution_;
            return static_cast<float>(res.width) / static_cast<float>(res.height);
        }

        uint64_t GetTotalPixels() const noexcept {
            const auto& res = *resolution_;
            return static_cast<uint64_t>(res.width) * static_cast<uint64_t>(res.height);
        }

        std::string GetDisplayModeString() const {
            const auto& res = *resolution_;
            return std::format("{}x{} {}",
                res.width, res.height,
                *fullscreen_ ? "Fullscreen" : "Windowed");
        }

        std::string GetPerformanceString() const {
            return std::format("{} | VSync: {} | MSAA: {} | Max FPS: {}",
                RenderingAPIToString(*renderingAPI_),
                *vsync_ ? "On" : "Off",
                MSAASamplesToString(*msaaSamples_),
                *maxFPS_ == 0 ? "Unlimited" : std::to_string(*maxFPS_));
        }

    private:
        // ========================================================================
        // Private Implementation
        // ========================================================================

        void InitializeDefaults() {
            // Set sensible defaults for all configuration values
            renderingAPI_ = ConfigValue<RenderingAPI>(RenderingAPI::D3D12);
            resolution_ = ConfigValue<Resolution>(Resolution{ 1920, 1080 });
            fullscreen_ = ConfigValue<bool>(false);
            vsync_ = ConfigValue<bool>(true);
            msaaSamples_ = ConfigValue<MSAASamples>(MSAASamples::MSAA4x);
            maxFPS_ = ConfigValue<uint32_t>(144, 0, 300); // 0 = unlimited, max 300
            hdrEnabled_ = ConfigValue<bool>(false);
            adapterPreference_ = ConfigValue<uint32_t>(0, 0, 15); // GPU adapter index
            debugLayerEnabled_ = ConfigValue<bool>(false);
            gpuValidationEnabled_ = ConfigValue<bool>(false);
        }

        void SetupValidation() {
            // Custom validation for resolution
            resolution_.SetValidator([](const Resolution& res) -> bool {
                // Must be reasonable resolution
                if (res.width < 320 || res.height < 240) return false;
                if (res.width > 7680 || res.height > 4320) return false;

                // Must be divisible by 2 for GPU compatibility
                if (res.width % 2 != 0 || res.height % 2 != 0) return false;

                return true;
                });

            // Custom validation for max FPS
            maxFPS_.SetValidator([](uint32_t fps) -> bool {
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
        ConfigValue<uint32_t> maxFPS_;
        ConfigValue<bool> hdrEnabled_;
        ConfigValue<uint32_t> adapterPreference_;
        ConfigValue<bool> debugLayerEnabled_;
        ConfigValue<bool> gpuValidationEnabled_;
    };

    // ============================================================================
    // Auto-Registration
    // ============================================================================

    AKH_REGISTER_CONFIG_SECTION(RenderingConfig);

    // ============================================================================
    // Convenience Access Function
    // ============================================================================

    inline std::shared_ptr<RenderingConfig> GetRenderingConfig() {
        return GetConfig<RenderingConfig>();
    }

} // namespace Akhanda::Configuration