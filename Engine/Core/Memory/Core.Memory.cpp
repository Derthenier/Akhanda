// Core.Memory.cpp - Memory Management Module Implementation
module;

#include <Windows.h>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iomanip>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

module Akhanda.Core.Memory;

import Akhanda.Core.Logging;

using namespace Akhanda::Memory;
using namespace Akhanda::Logging;

// =============================================================================
// Platform-Specific Aligned Allocation
// =============================================================================

namespace {
    void* PlatformAlignedAlloc(size_t size, size_t alignment) noexcept {
        assert(IsPowerOfTwo(alignment) && "Alignment must be power of 2");
        assert(alignment >= sizeof(void*) && "Alignment too small");

        return _aligned_malloc(size, alignment);
    }

    void PlatformAlignedFree(void* ptr) noexcept {
        _aligned_free(ptr);
    }

    size_t PlatformGetAllocSize(void* ptr) noexcept {
        return _aligned_msize(ptr, DEFAULT_ALIGNMENT, 0);
    }
}

// =============================================================================
// Memory Statistics Implementation
// =============================================================================

void MemoryStatistics::Reset() noexcept {
    totalAllocated.store(0, std::memory_order_relaxed);
    totalFreed.store(0, std::memory_order_relaxed);
    currentUsage.store(0, std::memory_order_relaxed);
    peakUsage.store(0, std::memory_order_relaxed);
    allocationCount.store(0, std::memory_order_relaxed);
    freeCount.store(0, std::memory_order_relaxed);
    activeAllocations.store(0, std::memory_order_relaxed);
}

// =============================================================================
// Memory Tracker Implementation
// =============================================================================

namespace {
    class MemoryTracker {
    public:
        static MemoryTracker& Instance() {
            static MemoryTracker instance;
            return instance;
        }

        void TrackAllocation(const AllocationInfo& info) {
            if (!enabled_.load(std::memory_order_relaxed)) return;

            std::lock_guard<std::shared_mutex> lock(mutex_);

            if (allocations_.size() >= MAX_TRACKED_ALLOCATIONS) {
                // Remove oldest allocation to make room
                auto oldest = std::min_element(allocations_.begin(), allocations_.end(),
                    [](const auto& a, const auto& b) {
                        return a.second.timestamp < b.second.timestamp;
                    });
                allocations_.erase(oldest);
            }

            allocations_[info.ptr] = info;

            // Add to history
            if (allocationHistory_.size() >= ALLOCATION_HISTORY_SIZE) {
                allocationHistory_.erase(allocationHistory_.begin());
            }
            allocationHistory_.push_back(info);
        }

        void TrackDeallocation(void* ptr, const std::source_location& location) {
            if (!enabled_.load(std::memory_order_relaxed)) return;

            std::lock_guard<std::shared_mutex> lock(mutex_);

            auto it = allocations_.find(ptr);
            if (it != allocations_.end()) {
                // Track deallocation in history
                AllocationInfo deallocInfo = it->second;
                deallocInfo.ptr = nullptr; // Mark as deallocation
                deallocInfo.location = location;
                deallocInfo.timestamp = std::chrono::high_resolution_clock::now();

                if (allocationHistory_.size() >= ALLOCATION_HISTORY_SIZE) {
                    allocationHistory_.erase(allocationHistory_.begin());
                }
                allocationHistory_.push_back(deallocInfo);

                allocations_.erase(it);
            }
        }

        void DumpLeaks() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);

            auto& logger = Channels::Engine();

            if (allocations_.empty()) {
                logger.Info("Memory leak detection: No leaks detected");
                return;
            }

            auto msg = std::format("Memory leak detection: {} leaked allocations found", allocations_.size());
            logger.Error(msg);

            size_t totalLeakedBytes = 0;
            for (const auto& [ptr, info] : allocations_) {
                totalLeakedBytes += info.size;

                auto msg1 = std::format("LEAK: {} bytes at 0x{:X} from {}:{} in {} (allocator: {})",
                    info.size,
                    reinterpret_cast<uintptr_t>(ptr),
                    info.location.file_name(),
                    info.location.line(),
                    info.location.function_name(),
                    info.allocatorName);
                logger.Error(msg1);
            }

            auto totalMsg = std::format("Total leaked memory: {}", FormatBytes(totalLeakedBytes));
            logger.Error(totalMsg);
        }

        void SetEnabled(bool enabled) {
            enabled_.store(enabled, std::memory_order_relaxed);
        }

        bool IsEnabled() const {
            return enabled_.load(std::memory_order_relaxed);
        }

        size_t GetTrackedAllocationCount() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return allocations_.size();
        }

    private:
        mutable std::shared_mutex mutex_;
        std::unordered_map<void*, AllocationInfo> allocations_;
        std::vector<AllocationInfo> allocationHistory_;
        std::atomic<bool> enabled_{ true };
    };
}

// =============================================================================
// Linear Allocator Implementation
// =============================================================================

class LinearAllocator::Impl {
public:
    explicit Impl(size_t size, const char* name)
        : name_(name), totalSize_(size), currentOffset_(0) {

        memory_ = static_cast<uint8_t*>(PlatformAlignedAlloc(size, MAX_ALIGNMENT));
        if (!memory_) {
            throw std::bad_alloc();
        }

        auto msg = std::format("LinearAllocator '{}' created with {} of memory",
            name_, FormatBytes(size));
        Channels::Engine().Info(msg);
    }

    ~Impl() {
        if (memory_) {
            auto msg = std::format("LinearAllocator '{}' destroyed. Peak usage: {} / {}",
                name_,
                FormatBytes(stats_.GetPeakUsage()),
                FormatBytes(totalSize_));
            Channels::Engine().Info(msg);
            PlatformAlignedFree(memory_);
        }
    }

    void* Allocate(size_t size, size_t alignment, const std::source_location& location) {
        if (size == 0) return nullptr;

        std::lock_guard<std::shared_mutex> lock(mutex_);

        const size_t alignedOffset = AlignUp(currentOffset_, alignment);
        const size_t alignedSize = AlignUp(size, DEFAULT_ALIGNMENT);

        if (alignedOffset + alignedSize > totalSize_) {
            auto msg = std::format("LinearAllocator '{}' out of memory. Requested: {}, Available: {}",
                name_, FormatBytes(alignedSize), FormatBytes(totalSize_ - alignedOffset));
            Channels::Engine().Error(msg);
            return nullptr;
        }

        void* result = memory_ + alignedOffset;
        currentOffset_ = alignedOffset + alignedSize;

        // Update statistics
        const size_t currentUsage = currentOffset_;
        stats_.totalAllocated.fetch_add(alignedSize, std::memory_order_relaxed);
        stats_.currentUsage.store(currentUsage, std::memory_order_relaxed);
        stats_.allocationCount.fetch_add(1, std::memory_order_relaxed);
        stats_.activeAllocations.fetch_add(1, std::memory_order_relaxed);

        // Update peak usage
        size_t expectedPeak = stats_.peakUsage.load(std::memory_order_relaxed);
        while (currentUsage > expectedPeak &&
            !stats_.peakUsage.compare_exchange_weak(expectedPeak, currentUsage, std::memory_order_relaxed)) {
            // Retry if another thread updated peak usage
        }

        // Track allocation
        MemoryTracker::Instance().TrackAllocation(
            AllocationInfo(result, alignedSize, alignment, name_.c_str(), location));

        return result;
    }

    void Deallocate(void* ptr, const std::source_location& location) {
        if (!ptr) return;

        // Linear allocator doesn't support individual deallocation
        // Just track for leak detection
        MemoryTracker::Instance().TrackDeallocation(ptr, location);

        // Note: We don't update statistics here since memory isn't actually freed
    }

    void Reset() {
        std::lock_guard<std::shared_mutex> lock(mutex_);

        const size_t wasUsed = currentOffset_;
        currentOffset_ = 0;

        stats_.totalFreed.fetch_add(wasUsed, std::memory_order_relaxed);
        stats_.currentUsage.store(0, std::memory_order_relaxed);
        stats_.activeAllocations.store(0, std::memory_order_relaxed);
        stats_.freeCount.fetch_add(1, std::memory_order_relaxed);

        auto msg = std::format("LinearAllocator '{}' reset. Freed {}",
            name_, FormatBytes(wasUsed));
        Channels::Engine().Info(msg);
    }

    bool OwnsPointer(void* ptr) const {
        if (!ptr) return false;

        std::shared_lock<std::shared_mutex> lock(mutex_);
        const uint8_t* bytePtr = static_cast<uint8_t*>(ptr);
        return bytePtr >= memory_ && bytePtr < memory_ + totalSize_;
    }

    std::string name_;
    uint8_t* memory_;
    size_t totalSize_;
    size_t currentOffset_;
    MemoryStatistics stats_;
    mutable std::shared_mutex mutex_;
};

LinearAllocator::LinearAllocator(size_t size, const char* name)
    : pImpl_(std::make_unique<Impl>(size, name)) {
}

LinearAllocator::~LinearAllocator() = default;

LinearAllocator::LinearAllocator(LinearAllocator&&) noexcept = default;
LinearAllocator& LinearAllocator::operator=(LinearAllocator&&) noexcept = default;

void* LinearAllocator::Allocate(size_t size, size_t alignment, const std::source_location& location) {
    return pImpl_->Allocate(size, alignment, location);
}

void LinearAllocator::Deallocate(void* ptr, const std::source_location& location) {
    pImpl_->Deallocate(ptr, location);
}

const char* LinearAllocator::GetName() const noexcept {
    return pImpl_->name_.c_str();
}

size_t LinearAllocator::GetTotalSize() const noexcept {
    return pImpl_->totalSize_;
}

size_t LinearAllocator::GetUsedSize() const noexcept {
    std::shared_lock<std::shared_mutex> lock(pImpl_->mutex_);
    return pImpl_->currentOffset_;
}

const MemoryStatistics& LinearAllocator::GetStatistics() const noexcept {
    return pImpl_->stats_;
}

void LinearAllocator::ResetStatistics() noexcept {
    pImpl_->stats_.Reset();
}

bool LinearAllocator::OwnsPointer(void* ptr) const noexcept {
    return pImpl_->OwnsPointer(ptr);
}

void LinearAllocator::Reset() {
    pImpl_->Reset();
}

void* LinearAllocator::GetBasePtr() const noexcept {
    return pImpl_->memory_;
}

size_t LinearAllocator::GetOffset() const noexcept {
    std::shared_lock<std::shared_mutex> lock(pImpl_->mutex_);
    return pImpl_->currentOffset_;
}

// =============================================================================
// Pool Allocator Implementation
// =============================================================================

class PoolAllocator::Impl {
public:
    PoolAllocator::Impl(size_t blockSize, size_t blockCount, size_t alignment, const char* name)
        : name_(name), blockSize_(AlignUp(blockSize, alignment)), blockCount_(blockCount),
        alignment_(alignment), freeBlocks_(blockCount) {

        totalSize_ = blockSize_ * blockCount_;
        memory_ = static_cast<uint8_t*>(PlatformAlignedAlloc(totalSize_, MAX_ALIGNMENT));

        if (!memory_) {
            throw std::bad_alloc();
        }

        // Initialize free list
        InitializeFreeList();

        auto msg = std::format("PoolAllocator '{}' created: {} blocks of {} each ({})",
            name_, blockCount_, FormatBytes(blockSize_), FormatBytes(totalSize_));
        Channels::Engine().Info(msg);
    }

    ~Impl() {
        if (memory_) {
            auto msg = std::format("PoolAllocator '{}' destroyed. {} / {} blocks used at peak",
                name_,
                blockCount_ - stats_.GetPeakUsage() / blockSize_,
                blockCount_);
            Channels::Engine().Info(msg);
            PlatformAlignedFree(memory_);
        }
    }

    void* Allocate(size_t size, size_t alignment, const std::source_location& location) {
        if (size == 0) return nullptr;

        if (size > blockSize_) {
            auto msg = std::format("PoolAllocator '{}' cannot allocate {} (block size: {})",
                name_, FormatBytes(size), FormatBytes(blockSize_));
            Channels::Engine().Error(msg);
            return nullptr;
        }

        if (alignment > alignment_) {
            auto msg = std::format("PoolAllocator '{}' cannot provide alignment {} (max: {})",
                name_, alignment, alignment_);
            Channels::Engine().Error(msg);
            return nullptr;
        }

        std::lock_guard<std::shared_mutex> lock(mutex_);

        if (!freeHead_) {
            auto msg = std::format("PoolAllocator '{}' out of blocks", name_);
            Channels::Engine().Error(msg);
            return nullptr;
        }

        void* result = freeHead_;
        freeHead_ = *static_cast<void**>(freeHead_);
        --freeBlocks_;

        // Update statistics
        const size_t currentUsage = (blockCount_ - freeBlocks_) * blockSize_;
        stats_.totalAllocated.fetch_add(blockSize_, std::memory_order_relaxed);
        stats_.currentUsage.store(currentUsage, std::memory_order_relaxed);
        stats_.allocationCount.fetch_add(1, std::memory_order_relaxed);
        stats_.activeAllocations.fetch_add(1, std::memory_order_relaxed);

        // Update peak usage
        size_t expectedPeak = stats_.peakUsage.load(std::memory_order_relaxed);
        while (currentUsage > expectedPeak &&
            !stats_.peakUsage.compare_exchange_weak(expectedPeak, currentUsage, std::memory_order_relaxed)) {
        }

        // Track allocation
        MemoryTracker::Instance().TrackAllocation(
            AllocationInfo(result, size, alignment, name_.c_str(), location));

        return result;
    }

    void Deallocate(void* ptr, const std::source_location& location) {
        if (!ptr) return;

        if (!OwnsPointer(ptr)) {
            auto msg = std::format("PoolAllocator '{}' does not own pointer 0x{:X}",
                name_, reinterpret_cast<uintptr_t>(ptr));
            Channels::Engine().Error(msg);
            return;
        }

        std::lock_guard<std::shared_mutex> lock(mutex_);

        *static_cast<void**>(ptr) = freeHead_;
        freeHead_ = ptr;
        ++freeBlocks_;

        // Update statistics
        const size_t currentUsage = (blockCount_ - freeBlocks_) * blockSize_;
        stats_.totalFreed.fetch_add(blockSize_, std::memory_order_relaxed);
        stats_.currentUsage.store(currentUsage, std::memory_order_relaxed);
        stats_.freeCount.fetch_add(1, std::memory_order_relaxed);
        stats_.activeAllocations.fetch_sub(1, std::memory_order_relaxed);

        // Track deallocation
        MemoryTracker::Instance().TrackDeallocation(ptr, location);
    }

    void Reset() {
        std::lock_guard<std::shared_mutex> lock(mutex_);

        const size_t wasUsed = (blockCount_ - freeBlocks_) * blockSize_;

        InitializeFreeList();

        stats_.totalFreed.fetch_add(wasUsed, std::memory_order_relaxed);
        stats_.currentUsage.store(0, std::memory_order_relaxed);
        stats_.activeAllocations.store(0, std::memory_order_relaxed);
        stats_.freeCount.fetch_add(1, std::memory_order_relaxed);

        auto msg = std::format("PoolAllocator '{}' reset. Freed {}",
            name_, FormatBytes(wasUsed));
        Channels::Engine().Info(msg);
    }

    bool OwnsPointer(void* ptr) const {
        if (!ptr) return false;

        const uint8_t* bytePtr = static_cast<uint8_t*>(ptr);
        if (bytePtr < memory_ || bytePtr >= memory_ + totalSize_) {
            return false;
        }

        // Check if pointer is aligned to block boundary
        const size_t offset = bytePtr - memory_;
        return (offset % blockSize_) == 0;
    }

private:
    void InitializeFreeList() {
        freeHead_ = memory_;
        freeBlocks_ = blockCount_;

        // Link all blocks in free list
        for (size_t i = 0; i < blockCount_ - 1; ++i) {
            void** current = reinterpret_cast<void**>(memory_ + i * blockSize_);
            *current = memory_ + (i + 1) * blockSize_;
        }

        // Last block points to null
        void** last = reinterpret_cast<void**>(memory_ + (blockCount_ - 1) * blockSize_);
        *last = nullptr;
    }

    std::string name_;
    uint8_t* memory_;
    size_t blockSize_;
    size_t blockCount_;
    size_t totalSize_;
    size_t alignment_;
    void* freeHead_;
    size_t freeBlocks_;
    MemoryStatistics stats_;
    mutable std::shared_mutex mutex_;

    friend class PoolAllocator; // Allow PoolAllocator to access private members
};

PoolAllocator::PoolAllocator(size_t blockSize, size_t blockCount, size_t alignment, const char* name)
    : pImpl_(std::make_unique<Impl>(blockSize, blockCount, alignment, name)) {
}

PoolAllocator::~PoolAllocator() = default;

PoolAllocator::PoolAllocator(PoolAllocator&&) noexcept = default;
PoolAllocator& PoolAllocator::operator=(PoolAllocator&&) noexcept = default;

void* PoolAllocator::Allocate(size_t size, size_t alignment, const std::source_location& location) {
    return pImpl_->Allocate(size, alignment, location);
}

void PoolAllocator::Deallocate(void* ptr, const std::source_location& location) {
    pImpl_->Deallocate(ptr, location);
}

const char* PoolAllocator::GetName() const noexcept {
    return pImpl_->name_.c_str();
}

size_t PoolAllocator::GetTotalSize() const noexcept {
    return pImpl_->totalSize_;
}

size_t PoolAllocator::GetUsedSize() const noexcept {
    std::shared_lock<std::shared_mutex> lock(pImpl_->mutex_);
    return (pImpl_->blockCount_ - pImpl_->freeBlocks_) * pImpl_->blockSize_;
}

const MemoryStatistics& PoolAllocator::GetStatistics() const noexcept {
    return pImpl_->stats_;
}

void PoolAllocator::ResetStatistics() noexcept {
    pImpl_->stats_.Reset();
}

bool PoolAllocator::OwnsPointer(void* ptr) const noexcept {
    return pImpl_->OwnsPointer(ptr);
}

void PoolAllocator::Reset() {
    pImpl_->Reset();
}

size_t PoolAllocator::GetBlockSize() const noexcept {
    return pImpl_->blockSize_;
}

size_t PoolAllocator::GetBlockCount() const noexcept {
    return pImpl_->blockCount_;
}

size_t PoolAllocator::GetFreeBlocks() const noexcept {
    std::shared_lock<std::shared_mutex> lock(pImpl_->mutex_);
    return pImpl_->freeBlocks_;
}

// =============================================================================
// Stack Allocator Implementation
// =============================================================================

class StackAllocator::Impl {
public:
    struct AllocationHeader {
        size_t size;
        size_t previousOffset;
        size_t padding;
    };

    explicit Impl(size_t size, const char* name)
        : name_(name), totalSize_(size), currentOffset_(0) {

        memory_ = static_cast<uint8_t*>(PlatformAlignedAlloc(size, MAX_ALIGNMENT));
        if (!memory_) {
            throw std::bad_alloc();
        }

        auto msg = std::format("StackAllocator '{}' created with {}",
            name_, FormatBytes(size));
        Channels::Engine().Info(msg);
    }

    ~Impl() {
        if (memory_) {
            auto msg = std::format("StackAllocator '{}' destroyed. Peak usage: {} / {}",
                name_,
                FormatBytes(stats_.GetPeakUsage()),
                FormatBytes(totalSize_));
            Channels::Engine().Info(msg);
            PlatformAlignedFree(memory_);
        }
    }

    void* Allocate(size_t size, size_t alignment, const std::source_location& location) {
        if (size == 0) return nullptr;

        std::lock_guard<std::shared_mutex> lock(mutex_);

        const size_t headerSize = sizeof(AllocationHeader);
        const size_t headerOffset = AlignUp(currentOffset_, alignof(AllocationHeader));
        const size_t dataOffset = AlignUp(headerOffset + headerSize, alignment);
        const size_t totalAllocationSize = dataOffset + size - currentOffset_;

        if (currentOffset_ + totalAllocationSize > totalSize_) {
            auto msg = std::format("StackAllocator '{}' out of memory. Requested: {}, Available: {}",
                name_, FormatBytes(totalAllocationSize),
                FormatBytes(totalSize_ - currentOffset_));
            Channels::Engine().Error(msg);
            return nullptr;
        }

        // Store allocation header
        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(memory_ + headerOffset);
        header->size = size;
        header->previousOffset = currentOffset_;
        header->padding = dataOffset - headerOffset - headerSize;

        void* result = memory_ + dataOffset;
        currentOffset_ = dataOffset + size;

        // Update statistics
        const size_t currentUsage = currentOffset_;
        stats_.totalAllocated.fetch_add(totalAllocationSize, std::memory_order_relaxed);
        stats_.currentUsage.store(currentUsage, std::memory_order_relaxed);
        stats_.allocationCount.fetch_add(1, std::memory_order_relaxed);
        stats_.activeAllocations.fetch_add(1, std::memory_order_relaxed);

        // Update peak usage
        size_t expectedPeak = stats_.peakUsage.load(std::memory_order_relaxed);
        while (currentUsage > expectedPeak &&
            !stats_.peakUsage.compare_exchange_weak(expectedPeak, currentUsage, std::memory_order_relaxed)) {
        }

        // Track allocation
        MemoryTracker::Instance().TrackAllocation(
            AllocationInfo(result, size, alignment, name_.c_str(), location));

        return result;
    }

    void Deallocate(void* ptr, const std::source_location& location) {
        if (!ptr) return;

        std::lock_guard<std::shared_mutex> lock(mutex_);

        // Find the allocation header
        const uint8_t* dataPtr = static_cast<uint8_t*>(ptr);
        const uint8_t* headerPtr = dataPtr;

        // Search backwards for the header
        while (headerPtr > memory_) {
            headerPtr -= sizeof(AllocationHeader);
            if (IsAligned(headerPtr, alignof(AllocationHeader))) {
                const AllocationHeader* header = reinterpret_cast<const AllocationHeader*>(headerPtr);
                const uint8_t* expectedDataPtr = headerPtr + sizeof(AllocationHeader) + header->padding;

                if (expectedDataPtr == dataPtr) {
                    // Found the correct header
                    const size_t freedBytes = currentOffset_ - header->previousOffset;
                    currentOffset_ = header->previousOffset;

                    // Update statistics
                    stats_.totalFreed.fetch_add(freedBytes, std::memory_order_relaxed);
                    stats_.currentUsage.store(currentOffset_, std::memory_order_relaxed);
                    stats_.freeCount.fetch_add(1, std::memory_order_relaxed);
                    stats_.activeAllocations.fetch_sub(1, std::memory_order_relaxed);

                    // Track deallocation
                    MemoryTracker::Instance().TrackDeallocation(ptr, location);
                    return;
                }
            }
        }

        auto msg = std::format("StackAllocator '{}' invalid deallocation at 0x{:X}",
            name_, reinterpret_cast<uintptr_t>(ptr));
        Channels::Engine().Error(msg);
    }

    void Reset() {
        std::lock_guard<std::shared_mutex> lock(mutex_);

        const size_t wasUsed = currentOffset_;
        currentOffset_ = 0;

        stats_.totalFreed.fetch_add(wasUsed, std::memory_order_relaxed);
        stats_.currentUsage.store(0, std::memory_order_relaxed);
        stats_.activeAllocations.store(0, std::memory_order_relaxed);
        stats_.freeCount.fetch_add(1, std::memory_order_relaxed);

        auto msg = std::format("StackAllocator '{}' reset. Freed {}",
            name_, FormatBytes(wasUsed));
        Channels::Engine().Info(msg);
    }

    bool OwnsPointer(void* ptr) const {
        if (!ptr) return false;

        std::shared_lock<std::shared_mutex> lock(mutex_);
        const uint8_t* bytePtr = static_cast<uint8_t*>(ptr);
        return bytePtr >= memory_ && bytePtr < memory_ + currentOffset_;
    }

    StackAllocator::Marker GetMarker() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return StackAllocator::Marker(currentOffset_);
    }

    void RewindToMarker(const StackAllocator::Marker& marker) {
        std::lock_guard<std::shared_mutex> lock(mutex_);

        const size_t markerOffset = marker.GetOffset();
        if (markerOffset <= currentOffset_) {
            const size_t freedBytes = currentOffset_ - markerOffset;
            currentOffset_ = markerOffset;

            stats_.totalFreed.fetch_add(freedBytes, std::memory_order_relaxed);
            stats_.currentUsage.store(currentOffset_, std::memory_order_relaxed);
            stats_.freeCount.fetch_add(1, std::memory_order_relaxed);

            auto msg = std::format("StackAllocator '{}' rewound to marker. Freed {}",
                name_, FormatBytes(freedBytes));
            Channels::Engine().Info(msg);
        }
    }

    std::string name_;
    uint8_t* memory_;
    size_t totalSize_;
    size_t currentOffset_;
    MemoryStatistics stats_;
    mutable std::shared_mutex mutex_;
};

StackAllocator::StackAllocator(size_t size, const char* name)
    : pImpl_(std::make_unique<Impl>(size, name)) {
}

StackAllocator::~StackAllocator() = default;

StackAllocator::StackAllocator(StackAllocator&&) noexcept = default;
StackAllocator& StackAllocator::operator=(StackAllocator&&) noexcept = default;

void* StackAllocator::Allocate(size_t size, size_t alignment, const std::source_location& location) {
    return pImpl_->Allocate(size, alignment, location);
}

void StackAllocator::Deallocate(void* ptr, const std::source_location& location) {
    pImpl_->Deallocate(ptr, location);
}

const char* StackAllocator::GetName() const noexcept {
    return pImpl_->name_.c_str();
}

size_t StackAllocator::GetTotalSize() const noexcept {
    return pImpl_->totalSize_;
}

size_t StackAllocator::GetUsedSize() const noexcept {
    std::shared_lock<std::shared_mutex> lock(pImpl_->mutex_);
    return pImpl_->currentOffset_;
}

const MemoryStatistics& StackAllocator::GetStatistics() const noexcept {
    return pImpl_->stats_;
}

void StackAllocator::ResetStatistics() noexcept {
    pImpl_->stats_.Reset();
}

bool StackAllocator::OwnsPointer(void* ptr) const noexcept {
    return pImpl_->OwnsPointer(ptr);
}

void StackAllocator::Reset() {
    pImpl_->Reset();
}

StackAllocator::Marker StackAllocator::GetMarker() const noexcept {
    return pImpl_->GetMarker();
}

void StackAllocator::RewindToMarker(const Marker& marker) {
    pImpl_->RewindToMarker(marker);
}

// =============================================================================
// Heap Allocator Implementation
// =============================================================================

class HeapAllocator::Impl {
public:
    explicit Impl(const char* name) : name_(name) {
        auto msg = std::format("HeapAllocator '{}' created", name_);
        Channels::Engine().Info(msg);
    }

    ~Impl() {
        auto msg = std::format("HeapAllocator '{}' destroyed. Total allocated: {}, Peak usage: {}",
            name_,
            FormatBytes(stats_.GetTotalAllocated()),
            FormatBytes(stats_.GetPeakUsage()));
        Channels::Engine().Info(msg);
    }

    void* Allocate(size_t size, size_t alignment, const std::source_location& location) {
        if (size == 0) return nullptr;

        void* result = PlatformAlignedAlloc(size, alignment);
        if (!result) {
            auto msg = std::format("HeapAllocator '{}' failed to allocate {}",
                name_, FormatBytes(size));
            Channels::Engine().Error(msg);
            return nullptr;
        }

        const size_t actualSize = PlatformGetAllocSize(result);

        // Update statistics
        stats_.totalAllocated.fetch_add(actualSize, std::memory_order_relaxed);
        const size_t currentUsage = stats_.currentUsage.fetch_add(actualSize, std::memory_order_relaxed) + actualSize;
        stats_.allocationCount.fetch_add(1, std::memory_order_relaxed);
        stats_.activeAllocations.fetch_add(1, std::memory_order_relaxed);

        // Update peak usage
        size_t expectedPeak = stats_.peakUsage.load(std::memory_order_relaxed);
        while (currentUsage > expectedPeak &&
            !stats_.peakUsage.compare_exchange_weak(expectedPeak, currentUsage, std::memory_order_relaxed)) {
        }

        // Track allocation
        MemoryTracker::Instance().TrackAllocation(
            AllocationInfo(result, actualSize, alignment, name_.c_str(), location));

        return result;
    }

    void Deallocate(void* ptr, const std::source_location& location) {
        if (!ptr) return;

        const size_t size = PlatformGetAllocSize(ptr);

        PlatformAlignedFree(ptr);

        // Update statistics
        stats_.totalFreed.fetch_add(size, std::memory_order_relaxed);
        stats_.currentUsage.fetch_sub(size, std::memory_order_relaxed);
        stats_.freeCount.fetch_add(1, std::memory_order_relaxed);
        stats_.activeAllocations.fetch_sub(1, std::memory_order_relaxed);

        // Track deallocation
        MemoryTracker::Instance().TrackDeallocation(ptr, location);
    }

    void Reset() {
        // Heap allocator cannot reset all allocations
        auto msg = std::format("HeapAllocator '{}' reset requested but cannot free individual allocations", name_);
        Channels::Engine().Warning(msg);
    }

    bool OwnsPointer([[maybe_unused]] void* ptr) const {
        // Heap allocator cannot determine ownership
        return false;
    }

    std::string name_;
    MemoryStatistics stats_;
};

HeapAllocator::HeapAllocator(const char* name)
    : pImpl_(std::make_unique<Impl>(name)) {
}

HeapAllocator::~HeapAllocator() = default;

HeapAllocator::HeapAllocator(HeapAllocator&&) noexcept = default;
HeapAllocator& HeapAllocator::operator=(HeapAllocator&&) noexcept = default;

void* HeapAllocator::Allocate(size_t size, size_t alignment, const std::source_location& location) {
    return pImpl_->Allocate(size, alignment, location);
}

void HeapAllocator::Deallocate(void* ptr, const std::source_location& location) {
    pImpl_->Deallocate(ptr, location);
}

const char* HeapAllocator::GetName() const noexcept {
    return pImpl_->name_.c_str();
}

size_t HeapAllocator::GetTotalSize() const noexcept {
    return SIZE_MAX; // Unlimited
}

size_t HeapAllocator::GetUsedSize() const noexcept {
    return pImpl_->stats_.GetCurrentUsage();
}

const MemoryStatistics& HeapAllocator::GetStatistics() const noexcept {
    return pImpl_->stats_;
}

void HeapAllocator::ResetStatistics() noexcept {
    pImpl_->stats_.Reset();
}

bool HeapAllocator::OwnsPointer(void* ptr) const noexcept {
    return pImpl_->OwnsPointer(ptr);
}

void HeapAllocator::Reset() {
    pImpl_->Reset();
}

// =============================================================================
// Memory Manager Implementation
// =============================================================================

class MemoryManager::Impl {
public:
    Impl() = default;

    ~Impl() {
        Shutdown();
    }

    void Initialize() {
        std::lock_guard<std::shared_mutex> lock(mutex_);

        if (initialized_) return;

        // Create default allocators
        defaultAllocator_ = std::make_unique<HeapAllocator>("Default");
        engineAllocator_ = std::make_unique<LinearAllocator>(ENGINE_POOL_SIZE, "Engine");
        renderAllocator_ = std::make_unique<LinearAllocator>(RENDER_POOL_SIZE, "Render");
        resourceAllocator_ = std::make_unique<LinearAllocator>(RESOURCE_POOL_SIZE, "Resource");
        gameplayAllocator_ = std::make_unique<LinearAllocator>(GAMEPLAY_POOL_SIZE, "Gameplay");

        // Register default allocators
        allocators_["Default"] = defaultAllocator_.get();
        allocators_["Engine"] = engineAllocator_.get();
        allocators_["Render"] = renderAllocator_.get();
        allocators_["Resource"] = resourceAllocator_.get();
        allocators_["Gameplay"] = gameplayAllocator_.get();

        initialized_ = true;

        Channels::Engine().Info("MemoryManager initialized with default allocators");
    }

    void Shutdown() {
        std::lock_guard<std::shared_mutex> lock(mutex_);

        if (!initialized_) return;

        // Dump final statistics
        DumpMemoryStatistics();

        // Check for leaks
        DumpMemoryLeaks();

        // Clear allocators
        allocators_.clear();
        gameplayAllocator_.reset();
        resourceAllocator_.reset();
        renderAllocator_.reset();
        engineAllocator_.reset();
        defaultAllocator_.reset();

        initialized_ = false;

        Channels::Engine().Info("MemoryManager shutdown");
    }

    void RegisterAllocator(const char* name, std::unique_ptr<IAllocator> allocator) {
        std::lock_guard<std::shared_mutex> lock(mutex_);

        customAllocators_[name] = std::move(allocator);
        allocators_[name] = customAllocators_[name].get();

        auto msg = std::format("Registered custom allocator '{}'", name);
        Channels::Engine().Info(msg);
    }

    void UnregisterAllocator(const char* name) {
        std::lock_guard<std::shared_mutex> lock(mutex_);

        allocators_.erase(name);
        customAllocators_.erase(name);

        auto msg = std::format("Unregistered allocator '{}'", name);
        Channels::Engine().Info(msg);
    }

    IAllocator* GetAllocator(const char* name) {
        std::shared_lock<std::shared_mutex> lock(mutex_);

        auto it = allocators_.find(name);
        return it != allocators_.end() ? it->second : nullptr;
    }

    void* GlobalAllocate(size_t size, size_t alignment, const std::source_location& location) {
        return defaultAllocator_->Allocate(size, alignment, location);
    }

    void GlobalDeallocate(void* ptr, const std::source_location& location) {
        defaultAllocator_->Deallocate(ptr, location);
    }

    void EnableMemoryTracking(bool enable) {
        MemoryTracker::Instance().SetEnabled(enable);
        trackingEnabled_ = enable;

        auto msg = std::format("Memory tracking {}", enable ? "enabled" : "disabled");
        Channels::Engine().Info(msg);
    }

    bool IsMemoryTrackingEnabled() const {
        return trackingEnabled_;
    }

    void DumpMemoryLeaks() const {
        MemoryTracker::Instance().DumpLeaks();
    }

    void DumpMemoryStatistics() const {
        auto& logger = Channels::Engine();

        logger.Info("=== Global Memory Statistics ===");

        const auto globalStats = GetGlobalStatistics();
        auto msg = std::format("Total allocated: {}", FormatBytes(globalStats.GetTotalAllocated()));
        logger.Info(msg);
        msg = std::format("Total freed: {}", FormatBytes(globalStats.totalFreed.load()));
        logger.Info(msg);
        msg = std::format("Current usage: {}", FormatBytes(globalStats.GetCurrentUsage()));
        logger.Info(msg);
        msg = std::format("Peak usage: {}", FormatBytes(globalStats.GetPeakUsage()));
        logger.Info(msg);
        msg = std::format("Active allocations: {}", globalStats.GetActiveAllocations());
        logger.Info(msg);
        msg = std::format("Allocation count: {}", globalStats.allocationCount.load());
        logger.Info(msg);
        msg = std::format("Free count: {}", globalStats.freeCount.load());
        logger.Info(msg);

        if (MemoryTracker::Instance().IsEnabled()) {
            auto msg1 = std::format("Tracked allocations: {}", MemoryTracker::Instance().GetTrackedAllocationCount());
            logger.Info(msg1);
        }
    }

    void DumpAllocatorStatistics() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);

        auto& logger = Channels::Engine();
        logger.Info("=== Allocator Statistics ===");

        for (const auto& [name, allocator] : allocators_) {
            const auto& stats = allocator->GetStatistics();
            auto msg = std::format("Allocator '{}': {} / {} used, {} peak, {} allocations",
                name,
                FormatBytes(allocator->GetUsedSize()),
                FormatBytes(allocator->GetTotalSize()),
                FormatBytes(stats.GetPeakUsage()),
                stats.GetActiveAllocations());
            logger.Info(msg);
        }
    }

    MemoryStatistics GetGlobalStatistics() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);

        MemoryStatistics combined;

        for (const auto& [name, allocator] : allocators_) {
            const auto& stats = allocator->GetStatistics();

            // Manually aggregate atomic values
            combined.totalAllocated.store(combined.totalAllocated.load() + stats.GetTotalAllocated(), std::memory_order_relaxed);
            combined.totalFreed.store(combined.totalFreed.load() + stats.totalFreed.load(), std::memory_order_relaxed);
            combined.currentUsage.store(combined.currentUsage.load() + stats.GetCurrentUsage(), std::memory_order_relaxed);
            combined.allocationCount.store(combined.allocationCount.load() + stats.allocationCount.load(), std::memory_order_relaxed);
            combined.freeCount.store(combined.freeCount.load() + stats.freeCount.load(), std::memory_order_relaxed);
            combined.activeAllocations.store(combined.activeAllocations.load() + stats.GetActiveAllocations(), std::memory_order_relaxed);

            // Update peak usage with maximum
            const size_t peak = stats.GetPeakUsage();
            size_t expectedPeak = combined.peakUsage.load(std::memory_order_relaxed);
            while (peak > expectedPeak &&
                !combined.peakUsage.compare_exchange_weak(expectedPeak, peak, std::memory_order_relaxed)) {
            }
        }

        return combined;
    }

    void ResetGlobalStatistics() {
        std::shared_lock<std::shared_mutex> lock(mutex_);

        for (const auto& [name, allocator] : allocators_) {
            allocator->ResetStatistics();
        }

        Channels::Engine().Info("Global memory statistics reset");
    }

    mutable std::shared_mutex mutex_;
    bool initialized_ = false;
    bool trackingEnabled_ = true;

    // Default allocators
    std::unique_ptr<IAllocator> defaultAllocator_;
    std::unique_ptr<IAllocator> engineAllocator_;
    std::unique_ptr<IAllocator> renderAllocator_;
    std::unique_ptr<IAllocator> resourceAllocator_;
    std::unique_ptr<IAllocator> gameplayAllocator_;

    // Custom allocators
    std::unordered_map<std::string, std::unique_ptr<IAllocator>> customAllocators_;

    // All allocators (for lookup)
    std::unordered_map<std::string, IAllocator*> allocators_;
};

MemoryManager& MemoryManager::Instance() noexcept {
    static MemoryManager instance;
    return instance;
}

MemoryManager::MemoryManager() : pImpl_(std::make_unique<Impl>()) {}

void MemoryManager::Initialize() {
    pImpl_->Initialize();
}

void MemoryManager::Shutdown() {
    pImpl_->Shutdown();
}

void MemoryManager::RegisterAllocator(const char* name, std::unique_ptr<IAllocator> allocator) {
    pImpl_->RegisterAllocator(name, std::move(allocator));
}

void MemoryManager::UnregisterAllocator(const char* name) {
    pImpl_->UnregisterAllocator(name);
}

IAllocator* MemoryManager::GetAllocator(const char* name) noexcept {
    return pImpl_->GetAllocator(name);
}

IAllocator& MemoryManager::GetDefaultAllocator() noexcept {
    return *pImpl_->defaultAllocator_;
}

IAllocator& MemoryManager::GetEngineAllocator() noexcept {
    return *pImpl_->engineAllocator_;
}

IAllocator& MemoryManager::GetRenderAllocator() noexcept {
    return *pImpl_->renderAllocator_;
}

IAllocator& MemoryManager::GetResourceAllocator() noexcept {
    return *pImpl_->resourceAllocator_;
}

IAllocator& MemoryManager::GetGameplayAllocator() noexcept {
    return *pImpl_->gameplayAllocator_;
}

void* MemoryManager::GlobalAllocate(size_t size, size_t alignment, const std::source_location& location) {
    return pImpl_->GlobalAllocate(size, alignment, location);
}

void MemoryManager::GlobalDeallocate(void* ptr, const std::source_location& location) {
    pImpl_->GlobalDeallocate(ptr, location);
}

void MemoryManager::EnableMemoryTracking(bool enable) noexcept {
    pImpl_->EnableMemoryTracking(enable);
}

bool MemoryManager::IsMemoryTrackingEnabled() const noexcept {
    return pImpl_->IsMemoryTrackingEnabled();
}

void MemoryManager::DumpMemoryLeaks() const {
    pImpl_->DumpMemoryLeaks();
}

void MemoryManager::DumpMemoryStatistics() const {
    pImpl_->DumpMemoryStatistics();
}

void MemoryManager::DumpAllocatorStatistics() const {
    pImpl_->DumpAllocatorStatistics();
}

MemoryStatistics MemoryManager::GetGlobalStatistics() const noexcept {
    return pImpl_->GetGlobalStatistics();
}

void MemoryManager::ResetGlobalStatistics() noexcept {
    pImpl_->ResetGlobalStatistics();
}

// =============================================================================
// Utility Functions Implementation
// =============================================================================

void Akhanda::Memory::MemSet(void* dest, int value, size_t count) noexcept {
    std::memset(dest, value, count);
}

void Akhanda::Memory::MemCopy(void* dest, const void* src, size_t count) noexcept {
    std::memcpy(dest, src, count);
}

void Akhanda::Memory::MemMove(void* dest, const void* src, size_t count) noexcept {
    std::memmove(dest, src, count);
}

int Akhanda::Memory::MemCompare(const void* ptr1, const void* ptr2, size_t count) noexcept {
    return std::memcmp(ptr1, ptr2, count);
}

void Akhanda::Memory::SecureNoMemory(void* ptr, size_t count) noexcept {
    ::SecureZeroMemory(ptr, count);
}

String Akhanda::Memory::FormatBytes(size_t bytes) {
    std::ostringstream oss;

    if (bytes >= GB) {
        oss << std::fixed << std::setprecision(2) << (static_cast<double>(bytes) / GB) << " GB";
    }
    else if (bytes >= MB) {
        oss << std::fixed << std::setprecision(2) << (static_cast<double>(bytes) / MB) << " MB";
    }
    else if (bytes >= KB) {
        oss << std::fixed << std::setprecision(2) << (static_cast<double>(bytes) / KB) << " KB";
    }
    else {
        oss << bytes << " bytes";
    }

    return String(oss.str().c_str(), STLAllocator<char>());
}

String Akhanda::Memory::FormatBytesDetailed(size_t bytes) {
    std::ostringstream oss;
    oss << FormatBytes(bytes).c_str() << " (" << bytes << " bytes)";
    return String(oss.str().c_str(), STLAllocator<char>());
}