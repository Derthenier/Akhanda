// D3D12Buffer.hpp
// Akhanda Game Engine - D3D12 Buffer Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <D3D12MemAlloc.h>
#include <memory>
#include <mutex>
#include <atomic>

import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Engine.RHI;
import Akhanda.Core.Logging;
import Akhanda.Core.Memory;
import std;

using Microsoft::WRL::ComPtr;

namespace Akhanda::RHI::D3D12 {

    // ========================================================================
    // D3D12 Buffer Implementation
    // ========================================================================

    class D3D12Buffer : public IRenderBuffer {
    private:
        // Core buffer objects
        ComPtr<ID3D12Resource> buffer_;
        D3D12MA::Allocation* allocation_ = nullptr;

        // Buffer properties
        BufferDesc desc_;
        ResourceState currentState_;
        D3D12_RESOURCE_STATES d3d12State_;

        // Memory mapping
        void* mappedData_ = nullptr;
        std::atomic<bool> isMapped_{ false };
        std::mutex mapMutex_;

        // GPU virtual address
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress_ = 0;

        // Views
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ = {};
        D3D12_INDEX_BUFFER_VIEW indexBufferView_ = {};
        D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc_ = {};

        // Debug information
        std::string debugName_;

        // Validity
        std::atomic<bool> isValid_{ false };

        // Logging
        Logging::LogChannel& logChannel_;

    public:
        D3D12Buffer();
        virtual ~D3D12Buffer();

        // IRenderBuffer implementation
        bool Initialize(const BufferDesc& desc) override;
        void Shutdown() override;

        void* Map(uint64_t offset = 0, uint64_t size = 0) override;
        void Unmap() override;
        void UpdateData(const void* data, uint64_t size, uint64_t offset = 0) override;

        uint64_t GetSize() const override { return desc_.size; }
        uint32_t GetStride() const override { return desc_.stride; }
        ResourceUsage GetUsage() const override { return desc_.usage; }
        bool IsCPUAccessible() const override { return desc_.cpuAccessible; }
        bool IsMapped() const override { return isMapped_.load(); }

        // IRenderResource implementation
        uint64_t GetGPUAddress() const override { return gpuAddress_; }
        void SetDebugName(const char* name) override;
        const char* GetDebugName() const override { return debugName_.c_str(); }
        bool IsValid() const override { return isValid_.load(); }

        // D3D12-specific methods
        bool InitializeWithDevice(const BufferDesc& desc, ID3D12Device* device,
            D3D12MA::Allocator* allocator);

        ID3D12Resource* GetD3D12Resource() const { return buffer_.Get(); }
        D3D12MA::Allocation* GetAllocation() const { return allocation_; }

        // Resource state management
        ResourceState GetCurrentState() const { return currentState_; }
        void SetCurrentState(ResourceState state);
        D3D12_RESOURCE_STATES GetD3D12State() const { return d3d12State_; }

        // Buffer views
        const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBufferView_; }
        const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return indexBufferView_; }
        const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetConstantBufferViewDesc() const { return constantBufferViewDesc_; }

        // Utility methods
        bool IsVertexBuffer() const { return (desc_.usage & ResourceUsage::VertexBuffer) != ResourceUsage::None; }
        bool IsIndexBuffer() const { return (desc_.usage & ResourceUsage::IndexBuffer) != ResourceUsage::None; }
        bool IsConstantBuffer() const { return (desc_.usage & ResourceUsage::ConstantBuffer) != ResourceUsage::None; }
        bool IsUploadBuffer() const { return desc_.cpuAccessible; }
        bool IsReadbackBuffer() const;

        // Memory statistics
        uint64_t GetMemorySize() const;
        uint64_t GetAlignedSize() const;

    private:
        // Initialization helpers
        bool CreateBuffer(ID3D12Device* device, D3D12MA::Allocator* allocator);
        bool CreateViews();
        void SetupHeapProperties(D3D12_HEAP_PROPERTIES& heapProps) const;
        void SetupResourceDesc(D3D12_RESOURCE_DESC& resourceDesc) const;
        void SetupInitialResourceState(D3D12_RESOURCE_STATES& initialState) const;

        // Validation helpers
        bool ValidateBufferDesc(const BufferDesc& desc) const;
        bool ValidateMapParameters(uint64_t offset, uint64_t size) const;
        bool ValidateUpdateParameters(const void* data, uint64_t size, uint64_t offset) const;

        // Buffer view creation
        void CreateVertexBufferView();
        void CreateIndexBufferView();
        void CreateConstantBufferView();

        // Memory management helpers
        bool MapInternal(uint64_t offset, uint64_t size);
        void UnmapInternal();

        // Debug helpers
        void LogBufferInfo() const;
        void LogMemoryInfo() const;

        // Utility functions
        D3D12_HEAP_TYPE GetHeapType() const;
        D3D12_RESOURCE_FLAGS GetResourceFlags() const;
        size_t GetRequiredAlignment() const;

        // Cleanup
        void CleanupResources();
    };

    // ========================================================================
    // D3D12 Buffer Utilities
    // ========================================================================

    namespace BufferUtils {

        // Buffer creation helpers
        std::unique_ptr<D3D12Buffer> CreateVertexBuffer(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint64_t size,
            uint32_t stride,
            const void* initialData = nullptr,
            const char* debugName = nullptr);

        std::unique_ptr<D3D12Buffer> CreateIndexBuffer(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint64_t size,
            Format indexFormat,
            const void* initialData = nullptr,
            const char* debugName = nullptr);

        std::unique_ptr<D3D12Buffer> CreateConstantBuffer(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint64_t size,
            const void* initialData = nullptr,
            const char* debugName = nullptr);

        std::unique_ptr<D3D12Buffer> CreateUploadBuffer(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint64_t size,
            const char* debugName = nullptr);

        std::unique_ptr<D3D12Buffer> CreateReadbackBuffer(ID3D12Device* device,
            D3D12MA::Allocator* allocator,
            uint64_t size,
            const char* debugName = nullptr);

        // Buffer size calculation
        uint64_t CalculateConstantBufferSize(uint64_t size);
        uint64_t CalculateVertexBufferSize(uint64_t vertexCount, uint32_t stride);
        uint64_t CalculateIndexBufferSize(uint64_t indexCount, Format indexFormat);

        // Buffer alignment
        uint64_t AlignBufferSize(uint64_t size, uint64_t alignment);
        uint64_t GetConstantBufferAlignment();
        uint64_t GetVertexBufferAlignment();
        uint64_t GetIndexBufferAlignment();

        // Format conversion
        DXGI_FORMAT GetDXGIFormat(Format format);
        Format GetEngineFormat(DXGI_FORMAT format);
        uint32_t GetFormatSize(Format format);

        // Resource usage conversion
        D3D12_RESOURCE_FLAGS ConvertResourceUsageToFlags(ResourceUsage usage);
        D3D12_HEAP_TYPE ConvertResourceUsageToHeapType(ResourceUsage usage, bool cpuAccessible);
        D3D12_RESOURCE_STATES ConvertResourceUsageToState(ResourceUsage usage);

        // Resource state conversion
        D3D12_RESOURCE_STATES ConvertResourceState(ResourceState state);
        ResourceState ConvertD3D12ResourceState(D3D12_RESOURCE_STATES state);

        // Validation
        bool IsValidBufferSize(uint64_t size);
        bool IsValidVertexStride(uint32_t stride);
        bool IsValidIndexFormat(Format format);
        bool IsCompatibleUsage(ResourceUsage usage, bool cpuAccessible);

        // Debug utilities
        const char* ResourceUsageToString(ResourceUsage usage);
        const char* ResourceStateToString(ResourceState state);
        const char* D3D12ResourceStateToString(D3D12_RESOURCE_STATES state);
        const char* D3D12HeapTypeToString(D3D12_HEAP_TYPE heapType);

    } // namespace BufferUtils

    // ========================================================================
    // D3D12 Buffer Manager
    // ========================================================================

    class D3D12BufferManager {
    private:
        ID3D12Device* device_ = nullptr;
        D3D12MA::Allocator* allocator_ = nullptr;

        // Buffer pools for common sizes
        std::vector<std::unique_ptr<D3D12Buffer>> vertexBufferPool_;
        std::vector<std::unique_ptr<D3D12Buffer>> indexBufferPool_;
        std::vector<std::unique_ptr<D3D12Buffer>> constantBufferPool_;
        std::vector<std::unique_ptr<D3D12Buffer>> uploadBufferPool_;

        // Pool management
        std::mutex poolMutex_;
        std::atomic<uint32_t> totalBuffers_{ 0 };
        std::atomic<uint64_t> totalMemory_{ 0 };

        // Configuration
        uint32_t maxPoolSize_ = 1000;
        uint64_t maxPoolMemory_ = 256 * 1024 * 1024; // 256MB

        Logging::LogChannel& logChannel_;

    public:
        D3D12BufferManager();
        ~D3D12BufferManager();

        bool Initialize(ID3D12Device* device, D3D12MA::Allocator* allocator);
        void Shutdown();

        // Buffer creation with pooling
        std::unique_ptr<D3D12Buffer> CreateBuffer(const BufferDesc& desc);
        void ReturnBuffer(std::unique_ptr<D3D12Buffer> buffer);

        // Pool management
        void FlushPools();
        void TrimPools();

        // Statistics
        uint32_t GetTotalBuffers() const { return totalBuffers_.load(); }
        uint64_t GetTotalMemory() const { return totalMemory_.load(); }
        uint32_t GetPoolSize(ResourceUsage usage) const;

        // Configuration
        void SetMaxPoolSize(uint32_t size) { maxPoolSize_ = size; }
        void SetMaxPoolMemory(uint64_t memory) { maxPoolMemory_ = memory; }

    private:
        // Pool helpers
        std::unique_ptr<D3D12Buffer> GetFromPool(ResourceUsage usage, uint64_t size);
        void AddToPool(std::unique_ptr<D3D12Buffer> buffer);
        bool CanAddToPool(const D3D12Buffer* buffer) const;

        // Cleanup
        void CleanupPool(std::vector<std::unique_ptr<D3D12Buffer>>& pool);
    };

} // namespace Akhanda::RHI::D3D12
