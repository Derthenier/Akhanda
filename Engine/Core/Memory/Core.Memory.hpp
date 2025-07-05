// Core.Memory.hpp - Memory Macros Header
#pragma once

// Import the memory module
import Akhanda.Core.Memory;



// =============================================================================
// Memory Debugging Macros (in header for inlining)
// =============================================================================

#ifdef AKH_DEBUG
#define AKH_ALLOCATE(allocator, size) \
        (allocator).Allocate((size), ::Akhanda::Memory::DEFAULT_ALIGNMENT, std::source_location::current())

#define AKH_ALLOCATE_ALIGNED(allocator, size, alignment) \
        (allocator).Allocate((size), (alignment), std::source_location::current())

#define AKH_DEALLOCATE(allocator, ptr) \
        (allocator).Deallocate((ptr), std::source_location::current())

#define AKH_NEW(allocator, type, ...) \
        ::Akhanda::Memory::MakeUnique<type>((allocator), ##__VA_ARGS__, std::source_location::current())

#define AKH_SHARED(allocator, type, ...) \
        ::Akhanda::Memory::MakeShared<type>((allocator), ##__VA_ARGS__, std::source_location::current())

#define AKH_GLOBAL_ALLOCATE(size) \
        ::Akhanda::Memory::MemoryManager::Instance().GlobalAllocate((size), ::Akhanda::Memory::DEFAULT_ALIGNMENT, std::source_location::current())

#define AKH_GLOBAL_DEALLOCATE(ptr) \
        ::Akhanda::Memory::MemoryManager::Instance().GlobalDeallocate((ptr), std::source_location::current())
#else
#define AKH_ALLOCATE(allocator, size) \
        (allocator).Allocate((size))

#define AKH_ALLOCATE_ALIGNED(allocator, size, alignment) \
        (allocator).Allocate((size), (alignment))

#define AKH_DEALLOCATE(allocator, ptr) \
        (allocator).Deallocate((ptr))

#define AKH_NEW(allocator, type, ...) \
        ::Akhanda::Memory::MakeUnique<type>((allocator), ##__VA_ARGS__)

#define AKH_SHARED(allocator, type, ...) \
        ::Akhanda::Memory::MakeShared<type>((allocator), ##__VA_ARGS__)

#define AKH_GLOBAL_ALLOCATE(size) \
        ::Akhanda::Memory::MemoryManager::Instance().GlobalAllocate((size))

#define AKH_GLOBAL_DEALLOCATE(ptr) \
        ::Akhanda::Memory::MemoryManager::Instance().GlobalDeallocate((ptr))
#endif
