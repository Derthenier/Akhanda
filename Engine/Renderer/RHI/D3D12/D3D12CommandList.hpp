// D3D12CommandList.hpp
// Akhanda Game Engine - D3D12 Command List Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <stack>
#include <mutex>
#include <atomic>

import Akhanda.Engine.RHI;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Core.Logging;
import Akhanda.Core.Memory;
import std;

using Microsoft::WRL::ComPtr;

namespace Akhanda::RHI::D3D12 {

    // Forward declarations
    class D3D12Device;
    class D3D12Buffer;
    class D3D12Texture;
    class D3D12Pipeline;
    class D3D12CommandQueue;

    // ========================================================================
    // Resource Barrier Tracker
    // ========================================================================

    class ResourceBarrierTracker {
    private:
        struct PendingBarrier {
            ComPtr<ID3D12Resource> resource;
            D3D12_RESOURCE_STATES beforeState;
            D3D12_RESOURCE_STATES afterState;
            uint32_t subresource;
            bool isBuffer;
        };

        std::vector<PendingBarrier> pendingBarriers_;
        std::vector<D3D12_RESOURCE_BARRIER> barrierBatch_;

        static constexpr uint32_t MAX_BARRIERS_PER_BATCH = 16;

    public:
        void AddTransition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
            D3D12_RESOURCE_STATES after, uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        void AddUAVBarrier(ID3D12Resource* resource);
        void AddAliasBarrier(ID3D12Resource* resourceBefore, ID3D12Resource* resourceAfter);

        void FlushBarriers(ID3D12GraphicsCommandList* commandList);
        void Reset();

        bool HasPendingBarriers() const { return !pendingBarriers_.empty(); }
        uint32_t GetPendingBarrierCount() const { return static_cast<uint32_t>(pendingBarriers_.size()); }
    };

    // ========================================================================
    // Descriptor Binding Tracker
    // ========================================================================

    class DescriptorBindingTracker {
    private:
        struct DescriptorTable {
            D3D12_GPU_DESCRIPTOR_HANDLE baseGpuHandle = {};
            uint32_t descriptorCount = 0;
            bool isDirty = false;
        };

        std::vector<DescriptorTable> graphicsDescriptorTables_;
        std::vector<DescriptorTable> computeDescriptorTables_;

        ID3D12RootSignature* currentGraphicsRootSignature_ = nullptr;
        ID3D12RootSignature* currentComputeRootSignature_ = nullptr;

        bool graphicsRootSignatureDirty_ = false;
        bool computeRootSignatureDirty_ = false;

    public:
        void SetGraphicsRootSignature(ID3D12RootSignature* rootSignature);
        void SetComputeRootSignature(ID3D12RootSignature* rootSignature);

        void SetGraphicsDescriptorTable(uint32_t rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor);
        void SetComputeDescriptorTable(uint32_t rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor);

        void SetGraphicsRootConstantBufferView(uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);
        void SetComputeRootConstantBufferView(uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);

        void SetGraphicsRoot32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* srcData, uint32_t destOffsetIn32BitValues);
        void SetComputeRoot32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* srcData, uint32_t destOffsetIn32BitValues);

        void CommitDescriptorTables(ID3D12GraphicsCommandList* commandList, bool isCompute = false);
        void Reset();
    };

    // ========================================================================
    // D3D12 Command List Implementation
    // ========================================================================

    class D3D12CommandList : public ICommandList {
    private:
        // Core command list objects
        ComPtr<ID3D12GraphicsCommandList> commandList_;
        ComPtr<ID3D12CommandAllocator> commandAllocator_;

        // Device reference
        D3D12Device* device_ = nullptr;
        D3D12CommandQueue* commandQueue_ = nullptr;

        // Command list properties
        CommandListType type_;
        D3D12_COMMAND_LIST_TYPE d3d12Type_;

        // State tracking
        std::atomic<bool> isRecording_{ false };
        std::atomic<bool> isClosed_{ false };
        std::atomic<bool> isReset_{ false };

        // Resource state tracking
        ResourceBarrierTracker barrierTracker_;
        DescriptorBindingTracker descriptorTracker_;

        // Current state
        ID3D12PipelineState* currentPipelineState_ = nullptr;
        D3D_PRIMITIVE_TOPOLOGY currentTopology_ = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

        // Render targets
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> currentRenderTargets_;
        D3D12_CPU_DESCRIPTOR_HANDLE currentDepthStencil_ = {};
        bool hasDepthStencil_ = false;

        // Vertex/Index buffers
        std::vector<D3D12_VERTEX_BUFFER_VIEW> currentVertexBuffers_;
        D3D12_INDEX_BUFFER_VIEW currentIndexBuffer_ = {};
        bool hasIndexBuffer_ = false;

        // Viewports and scissors
        std::vector<D3D12_VIEWPORT> currentViewports_;
        std::vector<D3D12_RECT> currentScissors_;

        // Debug and profiling
        std::stack<std::string> debugEventStack_;
        uint32_t debugEventDepth_ = 0;

        // Statistics
        struct CommandStats {
            uint32_t drawCalls = 0;
            uint32_t dispatchCalls = 0;
            uint32_t resourceBarriers = 0;
            uint32_t descriptorBindings = 0;
            uint32_t debugEvents = 0;
        };
        CommandStats stats_;

        // Debug information
        std::string debugName_;

        // Logging
        Logging::LogChannel& logChannel_;

    public:
        D3D12CommandList(CommandListType type);
        virtual ~D3D12CommandList();

        // ICommandList implementation
        bool Initialize(CommandListType type) override;
        void Shutdown() override;
        void Reset() override;
        void Close() override;

        void SetPipeline(PipelineHandle pipeline) override;
        void SetRootSignature(const RootSignatureDesc& desc) override;
        void SetViewports(uint32_t count, const Viewport* viewports) override;
        void SetScissorRects(uint32_t count, const Scissor* scissors) override;
        void SetPrimitiveTopology(PrimitiveTopology topology) override;

        void SetVertexBuffers(uint32_t startSlot, uint32_t count,
            const BufferHandle* buffers,
            const uint64_t* offsets = nullptr) override;
        void SetIndexBuffer(BufferHandle buffer, Format indexFormat,
            uint64_t offset = 0) override;
        void SetConstantBuffer(uint32_t rootParameterIndex,
            BufferHandle buffer) override;
        void SetShaderResource(uint32_t rootParameterIndex,
            TextureHandle texture) override;
        void SetUnorderedAccess(uint32_t rootParameterIndex,
            TextureHandle texture) override;
        void SetConstants(uint32_t rootParameterIndex,
            uint32_t numConstants,
            const void* constants) override;

        void SetRenderTargets(uint32_t count,
            const TextureHandle* renderTargets,
            TextureHandle depthStencil = {}) override;
        void ClearRenderTarget(TextureHandle renderTarget,
            const float color[4]) override;
        void ClearDepthStencil(TextureHandle depthStencil,
            bool clearDepth = true,
            bool clearStencil = false,
            float depth = 1.0f,
            uint8_t stencil = 0) override;

        void Draw(uint32_t vertexCount, uint32_t startVertex = 0) override;
        void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0,
            int32_t baseVertex = 0) override;
        void DrawInstanced(uint32_t vertexCountPerInstance,
            uint32_t instanceCount,
            uint32_t startVertex = 0,
            uint32_t startInstance = 0) override;
        void DrawIndexedInstanced(uint32_t indexCountPerInstance,
            uint32_t instanceCount,
            uint32_t startIndex = 0,
            int32_t baseVertex = 0,
            uint32_t startInstance = 0) override;

        void Dispatch(uint32_t threadGroupCountX,
            uint32_t threadGroupCountY,
            uint32_t threadGroupCountZ) override;

        void TransitionResource(TextureHandle texture,
            ResourceState fromState,
            ResourceState toState) override;
        void TransitionResource(BufferHandle buffer,
            ResourceState fromState,
            ResourceState toState) override;

        void ResourceBarrier() override;
        void UAVBarrier(TextureHandle texture) override;
        void UAVBarrier(BufferHandle buffer) override;

        void CopyBuffer(BufferHandle dest, BufferHandle src,
            uint64_t destOffset = 0,
            uint64_t srcOffset = 0,
            uint64_t size = UINT64_MAX) override;
        void CopyTexture(TextureHandle dest, TextureHandle src) override;
        void CopyBufferToTexture(TextureHandle dest, BufferHandle src,
            uint32_t srcOffset = 0) override;
        void CopyTextureToBuffer(BufferHandle dest, TextureHandle src,
            uint32_t destOffset = 0) override;

        void BeginEvent(const char* name) override;
        void EndEvent() override;
        void SetMarker(const char* name) override;
        void SetDebugName(const char* name) override;
        const char* GetDebugName() const override { return debugName_.c_str(); }

        CommandListType GetType() const override { return type_; }
        bool IsRecording() const override { return isRecording_.load(); }
        bool IsClosed() const override { return isClosed_.load(); }

        // D3D12-specific methods
        bool InitializeWithDevice(D3D12Device* device, D3D12CommandQueue* commandQueue);

        ID3D12GraphicsCommandList* GetD3D12CommandList() const { return commandList_.Get(); }
        ID3D12CommandAllocator* GetCommandAllocator() const { return commandAllocator_.Get(); }

        // Advanced state management
        void TransitionSubresource(TextureHandle texture, uint32_t subresource,
            ResourceState fromState, ResourceState toState);
        void FlushResourceBarriers();

        // Descriptor heap binding
        void SetDescriptorHeaps(uint32_t count, ID3D12DescriptorHeap* const* descriptorHeaps);

        // Advanced copy operations
        void CopyTextureRegion(TextureHandle dest, uint32_t destX, uint32_t destY, uint32_t destZ,
            TextureHandle src, const D3D12_BOX* srcBox = nullptr);
        void CopyBufferRegion(BufferHandle dest, uint64_t destOffset,
            BufferHandle src, uint64_t srcOffset, uint64_t numBytes);

        // Resolve operations
        void ResolveSubresource(TextureHandle dest, uint32_t destSubresource,
            TextureHandle src, uint32_t srcSubresource, Format format);

        // Query operations
        void BeginQuery(ID3D12QueryHeap* queryHeap, D3D12_QUERY_TYPE type, uint32_t index);
        void EndQuery(ID3D12QueryHeap* queryHeap, D3D12_QUERY_TYPE type, uint32_t index);
        void ResolveQueryData(ID3D12QueryHeap* queryHeap, D3D12_QUERY_TYPE type,
            uint32_t startIndex, uint32_t numQueries,
            BufferHandle destBuffer, uint64_t destOffset);

        // Statistics
        const CommandStats& GetStats() const { return stats_; }
        void ResetStats();

    private:
        // Initialization helpers
        bool CreateCommandAllocator();
        bool CreateCommandList();
        void SetupDebugLayer();

        // State validation
        bool ValidateRecordingState() const;
        bool ValidateDrawState() const;
        bool ValidateDispatchState() const;
        bool ValidateResourceTransition(ID3D12Resource* resource,
            D3D12_RESOURCE_STATES fromState,
            D3D12_RESOURCE_STATES toState) const;

        // Resource helpers
        D3D12Buffer* GetD3D12Buffer(BufferHandle handle) const;
        D3D12Texture* GetD3D12Texture(TextureHandle handle) const;
        D3D12Pipeline* GetD3D12Pipeline(PipelineHandle handle) const;

        // State conversion helpers
        D3D12_RESOURCE_STATES ConvertResourceState(ResourceState state) const;
        D3D_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(PrimitiveTopology topology) const;
        DXGI_FORMAT ConvertFormat(Format format) const;

        // Debug helpers
        void LogCommandListInfo() const;
        void ValidateDebugEventStack() const;

        // Statistics helpers
        void IncrementDrawCall();
        void IncrementDispatchCall();
        void IncrementBarrier();
        void IncrementDescriptorBinding();
        void IncrementDebugEvent();

        // Cleanup helpers
        void CleanupResources();
        void CleanupDebugEvents();
    };

    // ========================================================================
    // D3D12 Command List Utilities
    // ========================================================================

    namespace CommandListUtils {

        // Command list creation helpers
        std::unique_ptr<D3D12CommandList> CreateGraphicsCommandList(D3D12Device* device);
        std::unique_ptr<D3D12CommandList> CreateComputeCommandList(D3D12Device* device);
        std::unique_ptr<D3D12CommandList> CreateCopyCommandList(D3D12Device* device);
        std::unique_ptr<D3D12CommandList> CreateBundleCommandList(D3D12Device* device);

        // State conversion utilities
        D3D12_COMMAND_LIST_TYPE ConvertCommandListType(CommandListType type);
        CommandListType ConvertD3D12CommandListType(D3D12_COMMAND_LIST_TYPE type);

        D3D12_RESOURCE_STATES ConvertResourceState(ResourceState state);
        ResourceState ConvertD3D12ResourceState(D3D12_RESOURCE_STATES state);

        D3D_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(PrimitiveTopology topology);
        PrimitiveTopology ConvertD3D12PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology);

        // Viewport conversion
        D3D12_VIEWPORT ConvertViewport(const Viewport& viewport);
        Viewport ConvertD3D12Viewport(const D3D12_VIEWPORT& viewport);

        // Scissor conversion
        D3D12_RECT ConvertScissor(const Scissor& scissor);
        Scissor ConvertD3D12Scissor(const D3D12_RECT& scissor);

        // Validation utilities
        bool IsValidCommandListType(CommandListType type);
        bool IsValidResourceTransition(ResourceState fromState, ResourceState toState);
        bool IsValidDrawCall(uint32_t vertexCount, uint32_t instanceCount);
        bool IsValidDispatchCall(uint32_t x, uint32_t y, uint32_t z);

        // Resource barrier utilities
        D3D12_RESOURCE_BARRIER CreateTransitionBarrier(ID3D12Resource* resource,
            D3D12_RESOURCE_STATES before,
            D3D12_RESOURCE_STATES after,
            uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        D3D12_RESOURCE_BARRIER CreateUAVBarrier(ID3D12Resource* resource);
        D3D12_RESOURCE_BARRIER CreateAliasBarrier(ID3D12Resource* resourceBefore, ID3D12Resource* resourceAfter);

        // Debug utilities
        const char* CommandListTypeToString(CommandListType type);
        const char* ResourceStateToString(ResourceState state);
        const char* PrimitiveTopologyToString(PrimitiveTopology topology);

        // Performance utilities
        void BeginPIXEvent(ID3D12GraphicsCommandList* commandList, const char* name);
        void EndPIXEvent(ID3D12GraphicsCommandList* commandList);
        void SetPIXMarker(ID3D12GraphicsCommandList* commandList, const char* name);

    } // namespace CommandListUtils

    // ========================================================================
    // D3D12 Command List Manager
    // ========================================================================

    class D3D12CommandListManager {
    private:
        D3D12Device* device_ = nullptr;

        // Command list pools
        std::vector<std::unique_ptr<D3D12CommandList>> graphicsCommandLists_;
        std::vector<std::unique_ptr<D3D12CommandList>> computeCommandLists_;
        std::vector<std::unique_ptr<D3D12CommandList>> copyCommandLists_;

        // Available command lists
        std::queue<D3D12CommandList*> availableGraphicsCommandLists_;
        std::queue<D3D12CommandList*> availableComputeCommandLists_;
        std::queue<D3D12CommandList*> availableCopyCommandLists_;

        // In-flight command lists
        std::vector<std::pair<D3D12CommandList*, uint64_t>> inFlightCommandLists_;

        // Pool management
        std::mutex poolMutex_;
        uint32_t maxPoolSize_ = 32;

        // Statistics
        std::atomic<uint32_t> totalCommandLists_{ 0 };
        std::atomic<uint32_t> activeCommandLists_{ 0 };

        Logging::LogChannel& logChannel_;

    public:
        D3D12CommandListManager();
        ~D3D12CommandListManager();

        bool Initialize(D3D12Device* device);
        void Shutdown();

        // Command list allocation
        D3D12CommandList* GetCommandList(CommandListType type);
        void ReturnCommandList(D3D12CommandList* commandList, uint64_t fenceValue);

        // Pool management
        void ProcessCompletedCommandLists(uint64_t completedFenceValue);
        void FlushPools();

        // Statistics
        uint32_t GetTotalCommandLists() const { return totalCommandLists_.load(); }
        uint32_t GetActiveCommandLists() const { return activeCommandLists_.load(); }
        uint32_t GetAvailableCommandLists(CommandListType type) const;

        // Configuration
        void SetMaxPoolSize(uint32_t size) { maxPoolSize_ = size; }
        uint32_t GetMaxPoolSize() const { return maxPoolSize_; }

    private:
        // Pool helpers
        D3D12CommandList* GetFromPool(CommandListType type);
        void AddToPool(D3D12CommandList* commandList);
        void CreateNewCommandList(CommandListType type);

        // Cleanup
        void CleanupPool(std::vector<std::unique_ptr<D3D12CommandList>>& pool);
    };

} // namespace Akhanda::RHI::D3D12
