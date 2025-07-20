// Engine/Renderer/RHI/D3D12/D3D12DescriptorHeap.cpp
// Akhanda Game Engine - D3D12 Device Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#include "D3D12DescriptorHeap.hpp"

#include <format>
#include <sstream>

#undef max
#undef min

namespace Akhanda::RHI::D3D12 {

    // ========================================================================
    // DescriptorAllocator Implementation
    // ========================================================================

    DescriptorAllocator::DescriptorAllocator(uint32_t maxDescriptors)
        : totalDescriptors_(maxDescriptors)
        , usedSlots_(maxDescriptors, false) {

        // Initialize with one large free block covering entire range
        freeBlocks_.emplace(0, maxDescriptors);
        freeCount_.store(maxDescriptors);
        allocatedCount_.store(0);
    }

    uint32_t DescriptorAllocator::Allocate(uint32_t count) {
        if (count == 0) {
            return UINT32_MAX;
        }

        std::lock_guard<std::mutex> lock(allocationMutex_);

        // Find best-fit block (largest available that fits our request)
        std::priority_queue<FreeBlock> tempBlocks;
        uint32_t bestOffset = UINT32_MAX;
        bool foundBlock = false;

        while (!freeBlocks_.empty()) {
            FreeBlock block = freeBlocks_.top();
            freeBlocks_.pop();

            if (block.size >= count) {
                // Found a suitable block
                bestOffset = block.offset;
                foundBlock = true;

                // If block is larger than needed, add remainder back to free list
                if (block.size > count) {
                    FreeBlock remainder(block.offset + count, block.size - count);
                    tempBlocks.push(remainder);
                }
                break;
            }
            else {
                // Block too small, save for later
                tempBlocks.push(block);
            }
        }

        // Restore unused blocks to free list
        while (!tempBlocks.empty()) {
            freeBlocks_.push(tempBlocks.top());
            tempBlocks.pop();
        }

        if (!foundBlock) {
            return UINT32_MAX; // No suitable block found
        }

        // Mark slots as used
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t index = bestOffset + i;
            if (index >= totalDescriptors_ || usedSlots_[index]) {
                // This should never happen with correct logic
                // Rollback any changes made so far
                for (uint32_t j = 0; j < i; ++j) {
                    usedSlots_[bestOffset + j] = false;
                }
                return UINT32_MAX;
            }
            usedSlots_[index] = true;
        }

        // Update counters
        allocatedCount_.fetch_add(count);
        freeCount_.fetch_sub(count);
        UpdateStats(true, count);

        return bestOffset;
    }

    void DescriptorAllocator::Deallocate(uint32_t index, uint32_t count) {
        if (count == 0 || index >= totalDescriptors_ || index + count > totalDescriptors_) {
            return;
        }

        std::lock_guard<std::mutex> lock(allocationMutex_);

        // Validate that all slots in range are actually allocated
        for (uint32_t i = 0; i < count; ++i) {
            if (!usedSlots_[index + i]) {
                return; // Invalid deallocation attempt
            }
        }

        // Mark slots as free
        for (uint32_t i = 0; i < count; ++i) {
            usedSlots_[index + i] = false;
        }

        // Update counters
        allocatedCount_.fetch_sub(count);
        freeCount_.fetch_add(count);
        UpdateStats(false, count);

        // Add free block and attempt coalescing
        AddFreeBlock(index, count);
    }

    bool DescriptorAllocator::IsAllocated(uint32_t index) const {
        if (index >= totalDescriptors_) {
            return false;
        }

        std::lock_guard<std::mutex> lock(allocationMutex_);
        return usedSlots_[index];
    }

    std::vector<uint32_t> DescriptorAllocator::AllocateBatch(uint32_t count) {
        std::vector<uint32_t> indices;
        indices.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t index = Allocate(1);
            if (index == UINT32_MAX) {
                // Failed to allocate, rollback previous allocations
                for (uint32_t rollbackIndex : indices) {
                    Deallocate(rollbackIndex, 1);
                }
                return {}; // Return empty vector on failure
            }
            indices.push_back(index);
        }

        return indices;
    }

    void DescriptorAllocator::DeallocateBatch(const std::vector<uint32_t>& indices) {
        for (uint32_t index : indices) {
            Deallocate(index, 1);
        }
    }

    void DescriptorAllocator::Reset() {
        std::lock_guard<std::mutex> lock(allocationMutex_);

        // Clear all data structures
        while (!freeBlocks_.empty()) {
            freeBlocks_.pop();
        }

        std::fill(usedSlots_.begin(), usedSlots_.end(), false);

        // Reset to initial state
        freeBlocks_.emplace(0, totalDescriptors_);
        allocatedCount_.store(0);
        freeCount_.store(totalDescriptors_);

        // Reset statistics
        stats_.totalAllocations.store(0);
        stats_.totalDeallocations.store(0);
        stats_.fragmentationEvents.store(0);
        stats_.compactionEvents.store(0);
    }

    void DescriptorAllocator::Compact() {
        std::lock_guard<std::mutex> lock(allocationMutex_);

        // Rebuild free list by scanning for contiguous free regions
        std::priority_queue<FreeBlock> newFreeBlocks;

        uint32_t blockStart = UINT32_MAX;
        uint32_t blockSize = 0;

        for (uint32_t i = 0; i <= totalDescriptors_; ++i) {
            bool isSlotFree = (i < totalDescriptors_) ? !usedSlots_[i] : false;

            if (isSlotFree) {
                if (blockStart == UINT32_MAX) {
                    blockStart = i;
                    blockSize = 1;
                }
                else {
                    blockSize++;
                }
            }
            else {
                if (blockStart != UINT32_MAX) {
                    // End of free block, add it
                    newFreeBlocks.emplace(blockStart, blockSize);
                    blockStart = UINT32_MAX;
                    blockSize = 0;
                }
            }
        }

        // Replace free blocks with compacted version
        freeBlocks_ = std::move(newFreeBlocks);
        stats_.compactionEvents.fetch_add(1);
    }

    float DescriptorAllocator::GetFragmentation() const {
        std::lock_guard<std::mutex> lock(allocationMutex_);

        if (freeCount_.load() == 0) {
            return 0.0f; // No free space means no fragmentation
        }

        // Create a temporary copy to inspect free blocks
        auto tempBlocks = freeBlocks_;
        uint32_t numFreeBlocks = 0;
        uint32_t largestFreeBlock = 0;

        while (!tempBlocks.empty()) {
            FreeBlock block = tempBlocks.top();
            tempBlocks.pop();
            numFreeBlocks++;
            largestFreeBlock = std::max(largestFreeBlock, block.size);
        }

        if (numFreeBlocks <= 1) {
            return 0.0f; // No fragmentation with 0 or 1 free blocks
        }

        // Fragmentation metric: (total_free - largest_free) / total_free
        uint32_t totalFree = freeCount_.load();
        return static_cast<float>(totalFree - largestFreeBlock) / static_cast<float>(totalFree);
    }

    void DescriptorAllocator::ValidateIntegrity() const {
        std::lock_guard<std::mutex> lock(allocationMutex_);

        // Verify that allocated + free == total
        uint32_t allocated = allocatedCount_.load();
        uint32_t free = freeCount_.load();

        if (allocated + free != totalDescriptors_) {
            throw std::runtime_error(std::format(
                "DescriptorAllocator integrity violation: allocated({}) + free({}) != total({})",
                allocated, free, totalDescriptors_));
        }

        // Count actual allocated slots
        uint32_t actualAllocated = 0;
        for (bool used : usedSlots_) {
            if (used) {
                actualAllocated++;
            }
        }

        if (actualAllocated != allocated) {
            throw std::runtime_error(std::format(
                "DescriptorAllocator integrity violation: actual allocated({}) != tracked({})",
                actualAllocated, allocated));
        }

        // Verify free blocks don't overlap with allocated slots
        auto tempBlocks = freeBlocks_;
        while (!tempBlocks.empty()) {
            FreeBlock block = tempBlocks.top();
            tempBlocks.pop();

            for (uint32_t i = 0; i < block.size; ++i) {
                uint32_t index = block.offset + i;
                if (index >= totalDescriptors_ || usedSlots_[index]) {
                    throw std::runtime_error(std::format(
                        "DescriptorAllocator integrity violation: free block overlaps allocated slot at index {}",
                        index));
                }
            }
        }
    }

    std::string DescriptorAllocator::GetDebugInfo() const {
        std::lock_guard<std::mutex> lock(allocationMutex_);

        auto tempBlocks = freeBlocks_;
        std::string info;
        info.reserve(1024);

        info += std::format("DescriptorAllocator Debug Info:\n");
        info += std::format("  Total Descriptors: {}\n", totalDescriptors_);
        info += std::format("  Allocated: {}\n", allocatedCount_.load());
        info += std::format("  Free: {}\n", freeCount_.load());
        info += std::format("  Fragmentation: {:.2f}%\n", GetFragmentation() * 100.0f);

        info += std::format("  Statistics:\n");
        info += std::format("    Total Allocations: {}\n", stats_.totalAllocations.load());
        info += std::format("    Total Deallocations: {}\n", stats_.totalDeallocations.load());
        info += std::format("    Fragmentation Events: {}\n", stats_.fragmentationEvents.load());
        info += std::format("    Compaction Events: {}\n", stats_.compactionEvents.load());

        info += std::format("  Free Blocks:\n");
        uint32_t blockIndex = 0;
        while (!tempBlocks.empty() && blockIndex < 10) { // Limit to first 10 blocks
            FreeBlock block = tempBlocks.top();
            tempBlocks.pop();
            info += std::format("    Block {}: offset={}, size={}\n", blockIndex++, block.offset, block.size);
        }

        if (!tempBlocks.empty()) {
            info += std::format("    ... and {} more blocks\n", tempBlocks.size());
        }

        return info;
    }

    void DescriptorAllocator::AddFreeBlock(uint32_t offset, uint32_t size) {
        // This method assumes the mutex is already locked by the caller

        // Attempt to coalesce with existing free blocks
        std::priority_queue<FreeBlock> tempBlocks;
        uint32_t newOffset = offset;
        uint32_t newSize = size;
        bool coalesced = false;

        while (!freeBlocks_.empty()) {
            FreeBlock block = freeBlocks_.top();
            freeBlocks_.pop();

            // Check if blocks can be coalesced
            if (block.offset + block.size == newOffset) {
                // Block is immediately before new block
                newOffset = block.offset;
                newSize += block.size;
                coalesced = true;
            }
            else if (newOffset + newSize == block.offset) {
                // Block is immediately after new block
                newSize += block.size;
                coalesced = true;
            }
            else {
                // No coalescing possible, save block
                tempBlocks.push(block);
            }
        }

        // Add the (possibly coalesced) block
        tempBlocks.emplace(newOffset, newSize);

        // Restore all blocks to free list
        freeBlocks_ = std::move(tempBlocks);

        if (coalesced) {
            stats_.fragmentationEvents.fetch_add(1);
        }
    }

    void DescriptorAllocator::RemoveFreeBlock([[maybe_unused]] uint32_t offset, [[maybe_unused]] uint32_t size) {
        // This method is currently unused in this implementation
        // Could be used for more advanced allocation strategies
        // Left as placeholder for future enhancements
    }

    void DescriptorAllocator::UpdateStats(bool isAllocation, uint32_t count) {
        if (isAllocation) {
            stats_.totalAllocations.fetch_add(count);
        }
        else {
            stats_.totalDeallocations.fetch_add(count);
        }
    }

    // ========================================================================
    // D3D12DescriptorHeap Implementation
    // ========================================================================

    D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible)
        : heapType_(type)
        , isShaderVisible_(shaderVisible)
        , descriptorCount_(0)
        , descriptorSize_(0)
        , device_(nullptr)
        , heapFlags_(shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE)
        , baseCpuHandle_{}
        , baseGpuHandle_{}
        , currentFrame_(0)
        , logChannel_(Logging::LogManager::Instance().GetChannel("D3D12DescriptorHeap")) {
    }

    D3D12DescriptorHeap::~D3D12DescriptorHeap() {
        Shutdown();
    }

    bool D3D12DescriptorHeap::Initialize(uint32_t descriptorCount, bool shaderVisible) {
        if (descriptorCount == 0) {
            logChannel_.Log(Logging::LogLevel::Error, "Cannot initialize descriptor heap with zero descriptors");
            return false;
        }

        if (device_ == nullptr) {
            logChannel_.Log(Logging::LogLevel::Error, "Device must be set before initializing descriptor heap");
            return false;
        }

        descriptorCount_ = descriptorCount;
        isShaderVisible_ = shaderVisible;
        heapFlags_ = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        // Create allocator
        allocator_ = std::make_unique<DescriptorAllocator>(descriptorCount);
        if (!allocator_) {
            logChannel_.Log(Logging::LogLevel::Error, "Failed to create descriptor allocator");
            return false;
        }

        // Initialize descriptor tracking
        descriptorInfos_.resize(descriptorCount);

        // Create D3D12 descriptor heap
        if (!CreateDescriptorHeap()) {
            logChannel_.Log(Logging::LogLevel::Error, "Failed to create D3D12 descriptor heap");
            return false;
        }

        // Initialize handles
        InitializeHandles();

        // Setup debug information
        SetupDebugInfo();

        logChannel_.LogFormat(Logging::LogLevel::Info,
            "D3D12DescriptorHeap initialized: type={}, count={}, shaderVisible={}, size={}",
            GetDescriptorTypeName(heapType_), descriptorCount_, isShaderVisible_, descriptorSize_);

        return true;
    }

    bool D3D12DescriptorHeap::InitializeWithDevice(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type,
        uint32_t descriptorCount, bool shaderVisible) {
        if (!device) {
            logChannel_.Log(Logging::LogLevel::Error, "Invalid device pointer");
            return false;
        }

        device_ = device;
        heapType_ = type;
        return Initialize(descriptorCount, shaderVisible);
    }

    void D3D12DescriptorHeap::Shutdown() {
        if (descriptorHeap_) {
            logChannel_.LogFormat(Logging::LogLevel::Debug,
                "Shutting down descriptor heap: {} active descriptors",
                stats_.totalAllocations.load());
        }

        CleanupDescriptors();
        CleanupHeap();

        allocator_.reset();
        descriptorInfos_.clear();
        device_ = nullptr;
    }

    uint32_t D3D12DescriptorHeap::AllocateDescriptor() {
        if (!allocator_) {
            return UINT32_MAX;
        }

        uint32_t index = allocator_->Allocate(1);
        if (index != UINT32_MAX) {
            UpdateDescriptorInfo(index, true);
            LogDescriptorAllocation(index);
        }

        return index;
    }

    void D3D12DescriptorHeap::FreeDescriptor(uint32_t index) {
        if (!allocator_ || !IsValidDescriptorIndex(index)) {
            return;
        }

        std::lock_guard<std::mutex> lock(heapMutex_);

        // Clear descriptor info
        UpdateDescriptorInfo(index, false);
        LogDescriptorFree(index);

        // Free from allocator
        allocator_->Deallocate(index, 1);
    }

    void D3D12DescriptorHeap::Reset() {
        std::lock_guard<std::mutex> lock(heapMutex_);

        if (allocator_) {
            allocator_->Reset();
        }

        // Clear all descriptor info
        for (auto& info : descriptorInfos_) {
            info = DescriptorInfo{};
        }

        // Reset statistics
        stats_.totalAllocations.store(0);
        stats_.activeCBV.store(0);
        stats_.activeSRV.store(0);
        stats_.activeUAV.store(0);
        stats_.activeRTV.store(0);
        stats_.activeDSV.store(0);
        stats_.activeSampler.store(0);

        logChannel_.Log(Logging::LogLevel::Debug, "Descriptor heap reset completed");
    }

    void D3D12DescriptorHeap::CreateConstantBufferView(uint32_t descriptorIndex, BufferHandle buffer) {
        if (!IsValidDescriptorIndex(descriptorIndex)) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Invalid descriptor index {} for CBV creation", descriptorIndex);
            return;
        }

        D3D12Buffer* d3dBuffer = GetD3D12Buffer(buffer);
        if (!d3dBuffer) {
            logChannel_.Log(Logging::LogLevel::Error, "Invalid buffer handle for CBV creation");
            return;
        }

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = CreateCBVDesc(d3dBuffer);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUHandle(descriptorIndex);

        device_->CreateConstantBufferView(&cbvDesc, handle);

        UpdateAllocationStats(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
        stats_.activeCBV.fetch_add(1);
        UpdateDescriptorInfo(descriptorIndex, true, "CBV");

        LogDescriptorCreation(descriptorIndex, "CBV", "ConstantBuffer");
    }

    void D3D12DescriptorHeap::CreateShaderResourceView(uint32_t descriptorIndex, TextureHandle texture) {
        if (!IsValidDescriptorIndex(descriptorIndex)) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Invalid descriptor index {} for SRV creation", descriptorIndex);
            return;
        }

        D3D12Texture* d3dTexture = GetD3D12Texture(texture);
        if (!d3dTexture) {
            logChannel_.Log(Logging::LogLevel::Error, "Invalid texture handle for SRV creation");
            return;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = CreateSRVDesc(d3dTexture);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUHandle(descriptorIndex);

        // Get the D3D12 resource from texture
        ID3D12Resource* resource = nullptr; // d3dTexture->GetD3D12Resource();
        device_->CreateShaderResourceView(resource, &srvDesc, handle);

        UpdateAllocationStats(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
        stats_.activeSRV.fetch_add(1);
        UpdateDescriptorInfo(descriptorIndex, true, "SRV");

        LogDescriptorCreation(descriptorIndex, "SRV", "ShaderResource");
    }

    void D3D12DescriptorHeap::CreateUnorderedAccessView(uint32_t descriptorIndex, TextureHandle texture) {
        if (!IsValidDescriptorIndex(descriptorIndex)) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Invalid descriptor index {} for UAV creation", descriptorIndex);
            return;
        }

        D3D12Texture* d3dTexture = GetD3D12Texture(texture);
        if (!d3dTexture) {
            logChannel_.Log(Logging::LogLevel::Error, "Invalid texture handle for UAV creation");
            return;
        }

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = CreateUAVDesc(d3dTexture);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUHandle(descriptorIndex);

        // Get the D3D12 resource from texture
        ID3D12Resource* resource = nullptr; // d3dTexture->GetD3D12Resource();
        device_->CreateUnorderedAccessView(resource, nullptr, &uavDesc, handle);

        UpdateAllocationStats(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
        stats_.activeUAV.fetch_add(1);
        UpdateDescriptorInfo(descriptorIndex, true, "UAV");

        LogDescriptorCreation(descriptorIndex, "UAV", "UnorderedAccess");
    }

    void D3D12DescriptorHeap::CreateRenderTargetView(uint32_t descriptorIndex, TextureHandle texture) {
        if (!IsValidDescriptorIndex(descriptorIndex)) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Invalid descriptor index {} for RTV creation", descriptorIndex);
            return;
        }

        D3D12Texture* d3dTexture = GetD3D12Texture(texture);
        if (!d3dTexture) {
            logChannel_.Log(Logging::LogLevel::Error, "Invalid texture handle for RTV creation");
            return;
        }

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = CreateRTVDesc(d3dTexture);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUHandle(descriptorIndex);

        // Get the D3D12 resource from texture
        ID3D12Resource* resource = nullptr; // d3dTexture->GetD3D12Resource();
        device_->CreateRenderTargetView(resource, &rtvDesc, handle);

        UpdateAllocationStats(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, true);
        stats_.activeRTV.fetch_add(1);
        UpdateDescriptorInfo(descriptorIndex, true, "RTV");

        LogDescriptorCreation(descriptorIndex, "RTV", "RenderTarget");
    }

    void D3D12DescriptorHeap::CreateDepthStencilView(uint32_t descriptorIndex, TextureHandle texture) {
        if (!IsValidDescriptorIndex(descriptorIndex)) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Invalid descriptor index {} for DSV creation", descriptorIndex);
            return;
        }

        D3D12Texture* d3dTexture = GetD3D12Texture(texture);
        if (!d3dTexture) {
            logChannel_.Log(Logging::LogLevel::Error, "Invalid texture handle for DSV creation");
            return;
        }

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = CreateDSVDesc(d3dTexture);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUHandle(descriptorIndex);

        // Get the D3D12 resource from texture
        ID3D12Resource* resource = nullptr; // d3dTexture->GetD3D12Resource();
        device_->CreateDepthStencilView(resource, &dsvDesc, handle);

        UpdateAllocationStats(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, true);
        stats_.activeDSV.fetch_add(1);
        UpdateDescriptorInfo(descriptorIndex, true, "DSV");

        LogDescriptorCreation(descriptorIndex, "DSV", "DepthStencil");
    }

    void D3D12DescriptorHeap::CreateSampler(uint32_t descriptorIndex, const SamplerDesc& desc) {
        if (!IsValidDescriptorIndex(descriptorIndex)) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Invalid descriptor index {} for Sampler creation", descriptorIndex);
            return;
        }

        D3D12_SAMPLER_DESC samplerDesc = CreateSamplerDesc(desc);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUHandle(descriptorIndex);

        device_->CreateSampler(&samplerDesc, handle);

        UpdateAllocationStats(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, true);
        stats_.activeSampler.fetch_add(1);
        UpdateDescriptorInfo(descriptorIndex, true, "Sampler");

        LogDescriptorCreation(descriptorIndex, "Sampler", "Sampler");
    }

    uint32_t D3D12DescriptorHeap::GetFreeDescriptorCount() const {
        return allocator_ ? allocator_->GetFreeCount() : 0;
    }

    uint64_t D3D12DescriptorHeap::GetGPUAddress(uint32_t descriptorIndex) const {
        if (!isShaderVisible_ || !IsValidDescriptorIndex(descriptorIndex)) {
            return 0;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE handle = GetGPUHandle(descriptorIndex);
        return handle.ptr;
    }

    void D3D12DescriptorHeap::SetDebugName(const char* name) {
        if (name) {
            debugName_ = name;
            if (descriptorHeap_) {
                std::wstring wideName(name, name + strlen(name));
                descriptorHeap_->SetName(wideName.c_str());
            }
        }
    }

    DescriptorHandle D3D12DescriptorHeap::GetDescriptorHandle(uint32_t index) const {
        if (!IsValidDescriptorIndex(index)) {
            return DescriptorHandle{};
        }

        return DescriptorHandle{
            GetCPUHandle(index),
            GetGPUHandle(index),
            index,
            isShaderVisible_
        };
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCPUHandle(uint32_t index) const {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = baseCpuHandle_;
        handle.ptr += static_cast<SIZE_T>(index) * descriptorSize_;
        return handle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGPUHandle(uint32_t index) const {
        if (!isShaderVisible_) {
            return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
        }

        D3D12_GPU_DESCRIPTOR_HANDLE handle = baseGpuHandle_;
        handle.ptr += static_cast<UINT64>(index) * descriptorSize_;
        return handle;
    }

    std::vector<uint32_t> D3D12DescriptorHeap::AllocateDescriptors(uint32_t count) {
        if (!allocator_) {
            return {};
        }

        return allocator_->AllocateBatch(count);
    }

    void D3D12DescriptorHeap::FreeDescriptors(const std::vector<uint32_t>& indices) {
        if (!allocator_) {
            return;
        }

        for (uint32_t index : indices) {
            UpdateDescriptorInfo(index, false);
        }

        allocator_->DeallocateBatch(indices);
    }

    void D3D12DescriptorHeap::CopyDescriptor(uint32_t destIndex, const D3D12DescriptorHeap* srcHeap, uint32_t srcIndex) {
        if (!srcHeap || !IsValidDescriptorIndex(destIndex) || !srcHeap->IsValidDescriptorIndex(srcIndex)) {
            return;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE destHandle = GetCPUHandle(destIndex);
        D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = srcHeap->GetCPUHandle(srcIndex);

        device_->CopyDescriptorsSimple(1, destHandle, srcHandle, heapType_);
    }

    void D3D12DescriptorHeap::CopyDescriptors(uint32_t destStartIndex, const D3D12DescriptorHeap* srcHeap,
        uint32_t srcStartIndex, uint32_t count) {
        if (!srcHeap || count == 0) {
            return;
        }

        // Validate ranges
        if (destStartIndex + count > descriptorCount_ ||
            srcStartIndex + count > srcHeap->descriptorCount_) {
            return;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE destHandle = GetCPUHandle(destStartIndex);
        D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = srcHeap->GetCPUHandle(srcStartIndex);

        device_->CopyDescriptorsSimple(count, destHandle, srcHandle, heapType_);
    }

    void D3D12DescriptorHeap::BeginFrame(uint64_t frameNumber) {
        currentFrame_.store(frameNumber);
    }

    void D3D12DescriptorHeap::EndFrame() {
        // Frame-based cleanup or validation can be added here
    }

    void D3D12DescriptorHeap::ResetStats() {
        stats_.totalAllocations.store(0);
        stats_.activeCBV.store(0);
        stats_.activeSRV.store(0);
        stats_.activeUAV.store(0);
        stats_.activeRTV.store(0);
        stats_.activeDSV.store(0);
        stats_.activeSampler.store(0);

        if (allocator_) {
            // Reset allocator stats would be called here if exposed
        }
    }

    float D3D12DescriptorHeap::GetUtilization() const {
        if (descriptorCount_ == 0) {
            return 0.0f;
        }

        uint32_t allocated = allocator_ ? allocator_->GetAllocatedCount() : 0;
        return static_cast<float>(allocated) / static_cast<float>(descriptorCount_);
    }

    std::string D3D12DescriptorHeap::GetDebugInfo() const {
        std::string info;
        info.reserve(1024);

        info += std::format("D3D12DescriptorHeap Debug Info [{}]:\n", debugName_);
        info += std::format("  Type: {}\n", GetDescriptorTypeName(heapType_));
        info += std::format("  Descriptor Count: {}\n", descriptorCount_);
        info += std::format("  Descriptor Size: {}\n", descriptorSize_);
        info += std::format("  Shader Visible: {}\n", isShaderVisible_ ? "Yes" : "No");
        info += std::format("  Utilization: {:.2f}%\n", GetUtilization() * 100.0f);
        info += std::format("  Base CPU Handle: 0x{:016X}\n", baseCpuHandle_.ptr);

        if (isShaderVisible_) {
            info += std::format("  Base GPU Handle: 0x{:016X}\n", baseGpuHandle_.ptr);
        }

        info += std::format("  Active Descriptors:\n");
        info += std::format("    CBV: {}\n", stats_.activeCBV.load());
        info += std::format("    SRV: {}\n", stats_.activeSRV.load());
        info += std::format("    UAV: {}\n", stats_.activeUAV.load());
        info += std::format("    RTV: {}\n", stats_.activeRTV.load());
        info += std::format("    DSV: {}\n", stats_.activeDSV.load());
        info += std::format("    Sampler: {}\n", stats_.activeSampler.load());

        if (allocator_) {
            info += "\n";
            info += allocator_->GetDebugInfo();
        }

        return info;
    }

    void D3D12DescriptorHeap::LogHeapInfo() const {
        logChannel_.Log(Logging::LogLevel::Info, GetDebugInfo());
    }

    bool D3D12DescriptorHeap::ValidateDescriptor(uint32_t index) const {
        if (!IsValidDescriptorIndex(index)) {
            return false;
        }

        std::lock_guard<std::mutex> lock(heapMutex_);
        return descriptorInfos_[index].isAllocated;
    }

    void D3D12DescriptorHeap::ValidateHeapIntegrity() const {
        if (allocator_) {
            allocator_->ValidateIntegrity();
        }

        // Additional heap-specific validation
        if (!descriptorHeap_) {
            throw std::runtime_error("D3D12DescriptorHeap: Null descriptor heap");
        }

        if (descriptorCount_ == 0) {
            throw std::runtime_error("D3D12DescriptorHeap: Zero descriptor count");
        }

        if (descriptorSize_ == 0) {
            throw std::runtime_error("D3D12DescriptorHeap: Zero descriptor size");
        }
    }

    // ========================================================================
    // Private Implementation Methods
    // ========================================================================

    bool D3D12DescriptorHeap::CreateDescriptorHeap() {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = heapType_;
        heapDesc.NumDescriptors = descriptorCount_;
        heapDesc.Flags = heapFlags_;
        heapDesc.NodeMask = 0;

        HRESULT hr = device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap_));
        if (FAILED(hr)) {
            logChannel_.LogFormat(Logging::LogLevel::Error,
                "Failed to create descriptor heap: HRESULT = 0x{:08X}", static_cast<uint32_t>(hr));
            return false;
        }

        // Get descriptor size
        descriptorSize_ = device_->GetDescriptorHandleIncrementSize(heapType_);

        return true;
    }

    void D3D12DescriptorHeap::InitializeHandles() {
        baseCpuHandle_ = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();

        if (isShaderVisible_) {
            baseGpuHandle_ = descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
        }
        else {
            baseGpuHandle_ = D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
        }
    }

    void D3D12DescriptorHeap::SetupDebugInfo() {
        if (debugName_.empty()) {
            debugName_ = std::format("DescriptorHeap_{}", GetDescriptorTypeName(heapType_));
        }

        if (descriptorHeap_) {
            std::wstring wideName(debugName_.begin(), debugName_.end());
            descriptorHeap_->SetName(wideName.c_str());
        }
    }


    // ========================================================================
    // D3D12DescriptorHeapManager Implementation
    // ========================================================================

    D3D12DescriptorHeapManager::D3D12DescriptorHeapManager()
        : device_(nullptr)
        , heapSizes_{}
        , logChannel_(Logging::LogManager::Instance().GetChannel("D3D12DescriptorHeapManager")) {
    }

    D3D12DescriptorHeapManager::~D3D12DescriptorHeapManager() {
        Shutdown();
    }

    bool D3D12DescriptorHeapManager::Initialize(ID3D12Device* device, const HeapSizes& sizes) {
        if (!device) {
            logChannel_.Log(Logging::LogLevel::Error, "Invalid device pointer provided to descriptor heap manager");
            return false;
        }

        if (device_) {
            logChannel_.Log(Logging::LogLevel::Warning, "Descriptor heap manager already initialized, shutting down first");
            Shutdown();
        }

        device_ = device;
        heapSizes_ = sizes;

        logChannel_.Log(Logging::LogLevel::Info, "Initializing D3D12 descriptor heap manager");

        // Create RTV heap (non-shader-visible)
        if (!CreateRTVHeap()) {
            logChannel_.Log(Logging::LogLevel::Error, "Failed to create RTV descriptor heap");
            Shutdown();
            return false;
        }

        // Create DSV heap (non-shader-visible)
        if (!CreateDSVHeap()) {
            logChannel_.Log(Logging::LogLevel::Error, "Failed to create DSV descriptor heap");
            Shutdown();
            return false;
        }

        // Create CBV/SRV/UAV heap (non-shader-visible)
        if (!CreateCBVSRVUAVHeap()) {
            logChannel_.Log(Logging::LogLevel::Error, "Failed to create CBV/SRV/UAV descriptor heap");
            Shutdown();
            return false;
        }

        // Create Sampler heap (non-shader-visible)
        if (!CreateSamplerHeap()) {
            logChannel_.Log(Logging::LogLevel::Error, "Failed to create Sampler descriptor heap");
            Shutdown();
            return false;
        }

        // Create shader-visible CBV/SRV/UAV heap
        if (!CreateShaderVisibleCBVSRVUAVHeap()) {
            logChannel_.Log(Logging::LogLevel::Error, "Failed to create shader-visible CBV/SRV/UAV descriptor heap");
            Shutdown();
            return false;
        }

        // Create shader-visible Sampler heap
        if (!CreateShaderVisibleSamplerHeap()) {
            logChannel_.Log(Logging::LogLevel::Error, "Failed to create shader-visible Sampler descriptor heap");
            Shutdown();
            return false;
        }

        // Update statistics
        stats_.totalHeaps.store(6); // RTV, DSV, CBV_SRV_UAV, Sampler, ShaderVisible_CBV_SRV_UAV, ShaderVisible_Sampler
        stats_.totalDescriptors.store(
            heapSizes_.rtvHeapSize +
            heapSizes_.dsvHeapSize +
            heapSizes_.cbvSrvUavHeapSize +
            heapSizes_.samplerHeapSize +
            heapSizes_.shaderVisibleCbvSrvUavHeapSize +
            heapSizes_.shaderVisibleSamplerHeapSize
        );

        logChannel_.LogFormat(Logging::LogLevel::Info,
            "D3D12DescriptorHeapManager initialized successfully with {} heaps and {} total descriptors",
            stats_.totalHeaps.load(), stats_.totalDescriptors.load());

        LogAllHeapInfo();

        return true;
    }

    void D3D12DescriptorHeapManager::Shutdown() {
        if (!device_) {
            return; // Already shut down
        }

        logChannel_.Log(Logging::LogLevel::Info, "Shutting down D3D12 descriptor heap manager");

        // Shutdown heaps in reverse order of creation
        shaderVisibleSamplerHeap_.reset();
        shaderVisibleCbvSrvUavHeap_.reset();
        samplerHeap_.reset();
        cbvSrvUavHeap_.reset();
        dsvHeap_.reset();
        rtvHeap_.reset();

        // Reset statistics
        stats_.totalHeaps.store(0);
        stats_.totalDescriptors.store(0);
        stats_.totalMemoryUsed.store(0);

        device_ = nullptr;

        logChannel_.Log(Logging::LogLevel::Debug, "D3D12DescriptorHeapManager shutdown completed");
    }

    D3D12DescriptorHeap* D3D12DescriptorHeapManager::GetHeapByType(D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible) const {
        switch (type) {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            return shaderVisible ? shaderVisibleCbvSrvUavHeap_.get() : cbvSrvUavHeap_.get();

        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
            return shaderVisible ? shaderVisibleSamplerHeap_.get() : samplerHeap_.get();

        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            return rtvHeap_.get(); // RTVs are never shader-visible

        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            return dsvHeap_.get(); // DSVs are never shader-visible

        default:
            logChannel_.LogFormat(Logging::LogLevel::Error, "Unknown descriptor heap type: {}", static_cast<int>(type));
            return nullptr;
        }
    }

    std::vector<ID3D12DescriptorHeap*> D3D12DescriptorHeapManager::GetShaderVisibleHeaps() const {
        std::vector<ID3D12DescriptorHeap*> heaps;
        heaps.reserve(2);

        if (shaderVisibleCbvSrvUavHeap_ && shaderVisibleCbvSrvUavHeap_->GetD3D12DescriptorHeap()) {
            heaps.push_back(shaderVisibleCbvSrvUavHeap_->GetD3D12DescriptorHeap());
        }

        if (shaderVisibleSamplerHeap_ && shaderVisibleSamplerHeap_->GetD3D12DescriptorHeap()) {
            heaps.push_back(shaderVisibleSamplerHeap_->GetD3D12DescriptorHeap());
        }

        return heaps;
    }

    uint32_t D3D12DescriptorHeapManager::GetTotalDescriptorCount() const {
        return stats_.totalDescriptors.load();
    }

    uint32_t D3D12DescriptorHeapManager::GetUsedDescriptorCount() const {
        uint32_t usedCount = 0;

        if (rtvHeap_) {
            usedCount += rtvHeap_->GetDescriptorCount() - rtvHeap_->GetFreeDescriptorCount();
        }
        if (dsvHeap_) {
            usedCount += dsvHeap_->GetDescriptorCount() - dsvHeap_->GetFreeDescriptorCount();
        }
        if (cbvSrvUavHeap_) {
            usedCount += cbvSrvUavHeap_->GetDescriptorCount() - cbvSrvUavHeap_->GetFreeDescriptorCount();
        }
        if (samplerHeap_) {
            usedCount += samplerHeap_->GetDescriptorCount() - samplerHeap_->GetFreeDescriptorCount();
        }
        if (shaderVisibleCbvSrvUavHeap_) {
            usedCount += shaderVisibleCbvSrvUavHeap_->GetDescriptorCount() - shaderVisibleCbvSrvUavHeap_->GetFreeDescriptorCount();
        }
        if (shaderVisibleSamplerHeap_) {
            usedCount += shaderVisibleSamplerHeap_->GetDescriptorCount() - shaderVisibleSamplerHeap_->GetFreeDescriptorCount();
        }

        return usedCount;
    }

    float D3D12DescriptorHeapManager::GetOverallUtilization() const {
        uint32_t total = GetTotalDescriptorCount();
        if (total == 0) {
            return 0.0f;
        }

        uint32_t used = GetUsedDescriptorCount();
        return static_cast<float>(used) / static_cast<float>(total);
    }

    void D3D12DescriptorHeapManager::ResetAllStats() {
        if (rtvHeap_) rtvHeap_->ResetStats();
        if (dsvHeap_) dsvHeap_->ResetStats();
        if (cbvSrvUavHeap_) cbvSrvUavHeap_->ResetStats();
        if (samplerHeap_) samplerHeap_->ResetStats();
        if (shaderVisibleCbvSrvUavHeap_) shaderVisibleCbvSrvUavHeap_->ResetStats();
        if (shaderVisibleSamplerHeap_) shaderVisibleSamplerHeap_->ResetStats();

        logChannel_.Log(Logging::LogLevel::Debug, "All descriptor heap statistics reset");
    }

    std::string D3D12DescriptorHeapManager::GetDetailedDebugInfo() const {
        std::string info;
        info.reserve(2048);

        info += "D3D12DescriptorHeapManager Detailed Debug Info:\n";
        info += std::format("  Total Heaps: {}\n", stats_.totalHeaps.load());
        info += std::format("  Total Descriptors: {}\n", stats_.totalDescriptors.load());
        info += std::format("  Used Descriptors: {}\n", GetUsedDescriptorCount());
        info += std::format("  Overall Utilization: {:.2f}%\n", GetOverallUtilization() * 100.0f);
        info += std::format("  Total Memory Used: {} bytes\n", stats_.totalMemoryUsed.load());
        info += "\n";

        // Individual heap information
        if (rtvHeap_) {
            info += "RTV Heap:\n";
            info += AddIndentation(rtvHeap_->GetDebugInfo(), "  ");
            info += "\n";
        }

        if (dsvHeap_) {
            info += "DSV Heap:\n";
            info += AddIndentation(dsvHeap_->GetDebugInfo(), "  ");
            info += "\n";
        }

        if (cbvSrvUavHeap_) {
            info += "CBV/SRV/UAV Heap (Non-Shader-Visible):\n";
            info += AddIndentation(cbvSrvUavHeap_->GetDebugInfo(), "  ");
            info += "\n";
        }

        if (samplerHeap_) {
            info += "Sampler Heap (Non-Shader-Visible):\n";
            info += AddIndentation(samplerHeap_->GetDebugInfo(), "  ");
            info += "\n";
        }

        if (shaderVisibleCbvSrvUavHeap_) {
            info += "CBV/SRV/UAV Heap (Shader-Visible):\n";
            info += AddIndentation(shaderVisibleCbvSrvUavHeap_->GetDebugInfo(), "  ");
            info += "\n";
        }

        if (shaderVisibleSamplerHeap_) {
            info += "Sampler Heap (Shader-Visible):\n";
            info += AddIndentation(shaderVisibleSamplerHeap_->GetDebugInfo(), "  ");
            info += "\n";
        }

        return info;
    }

    void D3D12DescriptorHeapManager::LogAllHeapInfo() const {
        logChannel_.Log(Logging::LogLevel::Info, GetDetailedDebugInfo());
    }

    void D3D12DescriptorHeapManager::ValidateAllHeaps() const {
        if (rtvHeap_) {
            try {
                rtvHeap_->ValidateHeapIntegrity();
            }
            catch (const std::exception& e) {
                logChannel_.LogFormat(Logging::LogLevel::Error, "RTV heap validation failed: {}", e.what());
                throw;
            }
        }

        if (dsvHeap_) {
            try {
                dsvHeap_->ValidateHeapIntegrity();
            }
            catch (const std::exception& e) {
                logChannel_.LogFormat(Logging::LogLevel::Error, "DSV heap validation failed: {}", e.what());
                throw;
            }
        }

        if (cbvSrvUavHeap_) {
            try {
                cbvSrvUavHeap_->ValidateHeapIntegrity();
            }
            catch (const std::exception& e) {
                logChannel_.LogFormat(Logging::LogLevel::Error, "CBV/SRV/UAV heap validation failed: {}", e.what());
                throw;
            }
        }

        if (samplerHeap_) {
            try {
                samplerHeap_->ValidateHeapIntegrity();
            }
            catch (const std::exception& e) {
                logChannel_.LogFormat(Logging::LogLevel::Error, "Sampler heap validation failed: {}", e.what());
                throw;
            }
        }

        if (shaderVisibleCbvSrvUavHeap_) {
            try {
                shaderVisibleCbvSrvUavHeap_->ValidateHeapIntegrity();
            }
            catch (const std::exception& e) {
                logChannel_.LogFormat(Logging::LogLevel::Error, "Shader-visible CBV/SRV/UAV heap validation failed: {}", e.what());
                throw;
            }
        }

        if (shaderVisibleSamplerHeap_) {
            try {
                shaderVisibleSamplerHeap_->ValidateHeapIntegrity();
            }
            catch (const std::exception& e) {
                logChannel_.LogFormat(Logging::LogLevel::Error, "Shader-visible Sampler heap validation failed: {}", e.what());
                throw;
            }
        }

        logChannel_.Log(Logging::LogLevel::Debug, "All descriptor heaps validated successfully");
    }

    // ========================================================================
    // Private Implementation Methods
    // ========================================================================

    bool D3D12DescriptorHeapManager::CreateHeap(std::unique_ptr<D3D12DescriptorHeap>& heap,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        uint32_t descriptorCount,
        bool shaderVisible,
        const char* debugName) {
        if (descriptorCount == 0) {
            logChannel_.LogFormat(Logging::LogLevel::Warning, "Descriptor count is zero for heap type {}, skipping creation", static_cast<int>(type));
            return true; // Not an error, just skip creation
        }

        heap = std::make_unique<D3D12DescriptorHeap>(type, shaderVisible);
        if (!heap->InitializeWithDevice(device_, type, descriptorCount, shaderVisible)) {
            logChannel_.LogFormat(Logging::LogLevel::Error, "Failed to initialize descriptor heap of type {} with {} descriptors", static_cast<int>(type), descriptorCount);
            heap.reset();
            return false;
        }

        switch (type) {
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            heap->SetDebugName(debugName ? debugName : "RTVHeap");
            rtvHeap_ = std::move(heap);
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            heap->SetDebugName(debugName ? debugName : "DSVHeap");
            dsvHeap_ = std::move(heap);
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            heap->SetDebugName(debugName ? debugName : (shaderVisible ? "ShaderVisibleCBVSRVUAVHeap" : "CBVSRVUAVHeap"));
            if (shaderVisible) {
                shaderVisibleCbvSrvUavHeap_ = std::move(heap);
            }
            else {
                cbvSrvUavHeap_ = std::move(heap);
            }
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
            heap->SetDebugName(debugName ? debugName : (shaderVisible ? "ShaderVisibleSamplerHeap" : "SamplerHeap"));
            if (shaderVisible) {
                shaderVisibleSamplerHeap_ = std::move(heap);
            }
            else {
                samplerHeap_ = std::move(heap);
            }
            break;
        default:
            heap->SetDebugName(debugName ? debugName : "UnknownHeap");
            break;
        }
        return true;
    }

    bool D3D12DescriptorHeapManager::CreateRTVHeap() {
        rtvHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
        if (!rtvHeap_->InitializeWithDevice(device_, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, heapSizes_.rtvHeapSize, false)) {
            logChannel_.LogFormat(Logging::LogLevel::Error, "Failed to initialize RTV heap with {} descriptors", heapSizes_.rtvHeapSize);
            rtvHeap_.reset();
            return false;
        }

        rtvHeap_->SetDebugName("RTVHeap");
        logChannel_.LogFormat(Logging::LogLevel::Debug, "Created RTV heap with {} descriptors", heapSizes_.rtvHeapSize);
        return true;
    }

    bool D3D12DescriptorHeapManager::CreateDSVHeap() {
        dsvHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);
        if (!dsvHeap_->InitializeWithDevice(device_, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, heapSizes_.dsvHeapSize, false)) {
            logChannel_.LogFormat(Logging::LogLevel::Error, "Failed to initialize DSV heap with {} descriptors", heapSizes_.dsvHeapSize);
            dsvHeap_.reset();
            return false;
        }

        dsvHeap_->SetDebugName("DSVHeap");
        logChannel_.LogFormat(Logging::LogLevel::Debug, "Created DSV heap with {} descriptors", heapSizes_.dsvHeapSize);
        return true;
    }

    bool D3D12DescriptorHeapManager::CreateCBVSRVUAVHeap() {
        cbvSrvUavHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, false);
        if (!cbvSrvUavHeap_->InitializeWithDevice(device_, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, heapSizes_.cbvSrvUavHeapSize, false)) {
            logChannel_.LogFormat(Logging::LogLevel::Error, "Failed to initialize CBV/SRV/UAV heap with {} descriptors", heapSizes_.cbvSrvUavHeapSize);
            cbvSrvUavHeap_.reset();
            return false;
        }

        cbvSrvUavHeap_->SetDebugName("CBVSRVUAVHeap");
        logChannel_.LogFormat(Logging::LogLevel::Debug, "Created CBV/SRV/UAV heap with {} descriptors", heapSizes_.cbvSrvUavHeapSize);
        return true;
    }

    bool D3D12DescriptorHeapManager::CreateSamplerHeap() {
        samplerHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, false);
        if (!samplerHeap_->InitializeWithDevice(device_, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, heapSizes_.samplerHeapSize, false)) {
            logChannel_.LogFormat(Logging::LogLevel::Error, "Failed to initialize Sampler heap with {} descriptors", heapSizes_.samplerHeapSize);
            samplerHeap_.reset();
            return false;
        }

        samplerHeap_->SetDebugName("SamplerHeap");
        logChannel_.LogFormat(Logging::LogLevel::Debug, "Created Sampler heap with {} descriptors", heapSizes_.samplerHeapSize);
        return true;
    }

    bool D3D12DescriptorHeapManager::CreateShaderVisibleCBVSRVUAVHeap() {
        shaderVisibleCbvSrvUavHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
        if (!shaderVisibleCbvSrvUavHeap_->InitializeWithDevice(device_, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, heapSizes_.shaderVisibleCbvSrvUavHeapSize, true)) {
            logChannel_.LogFormat(Logging::LogLevel::Error, "Failed to initialize shader-visible CBV/SRV/UAV heap with {} descriptors", heapSizes_.shaderVisibleCbvSrvUavHeapSize);
            shaderVisibleCbvSrvUavHeap_.reset();
            return false;
        }

        shaderVisibleCbvSrvUavHeap_->SetDebugName("ShaderVisibleCBVSRVUAVHeap");
        logChannel_.LogFormat(Logging::LogLevel::Debug, "Created shader-visible CBV/SRV/UAV heap with {} descriptors", heapSizes_.shaderVisibleCbvSrvUavHeapSize);
        return true;
    }

    bool D3D12DescriptorHeapManager::CreateShaderVisibleSamplerHeap() {
        shaderVisibleSamplerHeap_ = std::make_unique<D3D12DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, true);
        if (!shaderVisibleSamplerHeap_->InitializeWithDevice(device_, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, heapSizes_.shaderVisibleSamplerHeapSize, true)) {
            logChannel_.LogFormat(Logging::LogLevel::Error, "Failed to initialize shader-visible Sampler heap with {} descriptors", heapSizes_.shaderVisibleSamplerHeapSize);
            shaderVisibleSamplerHeap_.reset();
            return false;
        }

        shaderVisibleSamplerHeap_->SetDebugName("ShaderVisibleSamplerHeap");
        logChannel_.LogFormat(Logging::LogLevel::Debug, "Created shader-visible Sampler heap with {} descriptors", heapSizes_.shaderVisibleSamplerHeapSize);
        return true;
    }

    void D3D12DescriptorHeapManager::LogHeapConfiguration() const {
        logChannel_.Log(Logging::LogLevel::Info, "Descriptor Heap Configuration:");
        logChannel_.LogFormat(Logging::LogLevel::Info, "  RTV Heap Size: {}", heapSizes_.rtvHeapSize);
        logChannel_.LogFormat(Logging::LogLevel::Info, "  DSV Heap Size: {}", heapSizes_.dsvHeapSize);
        logChannel_.LogFormat(Logging::LogLevel::Info, "  CBV/SRV/UAV Heap Size: {}", heapSizes_.cbvSrvUavHeapSize);
        logChannel_.LogFormat(Logging::LogLevel::Info, "  Sampler Heap Size: {}", heapSizes_.samplerHeapSize);
        logChannel_.LogFormat(Logging::LogLevel::Info, "  Shader-Visible CBV/SRV/UAV Heap Size: {}", heapSizes_.shaderVisibleCbvSrvUavHeapSize);
        logChannel_.LogFormat(Logging::LogLevel::Info, "  Shader-Visible Sampler Heap Size: {}", heapSizes_.shaderVisibleSamplerHeapSize);
    }

    std::string D3D12DescriptorHeapManager::AddIndentation(const std::string& text, const std::string& indent) const {
        std::string result;
        std::istringstream stream(text);
        std::string line;

        while (std::getline(stream, line)) {
            result += indent + line + "\n";
        }

        return result;
    }
} // namespace Akhanda::RHI::D3D12
