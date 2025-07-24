// Engine/Renderer/RHI/D3D12/D3D12Device.hpp
// Akhanda Game Engine - D3D12 Device Header
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

// Windows and D3D12 includes
#include <windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include <D3D12MemAlloc.h>

// STL includes
#include <memory>
#include <mutex>
#include <vector>
#include <queue>
#include <atomic>
#include <string>
#include <unordered_map>

#undef DeviceCapabilities

// Engine includes
import Akhanda.Engine.RHI;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Platform.Interfaces;
import Akhanda.Platform.RendererIntegration;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Core.Logging;
import Akhanda.Core.Memory;

namespace Akhanda::RHI::D3D12 {

    using Microsoft::WRL::ComPtr;

    // Forward declarations
    class D3D12DescriptorHeap;
    class D3D12SwapChain;
    class D3D12Pipeline;
    class D3D12CommandList;
    class D3D12Buffer;
    class D3D12Texture;

    // ========================================================================
    // D3D12 Command Queue Implementation
    // ========================================================================

    class D3D12CommandQueue {
    private:
        // Core D3D12 objects
        ComPtr<ID3D12CommandQueue> commandQueue_;
        ComPtr<ID3D12Fence> fence_;
        HANDLE fenceEvent_ = nullptr;
        std::atomic<uint64_t> nextFenceValue_;

        // Command allocator management
        std::queue<ComPtr<ID3D12CommandAllocator>> availableAllocators_;
        std::queue<std::pair<ComPtr<ID3D12CommandAllocator>, uint64_t>> inFlightAllocators_;

        // Configuration
        CommandListType type_;
        std::string debugName_;

        // Logging
        Logging::LogChannel& logChannel_;

    public:
        explicit D3D12CommandQueue(CommandListType type);
        virtual ~D3D12CommandQueue();

        // Initialization
        bool Initialize(ID3D12Device* device);
        void Shutdown();

        // Command execution
        uint64_t ExecuteCommandLists(uint32_t count, ID3D12CommandList* const* commandLists);
        void WaitForFenceValue(uint64_t fenceValue);
        void WaitForIdle();
        uint64_t Signal();

        // Command allocator management
        ComPtr<ID3D12CommandAllocator> GetCommandAllocator();
        void ReturnCommandAllocator(ComPtr<ID3D12CommandAllocator> allocator, uint64_t fenceValue);

        // Accessors
        ID3D12CommandQueue* GetD3D12CommandQueue() const { return commandQueue_.Get(); }
        ID3D12Fence* GetFence() const { return fence_.Get(); }
        uint64_t GetNextFenceValue() const { return nextFenceValue_.load(); }
        uint64_t GetCompletedFenceValue() const { return fence_ ? fence_->GetCompletedValue() : 0; }

        // Debug
        void SetDebugName(const char* name);
        const char* GetDebugName() const { return debugName_.c_str(); }

    private:
        void ProcessCompletedAllocators();
    };


    // ========================================================================
    // D3D12 Resource Manager
    // ========================================================================

    class D3D12ResourceManager {
    private:
        struct ResourceEntry {
            ComPtr<ID3D12Resource> resource;
            D3D12MA::Allocation* allocation = nullptr;
            RHI::ResourceState currentState = RHI::ResourceState::Common;
            uint32_t generation = 0;
            std::string debugName;
            bool isValid = true;
        };

        std::vector<ResourceEntry> buffers_;
        std::vector<ResourceEntry> textures_;
        std::vector<std::unique_ptr<D3D12Pipeline>> pipelines_;

        std::queue<uint32_t> freeBufferIndices_;
        std::queue<uint32_t> freeTextureIndices_;
        std::queue<uint32_t> freePipelineIndices_;

        std::mutex resourceMutex_;
        std::atomic<uint32_t> nextGeneration_{ 1 };

        ComPtr<D3D12MA::Allocator> allocator_;

    public:
        bool Initialize(ID3D12Device* device, IDXGIAdapter* adapter);
        void Shutdown();

        // Buffer management
        BufferHandle CreateBuffer(const BufferDesc& desc, ID3D12Device* device);
        void DestroyBuffer(BufferHandle handle);
        D3D12Buffer* GetBuffer(BufferHandle handle);

        // Texture management
        TextureHandle CreateTexture(const TextureDesc& desc, ID3D12Device* device);
        void DestroyTexture(TextureHandle handle);
        D3D12Texture* GetTexture(TextureHandle handle);

        // Pipeline management
        PipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc, ID3D12Device* device);
        PipelineHandle CreateComputePipeline(const ComputePipelineDesc& desc, ID3D12Device* device);
        void DestroyPipeline(PipelineHandle handle);
        D3D12Pipeline* GetPipeline(PipelineHandle handle);

        // Resource access
        ID3D12Resource* GetD3D12Resource(BufferHandle handle);
        ID3D12Resource* GetD3D12Resource(TextureHandle handle);

        // Memory statistics
        uint64_t GetUsedMemory() const;
        uint64_t GetTotalMemory() const;

    private:
        bool IsValidHandle(BufferHandle handle) const;
        bool IsValidHandle(TextureHandle handle) const;
        bool IsValidHandle(PipelineHandle handle) const;
    };

    // ========================================================================
    // D3D12 Device Implementation
    // ========================================================================

    class D3D12Device : public IRenderDevice {
    private:
        // Core D3D12 objects
        ComPtr<IDXGIFactory7> dxgiFactory_;
        ComPtr<IDXGIAdapter4> adapter_;
        ComPtr<ID3D12Device12> device_;

        // Debug layer objects
        ComPtr<ID3D12Debug> d3d12Debug_;
        ComPtr<ID3D12Debug1> d3d12Debug1_;
        ComPtr<ID3D12Debug5> d3d12Debug5_;
        ComPtr<ID3D12DebugDevice> debugDevice_;
        ComPtr<ID3D12InfoQueue> infoQueue_;

        // Command queues
        std::unique_ptr<D3D12CommandQueue> graphicsQueue_;
        std::unique_ptr<D3D12CommandQueue> computeQueue_;
        std::unique_ptr<D3D12CommandQueue> copyQueue_;

        // Descriptor heaps
        std::unique_ptr<D3D12DescriptorHeap> rtvHeap_;
        std::unique_ptr<D3D12DescriptorHeap> dsvHeap_;
        std::unique_ptr<D3D12DescriptorHeap> srvHeap_;
        std::unique_ptr<D3D12DescriptorHeap> samplerHeap_;

        // Resource management
        std::unique_ptr<D3D12ResourceManager> resourceManager_;
        std::unique_ptr<D3D12SwapChain> swapChain_;

        // Configuration and state
        Configuration::RenderingConfig config_;
        Platform::RendererSurfaceInfo surfaceInfo_;
        std::atomic<bool> isInitialized_;
        std::atomic<uint64_t> frameNumber_;

        // Capabilities and statistics
        DeviceCapabilities deviceCapabilities_;
        RenderStats renderStats_;

        // Debug state
        bool debugLayerEnabled_ = false;
        bool gpuValidationEnabled_ = false;
        std::vector<DebugMessage> debugMessages_;
        std::string debugName_;

        // Logging
        Logging::LogChannel& logChannel_;

    public:
        D3D12Device();
        virtual ~D3D12Device();

        // IRenderDevice implementation
        bool Initialize(const Configuration::RenderingConfig& config, const Platform::SurfaceInfo& surfaceInfo) override;
        void Shutdown() override;

        std::unique_ptr<IDescriptorHeap> CreateDescriptorHeap(uint32_t descriptorCount, bool shaderVisible) override;

        void WaitForIdle() override;
        void BeginFrame() override;
        void EndFrame() override;
        void Present() override;

        // Resource creation
        BufferHandle CreateBuffer(const BufferDesc& desc) override;
        TextureHandle CreateTexture(const TextureDesc& desc) override;
        PipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
        PipelineHandle CreateComputePipeline(const ComputePipelineDesc& desc) override;

        // Resource destruction
        void DestroyBuffer(BufferHandle handle) override;
        void DestroyTexture(TextureHandle handle) override;
        void DestroyPipeline(PipelineHandle handle) override;

        // Resource access
        IRenderBuffer* GetBuffer(BufferHandle handle) override;
        IRenderTexture* GetTexture(TextureHandle handle) override;
        IRenderPipeline* GetPipeline(PipelineHandle handle) override;

        // Command recording and execution
        std::unique_ptr<ICommandList> CreateCommandList(CommandListType type) override;
        uint64_t ExecuteCommandLists(CommandListType queueType, uint32_t count, ICommandList* const* commandLists) override;
        void ExecuteCommandLists([[maybe_unused]] uint32_t count, [[maybe_unused]] ICommandList* const* commandLists) override {}
        void WaitForFence(uint64_t fenceValue, CommandListType queueType);

        // Device properties
        const DeviceCapabilities& GetCapabilities() const override;
        const RenderStats& GetStats() const override;
        std::vector<DebugMessage> GetDebugMessages() override;

        // Debug support
        void SetDebugName(const char* name) override;
        const char* GetDebugName() const override;
        // Debug features
        void BeginDebugEvent(const char* name) override;
        void EndDebugEvent() override;
        void InsertDebugMarker(const char* name) override;

        // Memory management
        uint64_t GetUsedVideoMemory() const override;
        uint64_t GetAvailableVideoMemory() const override;
        void FlushMemory() override;

        // D3D12-specific accessors
        ID3D12Device* GetD3D12Device() const { return device_.Get(); }
        IDXGIFactory7* GetDXGIFactory() const { return dxgiFactory_.Get(); }
        IDXGIAdapter4* GetAdapter() const { return adapter_.Get(); }

        D3D12CommandQueue* GetGraphicsQueue() const { return graphicsQueue_.get(); }
        D3D12CommandQueue* GetComputeQueue() const { return computeQueue_.get(); }
        D3D12CommandQueue* GetCopyQueue() const { return copyQueue_.get(); }

        D3D12DescriptorHeap* GetRTVHeap() const { return rtvHeap_.get(); }
        D3D12DescriptorHeap* GetDSVHeap() const { return dsvHeap_.get(); }
        D3D12DescriptorHeap* GetSRVHeap() const { return srvHeap_.get(); }
        D3D12DescriptorHeap* GetSamplerHeap() const { return samplerHeap_.get(); }

        D3D12ResourceManager* GetResourceManager() const { return resourceManager_.get(); }
        ISwapChain* GetSwapChain() override;

        uint64_t GetFrameNumber() const { return frameNumber_.load(); }
        bool IsInitialized() const { return isInitialized_.load(); }

    private:
        // Initialization helpers
        bool InitializeDebugLayer();
        bool InitializeDevice();
        bool InitializeCommandQueues();
        bool InitializeDescriptorHeaps();
        bool InitializeResourceManager();
        bool InitializeSwapChain();

        // Capability detection
        void DetectCapabilities();
        void LogDeviceInfo();

        // Debug message processing
        void ProcessDebugMessages();
        DebugMessage::Severity ConvertD3D12Severity(D3D12_MESSAGE_SEVERITY severity);

        // Validation helpers
        bool ValidateBufferDesc(const BufferDesc& desc) const;
        bool ValidateTextureDesc(const TextureDesc& desc) const;
        bool ValidatePipelineDesc(const GraphicsPipelineDesc& desc) const;
        bool ValidatePipelineDesc(const ComputePipelineDesc& desc) const;

        // State management
        void UpdateRenderStats();
        void ResetFrameStats();

        // Cleanup helpers
        void CleanupResources();
        void CleanupDebugLayer();
    };

    // ========================================================================
    // D3D12 Factory Implementation
    // ========================================================================

    class D3D12Factory : public IRHIFactory {
    private:
        ComPtr<IDXGIFactory7> dxgiFactory_;
        std::unique_ptr<Platform::IRendererPlatformIntegration> platformIntegration_;
        std::vector<Platform::AdapterInfo> adapters_;
        std::string debugName_;
        Logging::LogChannel& logChannel_;

    public:
        D3D12Factory();
        virtual ~D3D12Factory();

        bool Initialize() override;
        void Shutdown() override;

        // IRHIFactory implementation
        std::unique_ptr<IRenderDevice> CreateDevice(
            const Configuration::RenderingConfig& config,
            const Platform::SurfaceInfo& surfaceInfo) override;

        bool IsSupported() const override;
        std::vector<std::string> GetSupportedDevices() const override;
        DeviceCapabilities GetDeviceCapabilities(uint32_t deviceIndex = 0) const override;

        void SetDebugName(const char* name) override;
        const char* GetDebugName() const override;

        // D3D12-specific methods
        IDXGIFactory7* GetDXGIFactory() const { return dxgiFactory_.Get(); }
        const std::vector<Platform::AdapterInfo>& GetAdapters() const { return adapters_; }

    private:
        void EnumerateAdapters();
        DeviceCapabilities GetAdapterCapabilities(const Platform::AdapterInfo& adapter) const;
    };

    // ========================================================================
    // Utility Functions
    // ========================================================================

    namespace Utils {
        // Format conversion utilities
        DXGI_FORMAT ConvertFormat(Format format);
        Format ConvertFormat(DXGI_FORMAT format);

        // Resource state conversion
        D3D12_RESOURCE_STATES ConvertResourceState(ResourceState state);
        ResourceState ConvertResourceState(D3D12_RESOURCE_STATES state);

        // Validation helpers
        bool IsDepthFormat(Format format);
        bool IsCompressedFormat(Format format);
        bool IsUAVCompatibleFormat(Format format);

        // Memory helpers
        uint32_t GetFormatBlockSize(Format format);
        uint32_t CalculateSubresourceIndex(uint32_t mipLevel, uint32_t arraySlice,
            uint32_t planeSlice, uint32_t mipLevels,
            uint32_t arraySize);

        // Debug helpers
        std::string GetFeatureLevelString(D3D_FEATURE_LEVEL featureLevel);
        std::string GetResourceStateString(D3D12_RESOURCE_STATES state);

        // Error handling
        std::string GetHRESULTString(HRESULT hr);
        void ThrowIfFailed(HRESULT hr, const char* message = nullptr);
    }

    // ========================================================================
    // Error Handling Macros
    // ========================================================================

#define CHECK_HR_RETURN(hr, msg, channel, retval) \
        if (FAILED(hr)) { \
            _com_error err(hr); \
            channel.LogFormat(Logging::LogLevel::Error, "{}: {} (HRESULT: 0x{:08X})", \
                msg, err.ErrorMessage(), static_cast<uint32_t>(hr)); \
            return retval; \
        }

#define CHECK_HR_RETURN_BOOL(hr, msg, channel) \
        CHECK_HR_RETURN(hr, msg, channel, false)

#define CHECK_HR_RETURN_VOID(hr, msg, channel) \
        if (FAILED(hr)) { \
            _com_error err(hr); \
            channel.LogFormat(Logging::LogLevel::Error, "{}: {} (HRESULT: 0x{:08X})", \
                msg, err.ErrorMessage(), static_cast<uint32_t>(hr)); \
            return; \
        }

#define VERIFY_D3D12_DEVICE(device) \
        if (!device) { \
            logChannel_.Log(Logging::LogLevel::Error, "D3D12 device is null"); \
            return false; \
        }

#define VERIFY_INITIALIZED() \
        if (!isInitialized_.load()) { \
            logChannel_.Log(Logging::LogLevel::Error, "Device not initialized"); \
            return false; \
        }

} // namespace Akhanda::RHI::D3D12