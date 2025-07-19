// D3D12SwapChain.hpp
// Akhanda Game Engine - D3D12 SwapChain Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

import Akhanda.Engine.RHI;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Platform.RendererIntegration;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Core.Logging;
import std;

using Microsoft::WRL::ComPtr;

namespace Akhanda::RHI::D3D12 {

    // Forward declarations
    class D3D12Device;
    class D3D12Texture;
    class D3D12CommandQueue;

    // ========================================================================
    // D3D12 SwapChain Implementation
    // ========================================================================

    class D3D12SwapChain : public ISwapChain {
    private:
        // Core swap chain objects
        ComPtr<IDXGISwapChain4> swapChain_;
        ComPtr<IDXGIFactory7> dxgiFactory_;

        // Device references
        D3D12Device* device_ = nullptr;
        D3D12CommandQueue* commandQueue_ = nullptr;

        // Back buffers
        std::vector<std::unique_ptr<D3D12Texture>> backBuffers_;
        std::vector<TextureHandle> backBufferHandles_;
        uint32_t currentBackBufferIndex_ = 0;
        uint32_t backBufferCount_ = 3; // Triple buffering by default

        // Swap chain properties
        uint32_t width_ = 0;
        uint32_t height_ = 0;
        Format format_ = Format::B8G8R8A8_UNorm;
        DXGI_FORMAT dxgiFormat_ = DXGI_FORMAT_B8G8R8A8_UNORM;
        uint32_t sampleCount_ = 1;
        uint32_t sampleQuality_ = 0;

        // Display properties
        bool isFullscreen_ = false;
        bool vsyncEnabled_ = true;
        bool tearingSupported_ = false;
        bool allowModeSwitch_ = true;
        uint32_t refreshRate_ = 60;

        // HDR support
        bool hdrEnabled_ = false;
        DXGI_COLOR_SPACE_TYPE colorSpace_ = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
        float maxLuminance_ = 1000.0f;
        float minLuminance_ = 0.1f;

        // Present synchronization
        std::atomic<uint64_t> presentCount_{ 0 };
        std::atomic<uint64_t> frameLatency_{ 0 };
        uint32_t maxFrameLatency_ = 3;
        HANDLE frameLatencyWaitableObject_ = nullptr;

        // Configuration
        Configuration::RenderingConfig config_;
        Platform::RendererSurfaceInfo surfaceInfo_;

        // Debug information
        std::string debugName_;

        // State tracking
        std::atomic<bool> isInitialized_{ false };
        std::atomic<bool> isOccluded_{ false };
        std::mutex swapChainMutex_;

        // Logging
        Logging::LogChannel& logChannel_;

    public:
        D3D12SwapChain();
        virtual ~D3D12SwapChain();

        // ISwapChain implementation
        bool Initialize(const Platform::SurfaceInfo& surfaceInfo,
            const Configuration::RenderingConfig& config) override;
        void Shutdown() override;
        bool Resize(uint32_t width, uint32_t height) override;
        void Present(bool vsync = true) override;

        uint32_t GetCurrentBackBufferIndex() const override { return currentBackBufferIndex_; }
        uint32_t GetBackBufferCount() const override { return backBufferCount_; }
        TextureHandle GetBackBuffer(uint32_t index) const override;
        TextureHandle GetCurrentBackBuffer() const override;

        uint32_t GetWidth() const override { return width_; }
        uint32_t GetHeight() const override { return height_; }
        Format GetFormat() const override { return format_; }
        bool IsFullscreen() const override { return isFullscreen_; }
        bool IsVSyncEnabled() const override { return vsyncEnabled_; }

        void SetDebugName(const char* name) override;
        const char* GetDebugName() const override { return debugName_.c_str(); }

        // D3D12-specific methods
        bool InitializeWithDevice(D3D12Device* device, D3D12CommandQueue* commandQueue,
            const Platform::RendererSurfaceInfo& surfaceInfo,
            const Configuration::RenderingConfig& config);

        IDXGISwapChain4* GetDXGISwapChain() const { return swapChain_.Get(); }
        DXGI_FORMAT GetDXGIFormat() const { return dxgiFormat_; }

        // Advanced features
        bool SetFullscreen(bool fullscreen);
        bool SetVSync(bool vsync);
        bool SetHDR(bool enabled);
        bool SetColorSpace(DXGI_COLOR_SPACE_TYPE colorSpace);
        bool SetMaxFrameLatency(uint32_t maxLatency);

        // Display mode management
        bool SetDisplayMode(uint32_t width, uint32_t height, uint32_t refreshRate);
        std::vector<Platform::DisplayMode> GetSupportedDisplayModes() const;
        Platform::DisplayMode GetCurrentDisplayMode() const;

        // HDR metadata
        void SetHDRMetadata(float maxLuminance, float minLuminance, float maxContentLightLevel, float maxFrameAverageLightLevel);
        void GetHDRCapabilities(float& maxLuminance, float& minLuminance) const;

        // Present statistics
        uint64_t GetPresentCount() const { return presentCount_.load(); }
        uint64_t GetFrameLatency() const { return frameLatency_.load(); }
        bool IsOccluded() const { return isOccluded_.load(); }

        // Frame timing
        void WaitForVBlank();
        HANDLE GetFrameLatencyWaitableObject() const { return frameLatencyWaitableObject_; }

        // Debugging
        void CheckForOcclusion();
        void LogSwapChainInfo() const;

    private:
        // Initialization helpers
        bool CreateSwapChain();
        bool CreateBackBuffers();
        void SetupSwapChainDesc(DXGI_SWAP_CHAIN_DESC1& desc) const;
        void SetupFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC& desc) const;

        // Back buffer management
        bool CreateBackBuffer(uint32_t index);
        void DestroyBackBuffers();
        void UpdateCurrentBackBufferIndex();

        // Format and color space
        bool SelectOptimalFormat();
        bool SelectOptimalColorSpace();
        DXGI_FORMAT GetBestSupportedFormat() const;
        DXGI_COLOR_SPACE_TYPE GetBestSupportedColorSpace() const;

        // Display capabilities
        bool QueryDisplayCapabilities();
        bool CheckTearingSupport();
        bool CheckHDRSupport();
        std::vector<DXGI_MODE_DESC1> EnumerateDisplayModes() const;

        // HDR helpers
        bool EnableHDR();
        void DisableHDR();
        void UpdateHDRMetadata();

        // Present helpers
        UINT GetPresentFlags(bool vsync) const;
        UINT GetSyncInterval(bool vsync) const;
        void HandlePresentResult(HRESULT hr);

        // Validation
        bool ValidateSurfaceInfo(const Platform::RendererSurfaceInfo& surfaceInfo) const;
        bool ValidateResizeParameters(uint32_t width, uint32_t height) const;
        bool ValidateDisplayMode(uint32_t width, uint32_t height, uint32_t refreshRate) const;

        // Debug helpers
        void LogDisplayModes() const;
        void LogColorSpaces() const;
        void LogPresentStatistics() const;

        // Cleanup helpers
        void CleanupSwapChain();
        void CleanupBackBuffers();
        void CleanupFrameLatencyObject();
    };

    // ========================================================================
    // SwapChain Utilities
    // ========================================================================

    namespace SwapChainUtils {

        // Format conversion
        DXGI_FORMAT ConvertFormat(Format format);
        Format ConvertDXGIFormat(DXGI_FORMAT format);

        // Color space utilities
        DXGI_COLOR_SPACE_TYPE GetColorSpaceForFormat(DXGI_FORMAT format, bool hdrEnabled);
        bool IsHDRColorSpace(DXGI_COLOR_SPACE_TYPE colorSpace);
        bool IsValidColorSpace(DXGI_FORMAT format, DXGI_COLOR_SPACE_TYPE colorSpace);

        // Display mode utilities
        Platform::DisplayMode ConvertDisplayMode(const DXGI_MODE_DESC1& dxgiMode);
        DXGI_MODE_DESC1 ConvertDisplayMode(const Platform::DisplayMode& mode);
        bool IsValidDisplayMode(const Platform::DisplayMode& mode);

        // SwapChain creation helpers
        DXGI_SWAP_CHAIN_DESC1 CreateSwapChainDesc(uint32_t width, uint32_t height,
            DXGI_FORMAT format, uint32_t bufferCount,
            bool allowTearing = false);
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC CreateFullscreenDesc(uint32_t refreshRateNumerator = 0,
            uint32_t refreshRateDenominator = 0,
            bool windowed = true);

        // Present flags
        UINT GetPresentFlags(bool vsync, bool allowTearing, bool testMode = false);
        UINT GetSwapChainFlags(bool allowModeSwitch, bool allowTearing);

        // Validation
        bool IsSupportedFormat(DXGI_FORMAT format);
        bool IsSupportedBufferCount(uint32_t bufferCount);
        bool IsSupportedSampleCount(uint32_t sampleCount);

        // Debug utilities
        const char* FormatToString(DXGI_FORMAT format);
        const char* ColorSpaceToString(DXGI_COLOR_SPACE_TYPE colorSpace);
        const char* PresentResultToString(HRESULT hr);

        // HDR utilities
        struct HDRMetadata {
            float maxOutputNits = 1000.0f;
            float minOutputNits = 0.1f;
            float maxContentLightLevel = 1000.0f;
            float maxFrameAverageLightLevel = 200.0f;

            // Chromaticity coordinates (Rec. 2020)
            float redPrimary[2] = { 0.708f, 0.292f };
            float greenPrimary[2] = { 0.170f, 0.797f };
            float bluePrimary[2] = { 0.131f, 0.046f };
            float whitePoint[2] = { 0.3127f, 0.3290f };
        };

        DXGI_HDR_METADATA_HDR10 CreateHDR10Metadata(const HDRMetadata& metadata);
        bool SetHDRMetadata(IDXGISwapChain4* swapChain, const HDRMetadata& metadata);

    } // namespace SwapChainUtils

    // ========================================================================
    // SwapChain Manager
    // ========================================================================

    class D3D12SwapChainManager {
    private:
        D3D12Device* device_ = nullptr;

        // Multiple swap chains support (for multi-window scenarios)
        std::vector<std::unique_ptr<D3D12SwapChain>> swapChains_;
        std::unordered_map<HWND, D3D12SwapChain*> windowToSwapChain_;

        // Primary swap chain
        D3D12SwapChain* primarySwapChain_ = nullptr;

        // Display monitoring
        std::vector<Platform::AdapterInfo> adapters_;
        std::vector<HMONITOR> monitors_;

        // Statistics
        struct SwapChainStats {
            uint64_t totalPresents = 0;
            uint64_t droppedFrames = 0;
            float averageFrameTime = 0.0f;
            uint32_t occlusionCount = 0;
        };
        SwapChainStats stats_;

        std::mutex managerMutex_;
        Logging::LogChannel& logChannel_;

    public:
        D3D12SwapChainManager();
        ~D3D12SwapChainManager();

        bool Initialize(D3D12Device* device);
        void Shutdown();

        // SwapChain management
        D3D12SwapChain* CreateSwapChain(const Platform::RendererSurfaceInfo& surfaceInfo,
            const Configuration::RenderingConfig& config);
        void DestroySwapChain(D3D12SwapChain* swapChain);

        // Primary swap chain
        D3D12SwapChain* GetPrimarySwapChain() const { return primarySwapChain_; }
        void SetPrimarySwapChain(D3D12SwapChain* swapChain) { primarySwapChain_ = swapChain; }

        // Window association
        D3D12SwapChain* GetSwapChainForWindow(HWND window) const;
        bool AssociateSwapChainWithWindow(D3D12SwapChain* swapChain, HWND window);

        // Display monitoring
        void RefreshDisplayInfo();
        const std::vector<Platform::AdapterInfo>& GetAdapters() const { return adapters_; }

        // Statistics
        const SwapChainStats& GetStats() const { return stats_; }
        void UpdateStats();
        void ResetStats();

        // Global settings
        void SetGlobalVSync(bool enabled);
        void SetGlobalHDR(bool enabled);

    private:
        // Helper methods
        void EnumerateAdapters();
        void EnumerateMonitors();
        void UpdateStatistics();

        // Cleanup
        void CleanupSwapChains();
    };

} // namespace Akhanda::RHI::D3D12
