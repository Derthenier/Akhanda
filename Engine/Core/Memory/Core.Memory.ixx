// Core.Memory.ixx - Memory Management Module Interface
module;

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <source_location>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

export module Akhanda.Core.Memory;

import Akhanda.Core.Logging;

// =============================================================================
// Forward Declarations
// =============================================================================

export namespace Akhanda::Memory {
    class IAllocator;
    class LinearAllocator;
    class PoolAllocator;
    class StackAllocator;
    class HeapAllocator;
    class MemoryPool;

    template<typename T>
    class STLAllocator;
}

// =============================================================================
// Constants
// =============================================================================

export namespace Akhanda::Memory {
    // Memory alignment constants
    inline constexpr size_t DEFAULT_ALIGNMENT = 16;  // SIMD-friendly alignment
    inline constexpr size_t MAX_ALIGNMENT = 64;      // Cache line alignment
    inline constexpr size_t MIN_ALIGNMENT = 8;       // Minimum platform alignment

    // Memory pool sizes
    inline constexpr size_t KB = 1024;
    inline constexpr size_t MB = KB * 1024;
    inline constexpr size_t GB = MB * 1024;

    // Default pool sizes for engine systems
    inline constexpr size_t ENGINE_POOL_SIZE = 64 * MB;
    inline constexpr size_t RENDER_POOL_SIZE = 128 * MB;
    inline constexpr size_t RESOURCE_POOL_SIZE = 256 * MB;
    inline constexpr size_t GAMEPLAY_POOL_SIZE = 32 * MB;

    // Memory tracking constants
    inline constexpr size_t MAX_TRACKED_ALLOCATIONS = 100000;
    inline constexpr size_t ALLOCATION_HISTORY_SIZE = 1000;
}


// =============================================================================
// Memory Allocation Information
// =============================================================================

export namespace Akhanda::Memory {
    struct AllocationInfo {
        void* ptr = nullptr;
        size_t size = 0;
        size_t alignment = DEFAULT_ALIGNMENT;
        std::source_location location;
        std::chrono::high_resolution_clock::time_point timestamp;
        const char* allocatorName = "Unknown";

        AllocationInfo() = default;
        AllocationInfo(void* p, size_t s, size_t a, const char* name,
            const std::source_location& loc = std::source_location::current())
            : ptr(p), size(s), alignment(a), location(loc),
            timestamp(std::chrono::high_resolution_clock::now()), allocatorName(name) {
        }
    };

    struct MemoryStatistics {
        std::atomic<size_t> totalAllocated{ 0 };
        std::atomic<size_t> totalFreed{ 0 };
        std::atomic<size_t> currentUsage{ 0 };
        std::atomic<size_t> peakUsage{ 0 };
        std::atomic<size_t> allocationCount{ 0 };
        std::atomic<size_t> freeCount{ 0 };
        std::atomic<size_t> activeAllocations{ 0 };

        // Default constructor
        MemoryStatistics() = default;

        // Copy constructor - reads atomic values
        MemoryStatistics(const MemoryStatistics& other) noexcept
            : totalAllocated(other.totalAllocated.load())
            , totalFreed(other.totalFreed.load())
            , currentUsage(other.currentUsage.load())
            , peakUsage(other.peakUsage.load())
            , allocationCount(other.allocationCount.load())
            , freeCount(other.freeCount.load())
            , activeAllocations(other.activeAllocations.load()) {
        }

        // Copy assignment operator
        MemoryStatistics& operator=(const MemoryStatistics& other) noexcept {
            if (this != &other) {
                totalAllocated.store(other.totalAllocated.load());
                totalFreed.store(other.totalFreed.load());
                currentUsage.store(other.currentUsage.load());
                peakUsage.store(other.peakUsage.load());
                allocationCount.store(other.allocationCount.load());
                freeCount.store(other.freeCount.load());
                activeAllocations.store(other.activeAllocations.load());
            }
            return *this;
        }

        // Move constructor
        MemoryStatistics(MemoryStatistics&& other) noexcept
            : totalAllocated(other.totalAllocated.load())
            , totalFreed(other.totalFreed.load())
            , currentUsage(other.currentUsage.load())
            , peakUsage(other.peakUsage.load())
            , allocationCount(other.allocationCount.load())
            , freeCount(other.freeCount.load())
            , activeAllocations(other.activeAllocations.load()) {
            // Reset source atomics to zero
            other.totalAllocated.store(0);
            other.totalFreed.store(0);
            other.currentUsage.store(0);
            other.peakUsage.store(0);
            other.allocationCount.store(0);
            other.freeCount.store(0);
            other.activeAllocations.store(0);
        }

        // Move assignment operator
        MemoryStatistics& operator=(MemoryStatistics&& other) noexcept {
            if (this != &other) {
                totalAllocated.store(other.totalAllocated.load());
                totalFreed.store(other.totalFreed.load());
                currentUsage.store(other.currentUsage.load());
                peakUsage.store(other.peakUsage.load());
                allocationCount.store(other.allocationCount.load());
                freeCount.store(other.freeCount.load());
                activeAllocations.store(other.activeAllocations.load());

                // Reset source atomics to zero
                other.totalAllocated.store(0);
                other.totalFreed.store(0);
                other.currentUsage.store(0);
                other.peakUsage.store(0);
                other.allocationCount.store(0);
                other.freeCount.store(0);
                other.activeAllocations.store(0);
            }
            return *this;
        }

        // Get current memory usage
        size_t GetCurrentUsage() const noexcept { return currentUsage.load(); }
        size_t GetPeakUsage() const noexcept { return peakUsage.load(); }
        size_t GetTotalAllocated() const noexcept { return totalAllocated.load(); }
        size_t GetActiveAllocations() const noexcept { return activeAllocations.load(); }

        // Reset statistics
        void Reset() noexcept;
    };
}

// =============================================================================
// Allocator Interface
// =============================================================================

export namespace Akhanda::Memory {
    class IAllocator {
    public:
        virtual ~IAllocator() = default;

        // Core allocation interface
        virtual void* Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT,
            const std::source_location& location = std::source_location::current()) = 0;
        virtual void Deallocate(void* ptr,
            const std::source_location& location = std::source_location::current()) = 0;

        // Allocator information
        virtual const char* GetName() const noexcept = 0;
        virtual size_t GetTotalSize() const noexcept = 0;
        virtual size_t GetUsedSize() const noexcept = 0;
        virtual size_t GetFreeSize() const noexcept { return GetTotalSize() - GetUsedSize(); }
        virtual bool IsThreadSafe() const noexcept = 0;

        // Statistics
        virtual const MemoryStatistics& GetStatistics() const noexcept = 0;
        virtual void ResetStatistics() noexcept = 0;

        // Utility functions
        virtual bool OwnsPointer(void* ptr) const noexcept = 0;
        virtual void Reset() = 0;  // Clear all allocations

        // Template convenience functions
        template<typename T>
        T* Allocate(size_t count = 1, const std::source_location& location = std::source_location::current()) {
            const size_t size = sizeof(T) * count;
            const size_t alignment = std::max(alignof(T), DEFAULT_ALIGNMENT);
            return static_cast<T*>(Allocate(size, alignment, location));
        }

        template<typename T>
        void Deallocate(T* ptr, const std::source_location& location = std::source_location::current()) {
            Deallocate(static_cast<void*>(ptr), location);
        }
    };
}

// =============================================================================
// Specific Allocator Types
// =============================================================================

export namespace Akhanda::Memory {
    // Linear allocator - fast allocation, no individual deallocation
    class LinearAllocator : public IAllocator {
    public:
        explicit LinearAllocator(size_t size, const char* name = "LinearAllocator");
        ~LinearAllocator() override;

        // Non-copyable, movable
        LinearAllocator(const LinearAllocator&) = delete;
        LinearAllocator& operator=(const LinearAllocator&) = delete;
        LinearAllocator(LinearAllocator&&) noexcept;
        LinearAllocator& operator=(LinearAllocator&&) noexcept;

        // IAllocator implementation
        void* Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT,
            const std::source_location& location = std::source_location::current()) override;
        void Deallocate(void* ptr,
            const std::source_location& location = std::source_location::current()) override;

        const char* GetName() const noexcept override;
        size_t GetTotalSize() const noexcept override;
        size_t GetUsedSize() const noexcept override;
        bool IsThreadSafe() const noexcept override { return true; }

        const MemoryStatistics& GetStatistics() const noexcept override;
        void ResetStatistics() noexcept override;
        bool OwnsPointer(void* ptr) const noexcept override;
        void Reset() override;

        // Linear allocator specific
        void* GetBasePtr() const noexcept;
        size_t GetOffset() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> pImpl_;
    };

    // Pool allocator - fixed-size block allocation
    class PoolAllocator : public IAllocator {
    public:
        PoolAllocator(size_t blockSize, size_t blockCount, size_t alignment = DEFAULT_ALIGNMENT,
            const char* name = "PoolAllocator");
        ~PoolAllocator() override;

        // Non-copyable, movable
        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;
        PoolAllocator(PoolAllocator&&) noexcept;
        PoolAllocator& operator=(PoolAllocator&&) noexcept;

        // IAllocator implementation
        void* Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT,
            const std::source_location& location = std::source_location::current()) override;
        void Deallocate(void* ptr,
            const std::source_location& location = std::source_location::current()) override;

        const char* GetName() const noexcept override;
        size_t GetTotalSize() const noexcept override;
        size_t GetUsedSize() const noexcept override;
        bool IsThreadSafe() const noexcept override { return true; }

        const MemoryStatistics& GetStatistics() const noexcept override;
        void ResetStatistics() noexcept override;
        bool OwnsPointer(void* ptr) const noexcept override;
        void Reset() override;

        // Pool allocator specific
        size_t GetBlockSize() const noexcept;
        size_t GetBlockCount() const noexcept;
        size_t GetFreeBlocks() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> pImpl_;
    };

    // Stack allocator - LIFO allocation/deallocation
    class StackAllocator : public IAllocator {
    public:
        explicit StackAllocator(size_t size, const char* name = "StackAllocator");
        ~StackAllocator() override;

        // Non-copyable, movable
        StackAllocator(const StackAllocator&) = delete;
        StackAllocator& operator=(const StackAllocator&) = delete;
        StackAllocator(StackAllocator&&) noexcept;
        StackAllocator& operator=(StackAllocator&&) noexcept;

        // IAllocator implementation
        void* Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT,
            const std::source_location& location = std::source_location::current()) override;
        void Deallocate(void* ptr,
            const std::source_location& location = std::source_location::current()) override;

        const char* GetName() const noexcept override;
        size_t GetTotalSize() const noexcept override;
        size_t GetUsedSize() const noexcept override;
        bool IsThreadSafe() const noexcept override { return true; }

        const MemoryStatistics& GetStatistics() const noexcept override;
        void ResetStatistics() noexcept override;
        bool OwnsPointer(void* ptr) const noexcept override;
        void Reset() override;

        // Stack allocator specific
        class Marker {
        public:
            explicit Marker(size_t offset) : offset_(offset) {}
            size_t GetOffset() const noexcept { return offset_; }
        private:
            size_t offset_;
        };

        Marker GetMarker() const noexcept;
        void RewindToMarker(const Marker& marker);

    private:
        class Impl;
        std::unique_ptr<Impl> pImpl_;
    };

    // Heap allocator - general purpose malloc/free wrapper
    class HeapAllocator : public IAllocator {
    public:
        explicit HeapAllocator(const char* name = "HeapAllocator");
        ~HeapAllocator() override;

        // Non-copyable, movable
        HeapAllocator(const HeapAllocator&) = delete;
        HeapAllocator& operator=(const HeapAllocator&) = delete;
        HeapAllocator(HeapAllocator&&) noexcept;
        HeapAllocator& operator=(HeapAllocator&&) noexcept;

        // IAllocator implementation
        void* Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT,
            const std::source_location& location = std::source_location::current()) override;
        void Deallocate(void* ptr,
            const std::source_location& location = std::source_location::current()) override;

        const char* GetName() const noexcept override;
        size_t GetTotalSize() const noexcept override;
        size_t GetUsedSize() const noexcept override;
        bool IsThreadSafe() const noexcept override { return true; }

        const MemoryStatistics& GetStatistics() const noexcept override;
        void ResetStatistics() noexcept override;
        bool OwnsPointer(void* ptr) const noexcept override;
        void Reset() override;

    private:
        class Impl;
        std::unique_ptr<Impl> pImpl_;
    };
}

// =============================================================================
// Memory Manager
// =============================================================================

export namespace Akhanda::Memory {
    class MemoryManager {
    public:
        static MemoryManager& Instance() noexcept;

        // Non-copyable, non-movable singleton
        MemoryManager(const MemoryManager&) = delete;
        MemoryManager& operator=(const MemoryManager&) = delete;
        MemoryManager(MemoryManager&&) = delete;
        MemoryManager& operator=(MemoryManager&&) = delete;

        // Lifecycle
        void Initialize();
        void Shutdown();

        // Allocator management
        void RegisterAllocator(const char* name, std::unique_ptr<IAllocator> allocator);
        void UnregisterAllocator(const char* name);
        IAllocator* GetAllocator(const char* name) noexcept;

        // Default allocators
        IAllocator& GetDefaultAllocator() noexcept;
        IAllocator& GetEngineAllocator() noexcept;
        IAllocator& GetRenderAllocator() noexcept;
        IAllocator& GetResourceAllocator() noexcept;
        IAllocator& GetGameplayAllocator() noexcept;

        // Global memory operations
        void* GlobalAllocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT,
            const std::source_location& location = std::source_location::current());
        void GlobalDeallocate(void* ptr,
            const std::source_location& location = std::source_location::current());

        // Memory tracking and debugging
        void EnableMemoryTracking(bool enable) noexcept;
        bool IsMemoryTrackingEnabled() const noexcept;
        void DumpMemoryLeaks() const;
        void DumpMemoryStatistics() const;
        void DumpAllocatorStatistics() const;

        // Global statistics
        MemoryStatistics GetGlobalStatistics() const noexcept;
        void ResetGlobalStatistics() noexcept;

    private:
        MemoryManager();
        ~MemoryManager() = default;

        class Impl;
        std::unique_ptr<Impl> pImpl_;
    };
}

// =============================================================================
// Smart Pointers
// =============================================================================

export namespace Akhanda::Memory {
    template<typename T>
    struct CustomDeleter {
        IAllocator* allocator = nullptr;
        std::source_location location;

        CustomDeleter() = default;
        explicit CustomDeleter(IAllocator* alloc,
            const std::source_location& loc = std::source_location::current())
            : allocator(alloc), location(loc) {
        }

        void operator()(T* ptr) {
            if (ptr && allocator) {
                ptr->~T();
                allocator->Deallocate(ptr, location);
            }
        }
    };

    template<typename T>
    using UniquePtr = std::unique_ptr<T, CustomDeleter<T>>;

    template<typename T>
    using SharedPtr = std::shared_ptr<T>;

    // Smart pointer creation functions
    template<typename T, typename... Args>
    UniquePtr<T> MakeUnique(IAllocator& allocator, Args&&... args,
        const std::source_location& location = std::source_location::current()) {
        T* ptr = allocator.template Allocate<T>(1, location);
        if (!ptr) {
            return nullptr;
        }

        try {
            new(ptr) T(std::forward<Args>(args)...);
            return UniquePtr<T>(ptr, CustomDeleter<T>(&allocator, location));
        }
        catch (...) {
            allocator.Deallocate(ptr, location);
            throw;
        }
    }

    template<typename T, typename... Args>
    SharedPtr<T> MakeShared(IAllocator& allocator, Args&&... args,
        const std::source_location& location = std::source_location::current()) {
        T* ptr = allocator.template Allocate<T>(1, location);
        if (!ptr) {
            return nullptr;
        }

        try {
            new(ptr) T(std::forward<Args>(args)...);
            return SharedPtr<T>(ptr, [&allocator, location](T* p) {
                if (p) {
                    p->~T();
                    allocator.Deallocate(p, location);
                }
                });
        }
        catch (...) {
            allocator.Deallocate(ptr, location);
            throw;
        }
    }
}

// =============================================================================
// STL Allocator Adapter
// =============================================================================

export namespace Akhanda::Memory {
    template<typename T>
    class STLAllocator {
    public:
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        template<typename U>
        struct rebind {
            using other = STLAllocator<U>;
        };

        STLAllocator() noexcept : allocator_(&MemoryManager::Instance().GetDefaultAllocator()) {}
        explicit STLAllocator(IAllocator& allocator) noexcept : allocator_(&allocator) {}

        template<typename U>
        STLAllocator(const STLAllocator<U>& other) noexcept : allocator_(other.allocator_) {}

        pointer allocate(size_type n, const std::source_location& location = std::source_location::current()) {
            if (n > std::numeric_limits<size_type>::max() / sizeof(T)) {
                throw std::bad_alloc();
            }

            const size_t size = n * sizeof(T);
            const size_t alignment = std::max(alignof(T), DEFAULT_ALIGNMENT);

            pointer result = static_cast<pointer>(allocator_->Allocate(size, alignment, location));
            if (!result) {
                throw std::bad_alloc();
            }

            return result;
        }

        void deallocate(pointer ptr, size_type,
            const std::source_location& location = std::source_location::current()) {
            allocator_->Deallocate(ptr, location);
        }

        template<typename U>
        bool operator==(const STLAllocator<U>& other) const noexcept {
            return allocator_ == other.allocator_;
        }

        template<typename U>
        bool operator!=(const STLAllocator<U>& other) const noexcept {
            return !(*this == other);
        }

        IAllocator* GetAllocator() const noexcept { return allocator_; }

    private:
        template<typename U> friend class STLAllocator;
        IAllocator* allocator_;
    };
}

// =============================================================================
// Memory-Aware Container Aliases
// =============================================================================

export namespace Akhanda::Memory {
    template<typename T>
    using Vector = std::vector<T, STLAllocator<T>>;

    template<typename T, typename Compare = std::less<T>>
    using Set = std::set<T, Compare, STLAllocator<T>>;

    template<typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
    using HashMap = std::unordered_map<Key, Value, Hash, Equal, STLAllocator<std::pair<const Key, Value>>>;

    template<typename Key, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
    using HashSet = std::unordered_set<Key, Hash, Equal, STLAllocator<Key>>;

    template<typename T>
    using Deque = std::deque<T, STLAllocator<T>>;

    template<typename T>
    using List = std::list<T, STLAllocator<T>>;

    using String = std::basic_string<char, std::char_traits<char>, STLAllocator<char>>;
    using WString = std::basic_string<wchar_t, std::char_traits<wchar_t>, STLAllocator<wchar_t>>;
}

// =============================================================================
// Utility Functions
// =============================================================================

export namespace Akhanda::Memory {
    // Alignment utilities
    constexpr size_t AlignUp(size_t value, size_t alignment) noexcept {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    constexpr size_t AlignDown(size_t value, size_t alignment) noexcept {
        return value & ~(alignment - 1);
    }

    bool IsAligned(const void* ptr, size_t alignment) noexcept {
        return (reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)) == 0;
    }

    constexpr bool IsPowerOfTwo(size_t value) noexcept {
        return value > 0 && (value & (value - 1)) == 0;
    }

    // Memory utilities
    void MemSet(void* dest, int value, size_t count) noexcept;
    void MemCopy(void* dest, const void* src, size_t count) noexcept;
    void MemMove(void* dest, const void* src, size_t count) noexcept;
    int MemCompare(const void* ptr1, const void* ptr2, size_t count) noexcept;
    void SecureNoMemory(void* ptr, size_t count) noexcept;

    // Size formatting for logging
    String FormatBytes(size_t bytes);
    String FormatBytesDetailed(size_t bytes);
}
