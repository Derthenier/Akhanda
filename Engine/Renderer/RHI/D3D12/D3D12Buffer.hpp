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
#include <vector>
#include <unordered_map>
#include <queue>
#include <array>

#include "D3D12Device.hpp"

import Akhanda.Engine.RHI;
import Akhanda.Engine.RHI.Interfaces;
import Akhanda.Core.Logging;
import Akhanda.Core.Memory;

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

        // Memory footprint
        uint64_t memorySize_ = 0;
        uint64_t stagingMemorySize_ = 0;
        D3D12_RESOURCE_ALLOCATION_INFO allocationInfo_ = {};

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

        // Resource usage conversion
        D3D12_RESOURCE_FLAGS ConvertResourceUsageToFlags(ResourceUsage usage);
        D3D12_HEAP_TYPE ConvertResourceUsageToHeapType(ResourceUsage usage, bool cpuAccessible);
        D3D12_RESOURCE_STATES ConvertResourceUsageToState(ResourceUsage usage);

        // Validation
        bool IsValidBufferSize(uint64_t size);
        bool IsValidVertexStride(uint32_t stride);
        bool IsValidIndexFormat(Format format);
        bool IsCompatibleUsage(ResourceUsage usage, bool cpuAccessible);

    } // namespace BufferUtils


    // ========================================================================
    // Buffer Pool Configuration
    // ========================================================================

    struct BufferPoolConfig {
        uint64_t minBufferSize = 1024;           // 1KB minimum
        uint64_t maxBufferSize = 256 * 1024 * 1024;  // 256MB maximum
        uint32_t maxPooledBuffers = 128;         // Maximum buffers per pool
        uint32_t initialPoolSize = 16;           // Initial buffers to pre-allocate
        bool enablePooling = true;               // Enable/disable buffer pooling
        bool enableStatistics = true;           // Enable memory statistics tracking
    };

    // ========================================================================
    // Buffer Pool Statistics
    // ========================================================================

    struct BufferPoolStats {
        uint32_t totalBuffersCreated = 0;
        uint32_t totalBuffersDestroyed = 0;
        uint32_t activeBuffers = 0;
        uint32_t pooledBuffers = 0;
        uint64_t totalMemoryAllocated = 0;
        uint64_t totalMemoryInUse = 0;
        uint64_t peakMemoryUsage = 0;
        uint32_t poolHits = 0;
        uint32_t poolMisses = 0;

        void Reset() {
            totalBuffersCreated = 0;
            totalBuffersDestroyed = 0;
            activeBuffers = 0;
            pooledBuffers = 0;
            totalMemoryAllocated = 0;
            totalMemoryInUse = 0;
            peakMemoryUsage = 0;
            poolHits = 0;
            poolMisses = 0;
        }
    };

    // ========================================================================
    // Staging Buffer Management
    // ========================================================================

    struct StagingBufferDesc {
        uint64_t size;
        uint32_t alignment;
        bool persistent;  // Keep mapped permanently
        const char* debugName;
    };

    class D3D12StagingBuffer {
    private:
        std::unique_ptr<D3D12Buffer> buffer_;
        uint64_t currentOffset_ = 0;
        uint64_t size_ = 0;
        void* mappedData_ = nullptr;
        bool isPersistentlyMapped_ = false;
        std::mutex offsetMutex_;

    public:
        bool Initialize(const StagingBufferDesc& desc, ID3D12Device* device, D3D12MA::Allocator* allocator);
        void Shutdown();

        struct StagingAllocation {
            void* cpuAddress;
            D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
            uint64_t offset;
            uint64_t size;
            bool valid;
        };

        StagingAllocation Allocate(uint64_t size, uint32_t alignment = 256);
        void Reset();  // Reset for next frame

        uint64_t GetSize() const { return size_; }
        uint64_t GetUsedSize() const { return currentOffset_; }
        uint64_t GetAvailableSize() const { return size_ - currentOffset_; }
        D3D12Buffer* GetBuffer() const { return buffer_.get(); }
    };

    // ========================================================================
    // Buffer Handle Management
    // ========================================================================

    class D3D12BufferHandleManager {
    private:
        struct HandleEntry {
            std::unique_ptr<D3D12Buffer> buffer;
            uint32_t refCount = 1;
            bool isValid = true;
        };

        std::unordered_map<uint32_t, HandleEntry> handleMap_;
        std::queue<uint32_t> availableHandles_;
        std::atomic<uint32_t> nextHandleId_{ 1 };
        std::mutex handleMutex_;

    public:
        BufferHandle CreateHandle(std::unique_ptr<D3D12Buffer> buffer);
        D3D12Buffer* GetBuffer(BufferHandle handle);
        bool IsValid(BufferHandle handle);
        void AddRef(BufferHandle handle);
        void Release(BufferHandle handle);
        void InvalidateHandle(BufferHandle handle);

        // Statistics
        uint32_t GetActiveHandleCount() const;
        std::vector<BufferHandle> GetActiveHandles() const;
    };

    // ========================================================================
    // Buffer Pool Implementation
    // ========================================================================

    class D3D12BufferPool {
    private:
        struct PoolKey {
            ResourceUsage usage;
            uint64_t sizeCategory;  // Bucketed size for pooling
            bool cpuAccessible;

            bool operator==(const PoolKey& other) const {
                return usage == other.usage &&
                    sizeCategory == other.sizeCategory &&
                    cpuAccessible == other.cpuAccessible;
            }
        };

        struct PoolKeyHash {
            size_t operator()(const PoolKey& key) const {
                size_t h1 = std::hash<uint32_t>{}(static_cast<uint32_t>(key.usage));
                size_t h2 = std::hash<uint64_t>{}(key.sizeCategory);
                size_t h3 = std::hash<bool>{}(key.cpuAccessible);
                return h1 ^ (h2 << 1) ^ (h3 << 2);
            }
        };

        using BufferPool = std::queue<std::unique_ptr<D3D12Buffer>>;
        std::unordered_map<PoolKey, BufferPool, PoolKeyHash> pools_;

        BufferPoolConfig config_;
        BufferPoolStats stats_;
        std::mutex poolMutex_;

        uint64_t GetSizeCategory(uint64_t size) const;
        PoolKey CreatePoolKey(const BufferDesc& desc) const;

    public:
        explicit D3D12BufferPool(const BufferPoolConfig& config);
        ~D3D12BufferPool();

        std::unique_ptr<D3D12Buffer> AcquireBuffer(const BufferDesc& desc,
            ID3D12Device* device,
            D3D12MA::Allocator* allocator);
        void ReturnBuffer(std::unique_ptr<D3D12Buffer> buffer);

        void PreallocateBuffers(const BufferDesc& desc, uint32_t count,
            ID3D12Device* device, D3D12MA::Allocator* allocator);

        // Statistics and management
        const BufferPoolStats& GetStats() const { return stats_; }
        void ResetStats() { std::lock_guard<std::mutex> lock(poolMutex_); stats_.Reset(); }
        void TrimPools(uint32_t maxBuffersPerPool = 0);
        void LogPoolStatistics() const;
    };

    // ========================================================================
    // Main Buffer Manager
    // ========================================================================

    class D3D12BufferManager {
    private:
        // Core D3D12 objects
        D3D12Device* device_ = nullptr;
        D3D12MA::Allocator* allocator_ = nullptr;

        // Management components
        std::unique_ptr<D3D12BufferPool> bufferPool_;
        std::unique_ptr<D3D12BufferHandleManager> handleManager_;

        // Staging buffer management
        std::array<std::unique_ptr<D3D12StagingBuffer>, 3> stagingBuffers_; // Triple buffered
        uint32_t currentStagingBufferIndex_ = 0;
        std::mutex stagingBufferMutex_;

        // Configuration
        BufferPoolConfig poolConfig_;

        // Statistics
        mutable BufferPoolStats globalStats_;
        mutable std::mutex statsMutex_;

        // Logging
        Logging::LogChannel& logChannel_;

        // Lifecycle
        std::atomic<bool> isInitialized_{ false };

    public:
        explicit D3D12BufferManager();
        ~D3D12BufferManager();

        // Initialization
        bool Initialize(D3D12Device* device, D3D12MA::Allocator* allocator,
            const BufferPoolConfig& config = {});
        void Shutdown();

        // Buffer creation and management
        BufferHandle CreateBuffer(const BufferDesc& desc);
        void DestroyBuffer(BufferHandle handle);
        D3D12Buffer* GetBuffer(BufferHandle handle);
        bool IsValidBuffer(BufferHandle handle) const;

        // Specialized buffer creation
        BufferHandle CreateVertexBuffer(uint64_t size, uint32_t stride,
            const void* initialData = nullptr,
            const char* debugName = nullptr);
        BufferHandle CreateIndexBuffer(uint64_t size, Format indexFormat,
            const void* initialData = nullptr,
            const char* debugName = nullptr);
        BufferHandle CreateConstantBuffer(uint64_t size,
            const void* initialData = nullptr,
            const char* debugName = nullptr);
        BufferHandle CreateStructuredBuffer(uint64_t elementCount, uint32_t elementSize,
            const void* initialData = nullptr,
            const char* debugName = nullptr);

        // Staging buffer operations
        struct StagingUpload {
            void* cpuAddress;
            D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
            uint64_t size;
            bool valid;
        };

        StagingUpload AllocateStagingMemory(uint64_t size, uint32_t alignment = 256);
        void UploadToBuffer(BufferHandle destBuffer, const void* data, uint64_t size,
            uint64_t offset = 0);
        void BeginFrame();
        void EndFrame();

        // Memory management
        void TrimMemory();
        void FlushPendingUploads();

        // Statistics and debugging
        BufferPoolStats GetGlobalStatistics() const;
        BufferPoolStats GetPoolStatistics() const;
        void LogStatistics() const;
        void LogDetailedStatistics() const;

        // Configuration
        void UpdatePoolConfiguration(const BufferPoolConfig& config);
        const BufferPoolConfig& GetPoolConfiguration() const { return poolConfig_; }

        // Handle management
        void AddBufferRef(BufferHandle handle);
        void ReleaseBufferRef(BufferHandle handle);

    private:
        // Internal helpers
        bool CreateStagingBuffers();
        void DestroyStagingBuffers();
        D3D12StagingBuffer* GetCurrentStagingBuffer();
        void AdvanceStagingBuffer();

        // Statistics helpers
        void UpdateGlobalStats(const BufferPoolStats& poolStats) const;
        void LogMemoryUsage() const;

        // Validation
        bool ValidateBufferDesc(const BufferDesc& desc) const;
    };

} // namespace Akhanda::RHI::D3D12
