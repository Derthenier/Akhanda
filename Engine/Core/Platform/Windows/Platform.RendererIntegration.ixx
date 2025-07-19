// Platform.RendererIntegration.ixx
// Akhanda Game Engine - Platform-Renderer Integration Module
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <Windows.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl/client.h>

export module Akhanda.Platform.RendererIntegration;

using Microsoft::WRL::ComPtr;

import Akhanda.Platform.Interfaces;
import Akhanda.Platform.Windows;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Core.Logging;

export namespace Akhanda::Platform {

    // ========================================================================
    // Display Mode Structure
    // ========================================================================

    struct DisplayMode {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t refreshRateNumerator = 60;
        uint32_t refreshRateDenominator = 1;
        uint32_t format = 0; // Platform-specific format (DXGI_FORMAT for Windows)
        uint32_t scanlineOrdering = 0;
        uint32_t scaling = 0;
        bool isInterlaced = false;

        float GetRefreshRate() const {
            return refreshRateDenominator > 0 ?
                static_cast<float>(refreshRateNumerator) / static_cast<float>(refreshRateDenominator) : 0.0f;
        }

        bool operator==(const DisplayMode& other) const {
            return width == other.width &&
                height == other.height &&
                refreshRateNumerator == other.refreshRateNumerator &&
                refreshRateDenominator == other.refreshRateDenominator;
        }

        bool operator!=(const DisplayMode& other) const {
            return !(*this == other);
        }
    };

    // ========================================================================
    // Enhanced Surface Information for Renderer Integration
    // ========================================================================

    struct RendererSurfaceInfo : public SurfaceInfo {
        // Windows-specific handles
        HWND windowHandle = nullptr;
        HINSTANCE instanceHandle = nullptr;

        // Display information
        HMONITOR monitor = nullptr;
        uint32_t refreshRate = 60;
        bool isHDRCapable = false;

        // Color space information
        uint32_t colorSpace = 0;  // DXGI_COLOR_SPACE_TYPE
        float maxLuminance = 80.0f;
        float minLuminance = 0.1f;

        // Multi-adapter information
        uint32_t preferredAdapterIndex = 0;
        bool useWarpAdapter = false;

        // Debug and validation
        bool enableDebugLayer = false;
        bool enableGPUValidation = false;
        bool enableDREDInfo = false;

        // Performance settings
        bool allowTearing = false;
        bool variableRefreshRate = false;

        // Factory method to create from window
        static RendererSurfaceInfo CreateFromWindow(IPlatformWindow* window,
            const Configuration::RenderingConfig& config);
    };

    // ========================================================================
    // Adapter Information
    // ========================================================================

    struct AdapterInfo {
        std::wstring description;
        uint32_t vendorId = 0;
        uint32_t deviceId = 0;
        uint32_t subSysId = 0;
        uint32_t revision = 0;

        size_t dedicatedVideoMemory = 0;
        size_t dedicatedSystemMemory = 0;
        size_t sharedSystemMemory = 0;

        bool isHardware = true;
        bool isIntegrated = false;
        bool supportsDXR = false;
        bool supportsVariableRateShading = false;
        bool supportsMeshShaders = false;

        // Output information
        struct OutputInfo {
            std::wstring deviceName;
            uint32_t width = 0;
            uint32_t height = 0;
            uint32_t refreshRateNumerator = 0;
            uint32_t refreshRateDenominator = 0;
            bool isHDRSupported = false;
            float maxLuminance = 0.0f;
            float minLuminance = 0.0f;
        };

        std::vector<OutputInfo> outputs;
    };

    // ========================================================================
    // Platform-Specific RHI Integration
    // ========================================================================

    class IRendererPlatformIntegration {
    public:
        virtual ~IRendererPlatformIntegration() = default;

        // Adapter enumeration
        virtual std::vector<AdapterInfo> EnumerateAdapters() = 0;
        virtual AdapterInfo GetAdapterInfo(uint32_t adapterIndex) = 0;
        virtual uint32_t GetDefaultAdapterIndex() = 0;

        // Surface creation
        virtual RendererSurfaceInfo CreateSurface(IPlatformWindow* window,
            const Configuration::RenderingConfig& config) = 0;
        virtual void DestroySurface(const RendererSurfaceInfo& surface) = 0;

        // Display mode management
        virtual std::vector<DisplayMode> GetSupportedDisplayModes(uint32_t adapterIndex,
            uint32_t outputIndex) = 0;
        virtual DisplayMode GetCurrentDisplayMode(uint32_t adapterIndex,
            uint32_t outputIndex) = 0;
        virtual bool SetDisplayMode(uint32_t adapterIndex, uint32_t outputIndex,
            const DisplayMode& mode) = 0;

        // HDR support
        virtual bool IsHDRSupported(uint32_t adapterIndex, uint32_t outputIndex) = 0;
        virtual void EnableHDR(uint32_t adapterIndex, uint32_t outputIndex, bool enable) = 0;
        virtual void GetHDRMetadata(uint32_t adapterIndex, uint32_t outputIndex,
            float& maxLuminance, float& minLuminance) = 0;

        // Debug features
        virtual bool IsDebugLayerAvailable() = 0;
        virtual bool IsGPUValidationAvailable() = 0;
        virtual bool IsDREDAvailable() = 0;
        virtual void EnableDebugLayer(bool enable) = 0;
        virtual void EnableGPUValidation(bool enable) = 0;
        virtual void EnableDRED(bool enable) = 0;

        // Performance features
        virtual bool IsTearingSupported() = 0;
        virtual bool IsVariableRefreshRateSupported(uint32_t adapterIndex,
            uint32_t outputIndex) = 0;

        // Factory access for D3D12
        virtual void* GetDXGIFactory() = 0;  // Returns IDXGIFactory7*
        virtual void* GetD3D12Debug() = 0;   // Returns ID3D12Debug*
    };

    // ========================================================================
    // Windows-Specific Implementation
    // ========================================================================

    class WindowsRendererPlatformIntegration : public IRendererPlatformIntegration {
    private:
        ComPtr<IDXGIFactory7> dxgiFactory_;
        ComPtr<ID3D12Debug> d3d12Debug_;
        ComPtr<ID3D12Debug1> d3d12Debug1_;
        ComPtr<ID3D12Debug3> d3d12Debug3_;

        std::vector<ComPtr<IDXGIAdapter4>> adapters_;
        bool debugLayerEnabled_ = false;
        bool gpuValidationEnabled_ = false;
        bool dredEnabled_ = false;

        Logging::LogChannel& logChannel_;

    public:
        WindowsRendererPlatformIntegration();
        virtual ~WindowsRendererPlatformIntegration();

        // Initialization
        bool Initialize();
        void Shutdown();

        // IRendererPlatformIntegration implementation
        std::vector<AdapterInfo> EnumerateAdapters() override;
        AdapterInfo GetAdapterInfo(uint32_t adapterIndex) override;
        uint32_t GetDefaultAdapterIndex() override;

        RendererSurfaceInfo CreateSurface(IPlatformWindow* window,
            const Configuration::RenderingConfig& config) override;
        void DestroySurface(const RendererSurfaceInfo& surface) override;

        std::vector<DisplayMode> GetSupportedDisplayModes(uint32_t adapterIndex,
            uint32_t outputIndex) override;
        DisplayMode GetCurrentDisplayMode(uint32_t adapterIndex,
            uint32_t outputIndex) override;
        bool SetDisplayMode(uint32_t adapterIndex, uint32_t outputIndex,
            const DisplayMode& mode) override;

        bool IsHDRSupported(uint32_t adapterIndex, uint32_t outputIndex) override;
        void EnableHDR(uint32_t adapterIndex, uint32_t outputIndex, bool enable) override;
        void GetHDRMetadata(uint32_t adapterIndex, uint32_t outputIndex,
            float& maxLuminance, float& minLuminance) override;

        bool IsDebugLayerAvailable() override;
        bool IsGPUValidationAvailable() override;
        bool IsDREDAvailable() override;
        void EnableDebugLayer(bool enable) override;
        void EnableGPUValidation(bool enable) override;
        void EnableDRED(bool enable) override;

        bool IsTearingSupported() override;
        bool IsVariableRefreshRateSupported(uint32_t adapterIndex,
            uint32_t outputIndex) override;

        void* GetDXGIFactory() override { return dxgiFactory_.Get(); }
        void* GetD3D12Debug() override { return d3d12Debug_.Get(); }

    private:
        // Helper methods
        void EnumerateAdaptersInternal();
        AdapterInfo CreateAdapterInfo(IDXGIAdapter4* adapter);
        void LogAdapterInfo(const AdapterInfo& info);
        DXGI_FORMAT GetDXGIFormat(uint32_t format);
        uint32_t GetEngineFormat(DXGI_FORMAT format);
    };

    // ========================================================================
    // Factory Functions
    // ========================================================================

    // Create platform-specific renderer integration
    std::unique_ptr<IRendererPlatformIntegration> CreateRendererPlatformIntegration();

    // ========================================================================
    // Surface Creation Utilities
    // ========================================================================

    // Create surface info from window and configuration
    RendererSurfaceInfo CreateRendererSurfaceInfo(IPlatformWindow* window,
        const Configuration::RenderingConfig& config);

    // Validate surface configuration
    bool ValidateSurfaceConfiguration(const RendererSurfaceInfo& surface,
        const Configuration::RenderingConfig& config);

    // Apply configuration to surface
    void ApplyConfigurationToSurface(RendererSurfaceInfo& surface,
        const Configuration::RenderingConfig& config);

    // ========================================================================
    // Display Mode Utilities
    // ========================================================================

    // Find best matching display mode
    DisplayMode FindBestDisplayMode(const std::vector<DisplayMode>& supportedModes,
        uint32_t preferredWidth, uint32_t preferredHeight,
        uint32_t preferredRefreshRate = 0);

    // Check if display mode is supported
    bool IsDisplayModeSupported(const std::vector<DisplayMode>& supportedModes,
        const DisplayMode& mode);

    // Convert display mode to string for logging
    std::string DisplayModeToString(const DisplayMode& mode);

    // ========================================================================
    // Debug and Validation Utilities
    // ========================================================================

    // Check if PIX is available
    bool IsPixAvailable();

    // Enable PIX capture
    void EnablePixCapture();

    // Set PIX marker
    void SetPixMarker(const char* name);

    // Begin PIX event
    void BeginPixEvent(const char* name);

    // End PIX event
    void EndPixEvent();

    // ========================================================================
    // Performance Utilities
    // ========================================================================

    // Get current GPU memory usage
    struct GPUMemoryInfo {
        uint64_t totalMemory = 0;
        uint64_t availableMemory = 0;
        uint64_t usedMemory = 0;
        uint64_t reservedMemory = 0;
        float utilizationPercentage = 0.0f;
    };

    GPUMemoryInfo GetGPUMemoryInfo(uint32_t adapterIndex = 0);

    // Get GPU utilization
    float GetGPUUtilization(uint32_t adapterIndex = 0);

    // Get GPU temperature (if available)
    float GetGPUTemperature(uint32_t adapterIndex = 0);

} // namespace Akhanda::Platform

// ========================================================================
// Implementation of inline methods
// ========================================================================

namespace Akhanda::Platform {

    inline RendererSurfaceInfo RendererSurfaceInfo::CreateFromWindow(
        IPlatformWindow* window, const Configuration::RenderingConfig& config) {

        RendererSurfaceInfo surface;
        if (!window) {
            return surface;
        }

        // Get basic surface info from window
        auto windowInfo = window->GetSurface()->GetSurfaceInfo();
        surface.nativeHandle = windowInfo.nativeHandle;
        surface.nativeDisplay = windowInfo.nativeDisplay;
        surface.width = windowInfo.width;
        surface.height = windowInfo.height;
        surface.format = windowInfo.format;
        surface.supportsPresent = windowInfo.supportsPresent;
        surface.sampleCount = windowInfo.sampleCount;
        surface.qualityLevels = windowInfo.qualityLevels;

        // Cast to Windows-specific handles
        surface.windowHandle = static_cast<HWND>(surface.nativeHandle);
        surface.instanceHandle = GetModuleHandle(nullptr);

        // Get monitor information
        surface.monitor = MonitorFromWindow(surface.windowHandle, MONITOR_DEFAULTTOPRIMARY);

        // Apply configuration
        ApplyConfigurationToSurface(surface, config);

        return surface;
    }

    inline WindowsRendererPlatformIntegration::WindowsRendererPlatformIntegration()
        : logChannel_(Logging::LogManager::Instance().GetChannel("Platform.Renderer")) {
    }

} // namespace Akhanda::Platform
