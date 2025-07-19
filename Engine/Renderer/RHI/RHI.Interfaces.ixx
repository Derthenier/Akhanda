// RHI.Interfaces.ixx
// Akhanda Game Engine - Core RHI Interface Classes
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

#undef DeviceCapabilities // Avoid macro conflicts

export module Akhanda.Engine.RHI.Interfaces;

import Akhanda.Engine.RHI;
import Akhanda.Core.Math;
import Akhanda.Core.Memory;
import Akhanda.Core.Configuration.Rendering;
import Akhanda.Platform.Interfaces;


export namespace Akhanda::RHI {

    // ========================================================================
    // Forward Declarations
    // ========================================================================

    class RenderDevice;

    // ========================================================================
    // SwapChain Interface
    // ========================================================================

    class ISwapChain {
    public:
        virtual ~ISwapChain() = default;

        // Swap chain management
        virtual bool Initialize(const Platform::SurfaceInfo& surfaceInfo,
            const Configuration::RenderingConfig& config) = 0;
        virtual void Shutdown() = 0;
        virtual bool Resize(uint32_t width, uint32_t height) = 0;
        virtual void Present(bool vsync = true) = 0;

        // Back buffer access
        virtual uint32_t GetCurrentBackBufferIndex() const = 0;
        virtual uint32_t GetBackBufferCount() const = 0;
        virtual TextureHandle GetBackBuffer(uint32_t index) const = 0;
        virtual TextureHandle GetCurrentBackBuffer() const = 0;

        // Properties
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual Format GetFormat() const = 0;
        virtual bool IsFullscreen() const = 0;
        virtual bool IsVSyncEnabled() const = 0;

        // Debug
        virtual void SetDebugName(const char* name) = 0;
        virtual const char* GetDebugName() const = 0;
    };

    // ========================================================================
    // Command List Interface
    // ========================================================================

    class ICommandList {
    public:
        virtual ~ICommandList() = default;

        // Command list lifecycle
        virtual bool Initialize(CommandListType type) = 0;
        virtual void Shutdown() = 0;
        virtual void Reset() = 0;
        virtual void Close() = 0;

        // State management
        virtual void SetPipeline(PipelineHandle pipeline) = 0;
        virtual void SetRootSignature(const RootSignatureDesc& desc) = 0;
        virtual void SetViewports(uint32_t count, const Viewport* viewports) = 0;
        virtual void SetScissorRects(uint32_t count, const Scissor* scissors) = 0;
        virtual void SetPrimitiveTopology(PrimitiveTopology topology) = 0;

        // Resource binding
        virtual void SetVertexBuffers(uint32_t startSlot, uint32_t count,
            const BufferHandle* buffers,
            const uint64_t* offsets = nullptr) = 0;
        virtual void SetIndexBuffer(BufferHandle buffer, Format indexFormat,
            uint64_t offset = 0) = 0;
        virtual void SetConstantBuffer(uint32_t rootParameterIndex,
            BufferHandle buffer) = 0;
        virtual void SetShaderResource(uint32_t rootParameterIndex,
            TextureHandle texture) = 0;
        virtual void SetUnorderedAccess(uint32_t rootParameterIndex,
            TextureHandle texture) = 0;
        virtual void SetConstants(uint32_t rootParameterIndex,
            uint32_t numConstants,
            const void* constants) = 0;

        // Render targets
        virtual void SetRenderTargets(uint32_t count,
            const TextureHandle* renderTargets,
            TextureHandle depthStencil = {}) = 0;
        virtual void ClearRenderTarget(TextureHandle renderTarget,
            const float color[4]) = 0;
        virtual void ClearDepthStencil(TextureHandle depthStencil,
            bool clearDepth = true,
            bool clearStencil = false,
            float depth = 1.0f,
            uint8_t stencil = 0) = 0;

        // Drawing commands
        virtual void Draw(uint32_t vertexCount, uint32_t startVertex = 0) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0,
            int32_t baseVertex = 0) = 0;
        virtual void DrawInstanced(uint32_t vertexCountPerInstance,
            uint32_t instanceCount,
            uint32_t startVertex = 0,
            uint32_t startInstance = 0) = 0;
        virtual void DrawIndexedInstanced(uint32_t indexCountPerInstance,
            uint32_t instanceCount,
            uint32_t startIndex = 0,
            int32_t baseVertex = 0,
            uint32_t startInstance = 0) = 0;

        // Compute commands
        virtual void Dispatch(uint32_t threadGroupCountX,
            uint32_t threadGroupCountY,
            uint32_t threadGroupCountZ) = 0;

        // Resource transitions
        virtual void TransitionResource(TextureHandle texture,
            ResourceState fromState,
            ResourceState toState) = 0;
        virtual void TransitionResource(BufferHandle buffer,
            ResourceState fromState,
            ResourceState toState) = 0;

        // Resource barriers
        virtual void ResourceBarrier() = 0;
        virtual void UAVBarrier(TextureHandle texture) = 0;
        virtual void UAVBarrier(BufferHandle buffer) = 0;

        // Resource copy operations
        virtual void CopyBuffer(BufferHandle dest, BufferHandle src,
            uint64_t destOffset = 0,
            uint64_t srcOffset = 0,
            uint64_t size = UINT64_MAX) = 0;
        virtual void CopyTexture(TextureHandle dest, TextureHandle src) = 0;
        virtual void CopyBufferToTexture(TextureHandle dest, BufferHandle src,
            uint32_t srcOffset = 0) = 0;
        virtual void CopyTextureToBuffer(BufferHandle dest, TextureHandle src,
            uint32_t destOffset = 0) = 0;

        // Debug and profiling
        virtual void BeginEvent(const char* name) = 0;
        virtual void EndEvent() = 0;
        virtual void SetMarker(const char* name) = 0;
        virtual void SetDebugName(const char* name) = 0;
        virtual const char* GetDebugName() const = 0;

        // Properties
        virtual CommandListType GetType() const = 0;
        virtual bool IsRecording() const = 0;
        virtual bool IsClosed() const = 0;
    };

    // ========================================================================
    // Buffer Interface
    // ========================================================================

    class IRenderBuffer : public IRenderResource {
    public:
        virtual ~IRenderBuffer() = default;

        // Buffer management
        virtual bool Initialize(const BufferDesc& desc) = 0;
        virtual void Shutdown() = 0;

        // Data access
        virtual void* Map(uint64_t offset = 0, uint64_t size = 0) = 0;
        virtual void Unmap() = 0;
        virtual void UpdateData(const void* data, uint64_t size,
            uint64_t offset = 0) = 0;

        // Properties
        virtual uint64_t GetSize() const = 0;
        virtual uint32_t GetStride() const = 0;
        virtual ResourceUsage GetUsage() const = 0;
        virtual bool IsCPUAccessible() const = 0;
        virtual bool IsMapped() const = 0;

        // Resource interface
        ResourceType GetType() const override { return ResourceType::Buffer; }
    };

    // ========================================================================
    // Texture Interface
    // ========================================================================

    class IRenderTexture : public IRenderResource {
    public:
        virtual ~IRenderTexture() = default;

        // Texture management
        virtual bool Initialize(const TextureDesc& desc) = 0;
        virtual void Shutdown() = 0;

        // Data access
        virtual void UpdateData(const void* data, uint32_t mipLevel = 0,
            uint32_t arraySlice = 0) = 0;

        // Properties
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual uint32_t GetDepth() const = 0;
        virtual uint32_t GetMipLevels() const = 0;
        virtual uint32_t GetArraySize() const = 0;
        virtual Format GetFormat() const = 0;
        virtual uint32_t GetSampleCount() const = 0;
        virtual uint32_t GetSampleQuality() const = 0;
        virtual ResourceUsage GetUsage() const = 0;
        virtual ResourceState GetCurrentState() const = 0;

        // Resource interface
        ResourceType GetType() const override { return ResourceType::Texture2D; }
    };

    // ========================================================================
    // Pipeline Interface
    // ========================================================================

    class IRenderPipeline {
    public:
        virtual ~IRenderPipeline() = default;

        // Pipeline management
        virtual bool Initialize(const GraphicsPipelineDesc& desc) = 0;
        virtual bool Initialize(const ComputePipelineDesc& desc) = 0;
        virtual void Shutdown() = 0;

        // Properties
        virtual bool IsGraphicsPipeline() const = 0;
        virtual bool IsComputePipeline() const = 0;
        virtual const RootSignatureDesc& GetRootSignature() const = 0;

        // Debug
        virtual void SetDebugName(const char* name) = 0;
        virtual const char* GetDebugName() const = 0;
    };

    // ========================================================================
    // Descriptor Heap Interface
    // ========================================================================

    class IDescriptorHeap {
    public:
        virtual ~IDescriptorHeap() = default;

        // Heap management
        virtual bool Initialize(uint32_t descriptorCount, bool shaderVisible) = 0;
        virtual void Shutdown() = 0;

        // Descriptor allocation
        virtual uint32_t AllocateDescriptor() = 0;
        virtual void FreeDescriptor(uint32_t index) = 0;
        virtual void Reset() = 0;

        // View creation
        virtual void CreateConstantBufferView(uint32_t descriptorIndex,
            BufferHandle buffer) = 0;
        virtual void CreateShaderResourceView(uint32_t descriptorIndex,
            TextureHandle texture) = 0;
        virtual void CreateUnorderedAccessView(uint32_t descriptorIndex,
            TextureHandle texture) = 0;
        virtual void CreateRenderTargetView(uint32_t descriptorIndex,
            TextureHandle texture) = 0;
        virtual void CreateDepthStencilView(uint32_t descriptorIndex,
            TextureHandle texture) = 0;
        virtual void CreateSampler(uint32_t descriptorIndex,
            const SamplerDesc& desc) = 0;

        // Properties
        virtual uint32_t GetDescriptorCount() const = 0;
        virtual uint32_t GetFreeDescriptorCount() const = 0;
        virtual bool IsShaderVisible() const = 0;
        virtual uint64_t GetGPUAddress(uint32_t descriptorIndex) const = 0;

        // Debug
        virtual void SetDebugName(const char* name) = 0;
        virtual const char* GetDebugName() const = 0;
    };

    // ========================================================================
    // Render Device Interface
    // ========================================================================

    class IRenderDevice {
    public:
        virtual ~IRenderDevice() = default;

        // Device lifecycle
        virtual bool Initialize(const Configuration::RenderingConfig& config,
            const Platform::SurfaceInfo& surfaceInfo) = 0;
        virtual void Shutdown() = 0;
        virtual void WaitForIdle() = 0;

        // Frame management
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void Present() = 0;

        // Resource creation
        virtual BufferHandle CreateBuffer(const BufferDesc& desc) = 0;
        virtual TextureHandle CreateTexture(const TextureDesc& desc) = 0;
        virtual PipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
        virtual PipelineHandle CreateComputePipeline(const ComputePipelineDesc& desc) = 0;

        // Resource destruction
        virtual void DestroyBuffer(BufferHandle handle) = 0;
        virtual void DestroyTexture(TextureHandle handle) = 0;
        virtual void DestroyPipeline(PipelineHandle handle) = 0;

        // Resource access
        virtual IRenderBuffer* GetBuffer(BufferHandle handle) = 0;
        virtual IRenderTexture* GetTexture(TextureHandle handle) = 0;
        virtual IRenderPipeline* GetPipeline(PipelineHandle handle) = 0;

        // Command lists
        virtual std::unique_ptr<ICommandList> CreateCommandList(CommandListType type) = 0;
        virtual void ExecuteCommandList(ICommandList* commandList) = 0;
        virtual void ExecuteCommandLists(uint32_t count, ICommandList* const* commandLists) = 0;

        // Swap chain
        virtual ISwapChain* GetSwapChain() = 0;

        // Descriptor management
        virtual std::unique_ptr<IDescriptorHeap> CreateDescriptorHeap(uint32_t descriptorCount,
            bool shaderVisible) = 0;

        // Device properties
        virtual const DeviceCapabilities& GetCapabilities() const = 0;
        virtual const RenderStats& GetStats() const = 0;
        virtual std::vector<DebugMessage> GetDebugMessages() = 0;

        // Debug features
        virtual void SetDebugName(const char* name) = 0;
        virtual const char* GetDebugName() const = 0;
        virtual void BeginDebugEvent(const char* name) = 0;
        virtual void EndDebugEvent() = 0;
        virtual void InsertDebugMarker(const char* name) = 0;

        // Memory management
        virtual uint64_t GetUsedVideoMemory() const = 0;
        virtual uint64_t GetAvailableVideoMemory() const = 0;
        virtual void FlushMemory() = 0;
    };

    // ========================================================================
    // RHI Factory Interface
    // ========================================================================

    class IRHIFactory {
    public:
        virtual ~IRHIFactory() = default;

        // Device creation
        virtual std::unique_ptr<IRenderDevice> CreateDevice(
            const Configuration::RenderingConfig& config,
            const Platform::SurfaceInfo& surfaceInfo) = 0;

        // Capability queries
        virtual bool IsSupported() const = 0;
        virtual std::vector<std::string> GetSupportedDevices() const = 0;
        virtual DeviceCapabilities GetDeviceCapabilities(uint32_t deviceIndex = 0) const = 0;

        // Debug
        virtual void SetDebugName(const char* name) = 0;
        virtual const char* GetDebugName() const = 0;
    };

    // ========================================================================
    // Factory Creation Functions
    // ========================================================================

    // Create D3D12 factory
    std::unique_ptr<IRHIFactory> CreateD3D12Factory();

    // Create factory based on configuration
    std::unique_ptr<IRHIFactory> CreateRHIFactory(Configuration::RenderingAPI api);

    // ========================================================================
    // Utility Functions
    // ========================================================================

    // Format conversion utilities
    Format GetDepthFormat(Format colorFormat);
    uint32_t GetFormatBytesPerPixel(Format format);
    bool IsDepthFormat(Format format);
    bool IsCompressedFormat(Format format);
    bool IsSRGBFormat(Format format);

    // Resource state transitions
    ResourceState GetOptimalResourceState(ResourceUsage usage);
    bool IsValidStateTransition(ResourceState from, ResourceState to);

    // Debug utilities
    const char* ResourceStateToString(ResourceState state);
    const char* ResourceUsageToString(ResourceUsage usage);
    const char* FormatToString(Format format);
    const char* CommandListTypeToString(CommandListType type);

} // namespace Akhanda::RHI
