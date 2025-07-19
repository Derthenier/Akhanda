// D3D12DescriptorHeap.hpp
// Akhanda Game Engine - D3D12 Descriptor Heap Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <bitset>
#include <unordered_map>

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

    // ========================================================================
    // Descriptor Handle Management
    // ========================================================================

    struct DescriptorHandle {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
        uint32_t index = UINT32_MAX;
        bool isValid = false;
        bool isShaderVisible = false;

        constexpr DescriptorHandle() = default;
        constexpr DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpu,
            D3D12_GPU_DESCRIPTOR_HANDLE gpu,
            uint32_t idx, bool shaderVisible)
            : cpuHandle(cpu), gpuHandle(gpu), index(idx), isValid(true), isShaderVisible(shaderVisible) {
        }

        void Invalidate() {
            cpuHandle = {};
            gpuHandle = {};
            index = UINT32_MAX;
            isValid = false;
            isShaderVisible = false;
        }
    };

    // ========================================================================
    // Descriptor Allocator
    // ========================================================================

    class DescriptorAllocator {
    private:
        struct FreeBlock {
            uint32_t offset = 0;
            uint32_t size = 0;

            constexpr FreeBlock() = default;
            constexpr FreeBlock(uint32_t off, uint32_t sz) : offset(off), size(sz) {}

            bool operator<(const FreeBlock& other) const {
                return size < other.size; // For priority queue (largest first)
            }
        };

        // Free list management
        std::priority_queue<FreeBlock> freeBlocks_;
        std::vector<bool> usedSlots_;
        uint32_t totalDescriptors_;
        std::atomic<uint32_t> allocatedCount_{ 0 };
        std::atomic<uint32_t> freeCount_{ 0 };

        // Thread safety
        mutable std::mutex allocationMutex_;

        // Statistics
        struct Stats {
            std::atomic<uint32_t> totalAllocations{ 0 };
            std::atomic<uint32_t> totalDeallocations{ 0 };
            std::atomic<uint32_t> fragmentationEvents{ 0 };
            std::atomic<uint32_t> compactionEvents{ 0 };
        };
        mutable Stats stats_;

    public:
        explicit DescriptorAllocator(uint32_t maxDescriptors);
        ~DescriptorAllocator() = default;

        // Non-copyable, non-movable
        DescriptorAllocator(const DescriptorAllocator&) = delete;
        DescriptorAllocator& operator=(const DescriptorAllocator&) = delete;
        DescriptorAllocator(DescriptorAllocator&&) = delete;
        DescriptorAllocator& operator=(DescriptorAllocator&&) = delete;

        // Allocation interface
        uint32_t Allocate(uint32_t count = 1);
        void Deallocate(uint32_t index, uint32_t count = 1);
        bool IsAllocated(uint32_t index) const;

        // Batch operations
        std::vector<uint32_t> AllocateBatch(uint32_t count);
        void DeallocateBatch(const std::vector<uint32_t>& indices);

        // Management
        void Reset();
        void Compact();

        // Statistics
        uint32_t GetAllocatedCount() const { return allocatedCount_.load(); }
        uint32_t GetFreeCount() const { return freeCount_.load(); }
        uint32_t GetTotalDescriptors() const { return totalDescriptors_; }
        float GetFragmentation() const;
        const Stats& GetStats() const { return stats_; }

        // Debug
        void ValidateIntegrity() const;
        std::string GetDebugInfo() const;

    private:
        void AddFreeBlock(uint32_t offset, uint32_t size);
        void RemoveFreeBlock(uint32_t offset, uint32_t size);
        void UpdateStats(bool isAllocation, uint32_t count = 1);
    };

    // ========================================================================
    // D3D12 Descriptor Heap Implementation
    // ========================================================================

    class D3D12DescriptorHeap : public IDescriptorHeap {
    private:
        // Core D3D12 objects
        ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
        ID3D12Device* device_ = nullptr;

        // Heap properties
        D3D12_DESCRIPTOR_HEAP_TYPE heapType_;
        D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags_;
        uint32_t descriptorCount_;
        uint32_t descriptorSize_;
        bool isShaderVisible_;

        // Memory management
        std::unique_ptr<DescriptorAllocator> allocator_;

        // Handle management
        D3D12_CPU_DESCRIPTOR_HANDLE baseCpuHandle_;
        D3D12_GPU_DESCRIPTOR_HANDLE baseGpuHandle_;

        // Descriptor tracking
        struct DescriptorInfo {
            D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            bool isAllocated = false;
            std::string debugName;
            uint64_t creationFrame = 0;
            void* associatedResource = nullptr; // Weak reference
        };
        std::vector<DescriptorInfo> descriptorInfos_;

        // Statistics
        struct HeapStats {
            std::atomic<uint32_t> totalAllocations{ 0 };
            std::atomic<uint32_t> activeCBV{ 0 };
            std::atomic<uint32_t> activeSRV{ 0 };
            std::atomic<uint32_t> activeUAV{ 0 };
            std::atomic<uint32_t> activeRTV{ 0 };
            std::atomic<uint32_t> activeDSV{ 0 };
            std::atomic<uint32_t> activeSampler{ 0 };
        };
        mutable HeapStats stats_;

        // Thread safety
        mutable std::mutex heapMutex_;

        // Debug information
        std::string debugName_;
        std::atomic<uint64_t> currentFrame_{ 0 };

        // Logging
        Logging::LogChannel& logChannel_;

    public:
        D3D12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible = false);
        virtual ~D3D12DescriptorHeap();

        // IDescriptorHeap implementation
        bool Initialize(uint32_t descriptorCount, bool shaderVisible) override;
        void Shutdown() override;

        uint32_t AllocateDescriptor() override;
        void FreeDescriptor(uint32_t index) override;
        void Reset() override;

        void CreateConstantBufferView(uint32_t descriptorIndex,
            BufferHandle buffer) override;
        void CreateShaderResourceView(uint32_t descriptorIndex,
            TextureHandle texture) override;
        void CreateUnorderedAccessView(uint32_t descriptorIndex,
            TextureHandle texture) override;
        void CreateRenderTargetView(uint32_t descriptorIndex,
            TextureHandle texture) override;
        void CreateDepthStencilView(uint32_t descriptorIndex,
            TextureHandle texture) override;
        void CreateSampler(uint32_t descriptorIndex,
            const SamplerDesc& desc) override;

        uint32_t GetDescriptorCount() const override { return descriptorCount_; }
        uint32_t GetFreeDescriptorCount() const override;
        bool IsShaderVisible() const override { return isShaderVisible_; }
        uint64_t GetGPUAddress(uint32_t descriptorIndex) const override;

        void SetDebugName(const char* name) override;
        const char* GetDebugName() const override { return debugName_.c_str(); }

        // D3D12-specific methods
        bool InitializeWithDevice(ID3D12Device* device, uint32_t descriptorCount,
            D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible);

        ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const { return descriptorHeap_.Get(); }
        D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return heapType_; }
        uint32_t GetDescriptorSize() const { return descriptorSize_; }

        // Handle generation
        DescriptorHandle GetDescriptorHandle(uint32_t index) const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index) const;

        // Batch operations
        std::vector<uint32_t> AllocateDescriptors(uint32_t count);
        void FreeDescriptors(const std::vector<uint32_t>& indices);

        // Descriptor copying
        void CopyDescriptor(uint32_t destIndex, const D3D12DescriptorHeap* srcHeap, uint32_t srcIndex);
        void CopyDescriptors(uint32_t destStartIndex, const D3D12DescriptorHeap* srcHeap,
            uint32_t srcStartIndex, uint32_t count);

        // Advanced view creation
        void CreateConstantBufferView(uint32_t descriptorIndex,
            const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc);
        void CreateShaderResourceView(uint32_t descriptorIndex,
            ID3D12Resource* resource,
            const D3D12_SHADER_RESOURCE_VIEW_DESC* desc = nullptr);
        void CreateUnorderedAccessView(uint32_t descriptorIndex,
            ID3D12Resource* resource,
            ID3D12Resource* counterResource = nullptr,
            const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc = nullptr);
        void CreateRenderTargetView(uint32_t descriptorIndex,
            ID3D12Resource* resource,
            const D3D12_RENDER_TARGET_VIEW_DESC* desc = nullptr);
        void CreateDepthStencilView(uint32_t descriptorIndex,
            ID3D12Resource* resource,
            const D3D12_DEPTH_STENCIL_VIEW_DESC* desc = nullptr);
        void CreateSampler(uint32_t descriptorIndex,
            const D3D12_SAMPLER_DESC& desc);

        // Frame management
        void BeginFrame(uint64_t frameNumber);
        void EndFrame();

        // Statistics and debugging
        const HeapStats& GetStats() const { return stats_; }
        void ResetStats();
        float GetUtilization() const;
        std::string GetDebugInfo() const;
        void LogHeapInfo() const;

        // Validation
        bool ValidateDescriptor(uint32_t index) const;
        void ValidateHeapIntegrity() const;

    private:
        // Initialization helpers
        bool CreateDescriptorHeap();
        void InitializeHandles();
        void SetupDebugInfo();

        // Resource access helpers
        D3D12Buffer* GetD3D12Buffer(BufferHandle handle) const;
        D3D12Texture* GetD3D12Texture(TextureHandle handle) const;

        // View creation helpers
        D3D12_CONSTANT_BUFFER_VIEW_DESC CreateCBVDesc(const D3D12Buffer* buffer) const;
        D3D12_SHADER_RESOURCE_VIEW_DESC CreateSRVDesc(const D3D12Texture* texture) const;
        D3D12_UNORDERED_ACCESS_VIEW_DESC CreateUAVDesc(const D3D12Texture* texture) const;
        D3D12_RENDER_TARGET_VIEW_DESC CreateRTVDesc(const D3D12Texture* texture) const;
        D3D12_DEPTH_STENCIL_VIEW_DESC CreateDSVDesc(const D3D12Texture* texture) const;
        D3D12_SAMPLER_DESC CreateSamplerDesc(const SamplerDesc& desc) const;

        // Format conversion
        DXGI_FORMAT ConvertFormat(Format format) const;
        D3D12_FILTER ConvertFilter(Filter filter) const;
        D3D12_TEXTURE_ADDRESS_MODE ConvertAddressMode(AddressMode mode) const;
        D3D12_COMPARISON_FUNC ConvertComparisonFunc(CompareFunction func) const;

        // Statistics helpers
        void UpdateAllocationStats(D3D12_DESCRIPTOR_HEAP_TYPE type, bool isAllocation);
        void UpdateDescriptorInfo(uint32_t index, bool isAllocated, const char* debugName = nullptr);

        // Validation helpers
        bool IsValidDescriptorIndex(uint32_t index) const;
        bool IsValidDescriptorType(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

        // Debug helpers
        void LogDescriptorCreation(uint32_t index, const char* type, const char* name) const;
        void LogDescriptorFree(uint32_t index) const;
        const char* GetDescriptorTypeName(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

        // Cleanup helpers
        void CleanupDescriptors();
        void CleanupHeap();
    };

    // ========================================================================
    // Descriptor Heap Manager
    // ========================================================================

    class D3D12DescriptorHeapManager {
    private:
        ID3D12Device* device_ = nullptr;

        // Descriptor heaps by type
        std::unique_ptr<D3D12DescriptorHeap> rtvHeap_;
        std::unique_ptr<D3D12DescriptorHeap> dsvHeap_;
        std::unique_ptr<D3D12DescriptorHeap> cbvSrvUavHeap_;
        std::unique_ptr<D3D12DescriptorHeap> samplerHeap_;

        // Shader-visible heaps
        std::unique_ptr<D3D12DescriptorHeap> shaderVisibleCbvSrvUavHeap_;
        std::unique_ptr<D3D12DescriptorHeap> shaderVisibleSamplerHeap_;

        // Configuration
        struct HeapSizes {
            uint32_t rtvHeapSize = 1000;
            uint32_t dsvHeapSize = 100;
            uint32_t cbvSrvUavHeapSize = 10000;
            uint32_t samplerHeapSize = 100;
            uint32_t shaderVisibleCbvSrvUavHeapSize = 1000;
            uint32_t shaderVisibleSamplerHeapSize = 16;
        };
        HeapSizes heapSizes_;

        // Statistics
        struct ManagerStats {
            std::atomic<uint32_t> totalHeaps{ 0 };
            std::atomic<uint32_t> totalDescriptors{ 0 };
            std::atomic<uint64_t> totalMemoryUsed{ 0 };
        };
        mutable ManagerStats stats_;

        Logging::LogChannel& logChannel_;

    public:
        D3D12DescriptorHeapManager();
        ~D3D12DescriptorHeapManager();

        bool Initialize(ID3D12Device* device, const HeapSizes& sizes = {});
        void Shutdown();

        // Heap access
        D3D12DescriptorHeap* GetRTVHeap() const { return rtvHeap_.get(); }
        D3D12DescriptorHeap* GetDSVHeap() const { return dsvHeap_.get(); }
        D3D12DescriptorHeap* GetCBVSRVUAVHeap() const { return cbvSrvUavHeap_.get(); }
        D3D12DescriptorHeap* GetSamplerHeap() const { return samplerHeap_.get(); }
        D3D12DescriptorHeap* GetShaderVisibleCBVSRVUAVHeap() const { return shaderVisibleCbvSrvUavHeap_.get(); }
        D3D12DescriptorHeap* GetShaderVisibleSamplerHeap() const { return shaderVisibleSamplerHeap_.get(); }

        // Convenience allocation
        uint32_t AllocateRTV() { return rtvHeap_->AllocateDescriptor(); }
        uint32_t AllocateDSV() { return dsvHeap_->AllocateDescriptor(); }
        uint32_t AllocateCBVSRVUAV(bool shaderVisible = false);
        uint32_t AllocateSampler(bool shaderVisible = false);

        // Frame management
        void BeginFrame(uint64_t frameNumber);
        void EndFrame();

        // Statistics
        const ManagerStats& GetStats() const { return stats_; }
        void LogAllHeapInfo() const;

        // Configuration
        void SetHeapSizes(const HeapSizes& sizes) { heapSizes_ = sizes; }
        const HeapSizes& GetHeapSizes() const { return heapSizes_; }

    private:
        bool CreateHeap(std::unique_ptr<D3D12DescriptorHeap>& heap,
            D3D12_DESCRIPTOR_HEAP_TYPE type,
            uint32_t descriptorCount,
            bool shaderVisible,
            const char* debugName);
        void UpdateManagerStats();
    };

    // ========================================================================
    // Descriptor Heap Utilities
    // ========================================================================

    namespace DescriptorHeapUtils {

        // Type utilities
        const char* GetDescriptorTypeName(D3D12_DESCRIPTOR_HEAP_TYPE type);
        uint32_t GetDescriptorSize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type);
        bool IsShaderVisibleType(D3D12_DESCRIPTOR_HEAP_TYPE type);

        // Validation utilities
        bool IsValidDescriptorHeapType(D3D12_DESCRIPTOR_HEAP_TYPE type);
        bool IsValidDescriptorCount(uint32_t count, D3D12_DESCRIPTOR_HEAP_TYPE type);
        bool IsValidDescriptorIndex(uint32_t index, uint32_t maxCount);

        // Memory calculations
        uint64_t CalculateHeapMemoryUsage(uint32_t descriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE type);
        uint32_t CalculateOptimalHeapSize(uint32_t estimatedUsage, D3D12_DESCRIPTOR_HEAP_TYPE type);

        // Debug utilities
        std::string FormatDescriptorHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& handle);
        std::string FormatDescriptorHandle(const D3D12_GPU_DESCRIPTOR_HANDLE& handle);
        void LogDescriptorHeapInfo(const D3D12DescriptorHeap* heap);

    } // namespace DescriptorHeapUtils

} // namespace Akhanda::RHI::D3D12
