// Renderer.Integration.ixx
// Akhanda Game Engine - Renderer Integration and Factory Module
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <memory>
#include <string>
#include <vector>
#include <format>

export module Akhanda.Engine.Renderer.Integration;

import Akhanda.Engine.Renderer;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Platform.RendererIntegration;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Core.Logging;

export namespace Akhanda::Renderer {

    // ========================================================================
    // Factory Functions for RHI Implementation
    // ========================================================================

    namespace RHI {

        

        // Get supported RHI APIs on current platform
        std::vector<Configuration::RenderingAPI> GetSupportedAPIs() {
            std::vector<Configuration::RenderingAPI> supportedAPIs;

            // Check D3D12 support
            auto d3d12Factory = Akhanda::RHI::CreateD3D12Factory();
            if (d3d12Factory && d3d12Factory->IsSupported()) {
                supportedAPIs.push_back(Configuration::RenderingAPI::D3D12);
            }

            // Future: Check Vulkan support
            // if (IsVulkanSupported()) {
            //     supportedAPIs.push_back(Configuration::RenderingAPI::Vulkan);
            // }

            return supportedAPIs;
        }

        // Get best supported API for current hardware
        Configuration::RenderingAPI GetBestSupportedAPI() {
            auto supportedAPIs = GetSupportedAPIs();

            if (supportedAPIs.empty()) {
                auto& logChannel = Logging::LogManager::Instance().GetChannel("RHI");
                logChannel.Log(Logging::LogLevel::Error, "No supported rendering APIs found!");
                return Configuration::RenderingAPI::D3D12; // Fallback
            }

            // Prefer D3D12 on Windows
            for (auto api : supportedAPIs) {
                if (api == Configuration::RenderingAPI::D3D12) {
                    return api;
                }
            }

            // Return first available
            return supportedAPIs[0];
        }

    } // namespace RHI

    // ========================================================================
    // Standard Renderer Factory Implementation
    // ========================================================================

    std::unique_ptr<IRenderer> CreateRenderer() {
        return CreateStandardRenderer();
    }

    std::unique_ptr<StandardRenderer> CreateStandardRenderer() {
        try {
            return std::make_unique<StandardRenderer>();
        }
        catch (const std::exception& e) {
            auto& logChannel = Logging::LogManager::Instance().GetChannel("Renderer");
            logChannel.LogFormat(Logging::LogLevel::Error, "Failed to create standard renderer: {}", e.what());
            return nullptr;
        }
    }

    // ========================================================================
    // Renderer Validation and Diagnostics
    // ========================================================================

    namespace Diagnostics {

        struct SystemInfo {
            std::string operatingSystem;
            std::string cpuModel;
            uint64_t systemMemory = 0;
            std::vector<Platform::AdapterInfo> graphicsAdapters;
            std::vector<Configuration::RenderingAPI> supportedAPIs;
            Configuration::RenderingAPI recommendedAPI = Configuration::RenderingAPI::D3D12;
        };

        // Get comprehensive system information
        SystemInfo GetSystemInfo() {
            SystemInfo info;

            // Get platform integration
            auto platformIntegration = Platform::CreateRendererPlatformIntegration();
            if (platformIntegration) {
                info.graphicsAdapters = platformIntegration->EnumerateAdapters();
            }

            info.supportedAPIs = RHI::GetSupportedAPIs();
            info.recommendedAPI = RHI::GetBestSupportedAPI();

            // Platform-specific system info
#ifdef _WIN32
            info.operatingSystem = "Windows";
            // TODO: Get detailed Windows version
#endif

            return info;
        }

        // Validate configuration against system capabilities
        struct ValidationResult {
            bool isValid = false;
            std::vector<std::string> warnings;
            std::vector<std::string> errors;
            Configuration::RenderingConfig recommendedConfig;
        };

        ValidationResult ValidateConfiguration(const Configuration::RenderingConfig& config) {
            ValidationResult result;
            result.recommendedConfig = config;

            auto systemInfo = GetSystemInfo();

            // Check if requested API is supported
            bool apiSupported = false;
            for (auto api : systemInfo.supportedAPIs) {
                if (api == config.GetRenderingAPI()) {
                    apiSupported = true;
                    break;
                }
            }

            if (!apiSupported) {
                result.errors.push_back(
                    std::format("Requested rendering API {} is not supported",
                        Configuration::RenderingAPIToString(config.GetRenderingAPI())));
                result.recommendedConfig.SetRenderingAPI(systemInfo.recommendedAPI);
            }

            // Validate resolution
            const auto& resolution = config.GetResolution();
            if (resolution.width < 320 || resolution.height < 240) {
                result.errors.push_back("Resolution too small (minimum 320x240)");
                result.recommendedConfig.SetResolution(Configuration::Resolution{ 1280, 720 });
            }

            if (resolution.width > 7680 || resolution.height > 4320) {
                result.warnings.push_back("Very high resolution may impact performance");
            }

            // Validate MSAA settings
            const auto msaa = config.GetMsaaSamples();
            if (msaa != Configuration::MSAASamples::None) {
                result.warnings.push_back("MSAA may impact performance, consider using TAA instead");
            }

            // Check if HDR is requested but not supported
            if (config.GetHdrEnabled()) {
                // TODO: Check actual HDR support
                result.warnings.push_back("HDR support requires compatible display");
            }

            result.isValid = result.errors.empty();
            return result;
        }

        // Get performance recommendations
        struct PerformanceRecommendations {
            Configuration::RenderingConfig optimizedConfig;
            std::vector<std::string> recommendations;
        };

        PerformanceRecommendations GetPerformanceRecommendations(const SystemInfo& systemInfo) {
            PerformanceRecommendations recommendations;

            // Start with default high-quality settings
            recommendations.optimizedConfig.SetRenderingAPI(systemInfo.recommendedAPI);
            recommendations.optimizedConfig.SetResolution(Configuration::Resolution{ 1920, 1080 });
            recommendations.optimizedConfig.SetFullscreen(false);
            recommendations.optimizedConfig.SetVsync(true);
            recommendations.optimizedConfig.SetMsaaSamples(Configuration::MSAASamples::None);
            recommendations.optimizedConfig.SetHdrEnabled(false);

            // Analyze GPU capabilities
            if (!systemInfo.graphicsAdapters.empty()) {
                const auto& primaryAdapter = systemInfo.graphicsAdapters[0];

                // Check VRAM
                if (primaryAdapter.dedicatedVideoMemory < 2ULL * 1024 * 1024 * 1024) { // < 2GB
                    recommendations.recommendations.push_back("Consider lower resolution for better performance (GPU has limited VRAM)");
                    recommendations.optimizedConfig.SetResolution(Configuration::Resolution{ 1280, 720 });
                }

                // Check if integrated GPU
                if (primaryAdapter.isIntegrated) {
                    recommendations.recommendations.push_back("Integrated GPU detected - optimizing for performance over quality");
                    recommendations.optimizedConfig.SetResolution(Configuration::Resolution{ 1280, 720 });
                    recommendations.optimizedConfig.SetMsaaSamples(Configuration::MSAASamples::None);
                }

                // Feature support recommendations
                if (primaryAdapter.supportsDXR) {
                    recommendations.recommendations.push_back("Hardware ray tracing supported - consider enabling for enhanced lighting");
                }

                if (primaryAdapter.supportsVariableRateShading) {
                    recommendations.recommendations.push_back("Variable Rate Shading supported - can improve performance in VR or high resolution scenarios");
                }
            }

            return recommendations;
        }

        // Log system diagnostics
        void LogSystemDiagnostics() {
            auto& logChannel = Logging::LogManager::Instance().GetChannel("Diagnostics");

            logChannel.Log(Logging::LogLevel::Info, "=== Akhanda Engine System Diagnostics ===");

            auto systemInfo = GetSystemInfo();

            logChannel.LogFormat(Logging::LogLevel::Info, "Operating System: {}", systemInfo.operatingSystem);
            logChannel.LogFormat(Logging::LogLevel::Info, "Supported Rendering APIs: {}", systemInfo.supportedAPIs.size());

            for (auto api : systemInfo.supportedAPIs) {
                logChannel.LogFormat(Logging::LogLevel::Info, "  - {}", Configuration::RenderingAPIToString(api));
            }

            logChannel.LogFormat(Logging::LogLevel::Info, "Recommended API: {}",
                Configuration::RenderingAPIToString(systemInfo.recommendedAPI));

            logChannel.LogFormat(Logging::LogLevel::Info, "Graphics Adapters: {}", systemInfo.graphicsAdapters.size());
            for (size_t i = 0; i < systemInfo.graphicsAdapters.size(); ++i) {
                const auto& adapter = systemInfo.graphicsAdapters[i];
                logChannel.LogFormat(Logging::LogLevel::Info, L"  [{}] {}", i, adapter.description);
                logChannel.LogFormat(Logging::LogLevel::Info, "      VRAM: {} MB", adapter.dedicatedVideoMemory / (1024 * 1024));
                logChannel.LogFormat(Logging::LogLevel::Info, "      Type: {}", adapter.isIntegrated ? "Integrated" : "Discrete");

                if (adapter.supportsDXR) {
                    logChannel.LogFormat(Logging::LogLevel::Info, "      Features: DXR Ray Tracing");
                }
                if (adapter.supportsVariableRateShading) {
                    logChannel.LogFormat(Logging::LogLevel::Info, "      Features: Variable Rate Shading");
                }
                if (adapter.supportsMeshShaders) {
                    logChannel.LogFormat(Logging::LogLevel::Info, "      Features: Mesh Shaders");
                }
            }

            logChannel.Log(Logging::LogLevel::Info, "=== End System Diagnostics ===");
        }

    } // namespace Diagnostics

    // ========================================================================
    // Engine Integration Helpers
    // ========================================================================

    namespace Integration {

        // Initialize renderer subsystem with automatic configuration
        struct RendererInitResult {
            std::unique_ptr<IRenderer> renderer;
            Configuration::RenderingConfig finalConfig;
            bool success = false;
            std::string errorMessage;
        };

        RendererInitResult InitializeRendererSubsystem(
            const Configuration::RenderingConfig& requestedConfig,
            const Platform::SurfaceInfo& surfaceInfo,
            bool enableValidation = true) {

            RendererInitResult result;
            auto& logChannel = Logging::LogManager::Instance().GetChannel("RendererInit");

            logChannel.Log(Logging::LogLevel::Info, "Initializing renderer subsystem...");

            try {
                // Validate configuration
                if (enableValidation) {
                    auto validation = Diagnostics::ValidateConfiguration(requestedConfig);

                    if (!validation.isValid) {
                        result.errorMessage = "Configuration validation failed: " + validation.errors[0];
                        return result;
                    }

                    result.finalConfig = validation.recommendedConfig;

                    // Log warnings
                    for (const auto& warning : validation.warnings) {
                        logChannel.Log(Logging::LogLevel::Warning, warning);
                    }
                }
                else {
                    result.finalConfig = requestedConfig;
                }

                // Create renderer
                result.renderer = CreateStandardRenderer();
                if (!result.renderer) {
                    result.errorMessage = "Failed to create renderer instance";
                    return result;
                }

                // Initialize renderer
                if (!result.renderer->Initialize(result.finalConfig, surfaceInfo)) {
                    result.errorMessage = "Failed to initialize renderer";
                    result.renderer.reset();
                    return result;
                }

                logChannel.Log(Logging::LogLevel::Info, "Renderer subsystem initialized successfully");
                result.success = true;

            }
            catch (const std::exception& e) {
                result.errorMessage = std::format("Exception during renderer initialization: {}", e.what());
                logChannel.Log(Logging::LogLevel::Error, result.errorMessage);
            }

            return result;
        }

        // Quick initialization for common scenarios
        std::unique_ptr<IRenderer> QuickInitialize(Platform::IPlatformWindow* window, bool enableDebug = true) {
            if (!window) {
                return {};
            }

            // Create default configuration
            Configuration::RenderingConfig config;
            config.SetRenderingAPI(RHI::GetBestSupportedAPI());

            auto surfaceInfo = window->GetSurface()->GetSurfaceInfo();
            config.SetResolution(Configuration::Resolution{ surfaceInfo.width, surfaceInfo.height });
            config.SetVsync(true);
            config.SetDebugLayerEnabled(enableDebug);
            config.SetGpuValidationEnabled(enableDebug);

            auto result = InitializeRendererSubsystem(config, surfaceInfo, true);

            return std::move(result.renderer);
        }

    } // namespace Integration

    // ========================================================================
    // Module Exports Summary
    // ========================================================================

    /*
    This module provides the main integration points for the Akhanda rendering system:

    1. **RHI Factory Functions**:
       - CreateD3D12Factory() - Create D3D12 implementation
       - CreateRHIFactory(api) - Create factory for specific API
       - GetSupportedAPIs() - Query available APIs
       - GetBestSupportedAPI() - Get recommended API

    2. **Renderer Factory Functions**:
       - CreateRenderer() - Create default renderer
       - CreateStandardRenderer() - Create standard renderer implementation

    3. **Diagnostics and Validation**:
       - GetSystemInfo() - Comprehensive system information
       - ValidateConfiguration() - Validate rendering configuration
       - GetPerformanceRecommendations() - Get optimized settings
       - LogSystemDiagnostics() - Log detailed system info

    4. **Integration Helpers**:
       - InitializeRendererSubsystem() - Full initialization with validation
       - QuickInitialize() - Simple initialization for prototypes

    Usage Example:
    ```cpp
    import Akhanda.Engine.Renderer.Integration;

    // Simple initialization
    auto window = Platform::CreateWindow(windowDesc);
    auto renderer = Renderer::Integration::QuickInitialize(window.get());

    // Or detailed initialization
    Configuration::RenderingConfig config;
    // ... configure settings ...

    auto result = Renderer::Integration::InitializeRendererSubsystem(
        config, window->GetSurfaceInfo());

    if (result.success) {
        auto renderer = std::move(result.renderer);
        // Use renderer...
    }
    ```
    */

} // namespace Akhanda::Renderer
