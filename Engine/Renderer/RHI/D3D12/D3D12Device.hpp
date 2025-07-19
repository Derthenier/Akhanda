// D3D12Device.hpp
// Akhanda Game Engine - D3D12 Render Device Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <D3D12MemAlloc.h>
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <unordered_map>

#undef DeviceCapabilities

import Akhanda.Engine.RHI;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Platform.Interfaces;
import Akhanda.Platform.RendererIntegration;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Core.Logging;
import Akhanda.Core.Memory;

using Microsoft::WRL::ComPtr;

namespace Akhanda::RHI::D3D12 {

    // ========================================================================
    // Forward Declarations
    // ========================================================================

    class D3D12CommandList;
    class D3D12Buffer;
    class D3D12Texture;
    class D3D12Pipeline;
    class D3D12SwapChain;
    class D3D12DescriptorHeap;
    class D3D12ResourceManager;
    class D3D12CommandQueue;

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
    // D3D12 Command Queue
    // ========================================================================

    class D3D12CommandQueue {
    private:
        ComPtr<ID3D12CommandQueue> commandQueue_;
        ComPtr<ID3D12Fence> fence_;
        std::atomic<uint64_t> fenceValue_{ 0 };
        HANDLE fenceEvent_ = nullptr;

        std::queue<ComPtr<ID3D12CommandAllocator>> availableAllocators_;
        std::queue<std::pair<ComPtr<ID3D12CommandAllocator>, uint64_t>> inFlightAllocators_;
        std::mutex allocatorMutex_;

        D3D12_COMMAND_LIST_TYPE type_;
        std::string debugName_;

    public:
        D3D12CommandQueue(D3D12_COMMAND_LIST_TYPE type);
        ~D3D12CommandQueue();

        bool Initialize(ID3D12Device* device);
        void Shutdown();

        // Command execution
        uint64_t ExecuteCommandList(ID3D12CommandList* commandList);
        uint64_t ExecuteCommandLists(uint32_t count, ID3D12CommandList* const* commandLists);

        // Synchronization
        void WaitForFence(uint64_t fenceValue);
        void WaitForIdle();
        bool IsFenceComplete(uint64_t fenceValue) const;
        uint64_t GetCompletedFenceValue() const;

        // Command allocator management
        ComPtr<ID3D12CommandAllocator> GetCommandAllocator();
        void ReturnCommandAllocator(ComPtr<ID3D12CommandAllocator> allocator, uint64_t fenceValue);

        // Properties
        ID3D12CommandQueue* GetD3D12CommandQueue() const { return commandQueue_.Get(); }
        D3D12_COMMAND_LIST_TYPE GetType() const { return type_; }

        void SetDebugName(const char* name);
        const char* GetDebugName() const { return debugName_.c_str(); }

    private:
        void ProcessCompletedAllocators();
    };

    // ========================================================================
    // D3D12 Device Implementation
    // ========================================================================

    class D3D12Device : public IRenderDevice {
    private:
        // Core D3D12 objects
        ComPtr<ID3D12Device> device_;
        ComPtr<IDXGIFactory7> dxgiFactory_;
        ComPtr<IDXGIAdapter4> adapter_;

        // Debug interfaces
        ComPtr<ID3D12Debug> d3d12Debug_;
        ComPtr<ID3D12Debug1> d3d12Debug1_;
        ComPtr<ID3D12Debug5> d3d12Debug5_;
        ComPtr<ID3D12DebugDevice> debugDevice_;
        ComPtr<ID3D12InfoQueue> infoQueue_;

        // Command queues
        std::unique_ptr<D3D12CommandQueue> graphicsQueue_;
        std::unique_ptr<D3D12CommandQueue> computeQueue_;
        std::unique_ptr<D3D12CommandQueue> copyQueue_;

        // Resource management
        std::unique_ptr<D3D12ResourceManager> resourceManager_;

        // Swap chain
        std::unique_ptr<D3D12SwapChain> swapChain_;

        // Descriptor heaps
        std::unique_ptr<D3D12DescriptorHeap> rtvHeap_;
        std::unique_ptr<D3D12DescriptorHeap> dsvHeap_;
        std::unique_ptr<D3D12DescriptorHeap> srvHeap_;
        std::unique_ptr<D3D12DescriptorHeap> samplerHeap_;

        // Device state
        DeviceCapabilities capabilities_;
        RenderStats renderStats_;
        std::vector<DebugMessage> debugMessages_;

        // Configuration
        Configuration::RenderingConfig config_;
        Platform::RendererSurfaceInfo surfaceInfo_;

        // Synchronization
        std::mutex deviceMutex_;
        std::atomic<bool> isInitialized_{ false };
        std::atomic<uint64_t> frameNumber_{ 0 };

        // Logging
        Logging::LogChannel& logChannel_;

        // Debug and validation
        bool debugLayerEnabled_ = false;
        bool gpuValidationEnabled_ = false;
        bool dredEnabled_ = false;

    public:
        D3D12Device();
        virtual ~D3D12Device();

        // IRenderDevice implementation
        bool Initialize(const Configuration::RenderingConfig& config,
            const Platform::SurfaceInfo& surfaceInfo) override;
        void Shutdown() override;
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

        // Command lists
        std::unique_ptr<ICommandList> CreateCommandList(CommandListType type) override;
        void ExecuteCommandList(ICommandList* commandList) override;
        void ExecuteCommandLists(uint32_t count, ICommandList* const* commandLists) override;

        // Swap chain
        ISwapChain* GetSwapChain() override;

        // Descriptor management
        std::unique_ptr<IDescriptorHeap> CreateDescriptorHeap(uint32_t descriptorCount,
            bool shaderVisible) override;

        // Device properties
        const DeviceCapabilities& GetCapabilities() const override { return capabilities_; }
        const RenderStats& GetStats() const override { return renderStats_; }
        std::vector<DebugMessage> GetDebugMessages() override;

        // Debug features
        void SetDebugName(const char* name) override;
        const char* GetDebugName() const override;
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

    private:
        // Initialization helpers
        bool InitializeDebugLayer();
        bool InitializeDevice();
        bool InitializeCommandQueues();
        bool InitializeDescriptorHeaps();
        bool InitializeSwapChain();
        bool InitializeResourceManager();

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
        Logging::LogChannel& logChannel_;

    public:
        D3D12Factory();
        virtual ~D3D12Factory();

        // IRHIFactory implementation
        std::unique_ptr<IRenderDevice> CreateDevice(
            const Configuration::RenderingConfig& config,
            const Platform::SurfaceInfo& surfaceInfo) override;

        bool IsSupported() const override;
        std::vector<std::string> GetSupportedDevices() const override;
        DeviceCapabilities GetDeviceCapabilities(uint32_t deviceIndex = 0) const override;

        void SetDebugName(const char* name) override;
        const char* GetDebugName() const override;

    private:
        bool Initialize();
        void Shutdown();
        void EnumerateAdapters();
        DeviceCapabilities GetAdapterCapabilities(const Platform::AdapterInfo& adapter) const;
    };

} // namespace Akhanda::RHI::D3D12
