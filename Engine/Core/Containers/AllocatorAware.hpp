// AllocatorAware.hpp
// Akhanda Game Engine - Allocator-Aware Container Base
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include "ContainerTraits.hpp"
#include <memory>
#include <utility>

namespace Akhanda::Containers {

    // Default allocator that integrates with engine memory system
    template<typename T>
    class DefaultAllocator {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;

        template<typename U>
        struct rebind {
            using other = DefaultAllocator<U>;
        };

        DefaultAllocator() noexcept = default;

        template<typename U>
        DefaultAllocator(const DefaultAllocator<U>&) noexcept {}

        [[nodiscard]] pointer allocate(size_type n) {
            if (n > max_size()) {
                throw std::bad_array_new_length();
            }

            const size_type alignment = Traits::OptimalAlignment_v<T>;
            const size_type size = n * sizeof(T);

            // Use engine's memory allocator with proper alignment
            void* ptr = Memory::AlignedAlloc(size, alignment);
            if (!ptr) {
                throw std::bad_alloc();
            }

            return static_cast<pointer>(ptr);
        }

        void deallocate(pointer p, size_type n) noexcept {
            if (p) {
                Memory::AlignedFree(p);
            }
        }

        [[nodiscard]] constexpr size_type max_size() const noexcept {
            return std::numeric_limits<size_type>::max() / sizeof(T);
        }

        template<typename U, typename... Args>
        void construct(U* p, Args&&... args) {
            new (p) U(std::forward<Args>(args)...);
        }

        template<typename U>
        void destroy(U* p) noexcept {
            p->~U();
        }

        bool operator==(const DefaultAllocator&) const noexcept { return true; }
        bool operator!=(const DefaultAllocator&) const noexcept { return false; }
    };

    // Pool allocator for frequent small allocations
    template<typename T, std::size_t PoolSize = 1024>
    class PoolAllocator {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using const_pointer = const T*;

        template<typename U>
        struct rebind {
            using other = PoolAllocator<U, PoolSize>;
        };

        PoolAllocator() noexcept = default;

        template<typename U>
        PoolAllocator(const PoolAllocator<U, PoolSize>&) noexcept {}

        [[nodiscard]] pointer allocate(size_type n) {
            // For small allocations, use pool. For large ones, fall back to default.
            if (n * sizeof(T) <= PoolSize && pool_available(n)) {
                return allocate_from_pool(n);
            }

            return DefaultAllocator<T>{}.allocate(n);
        }

        void deallocate(pointer p, size_type n) noexcept {
            if (is_from_pool(p)) {
                deallocate_to_pool(p, n);
            }
            else {
                DefaultAllocator<T>{}.deallocate(p, n);
            }
        }

        [[nodiscard]] constexpr size_type max_size() const noexcept {
            return DefaultAllocator<T>{}.max_size();
        }

        bool operator==(const PoolAllocator&) const noexcept { return true; }
        bool operator!=(const PoolAllocator&) const noexcept { return false; }

    private:
        // Pool implementation would go here
        // For now, delegate to default allocator
        pointer allocate_from_pool(size_type n) {
            return DefaultAllocator<T>{}.allocate(n);
        }

        void deallocate_to_pool(pointer p, size_type n) noexcept {
            DefaultAllocator<T>{}.deallocate(p, n);
        }

        bool pool_available(size_type n) const noexcept {
            return false; // Simplified for now
        }

        bool is_from_pool(pointer p) const noexcept {
            return false; // Simplified for now
        }
    };

    // Base class for allocator-aware containers
    template<typename T, typename Allocator = DefaultAllocator<T>>
    class AllocatorAwareBase {
    public:
        using allocator_type = Allocator;
        using value_type = T;
        using size_type = typename std::allocator_traits<Allocator>::size_type;
        using difference_type = typename std::allocator_traits<Allocator>::difference_type;
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
        using reference = T&;
        using const_reference = const T&;

    protected:
        allocator_type allocator_;

        // Helper aliases for allocator traits
        using AllocTraits = std::allocator_traits<allocator_type>;

        explicit AllocatorAwareBase(const allocator_type& alloc = allocator_type{})
            : allocator_(alloc) {
        }

        AllocatorAwareBase(const AllocatorAwareBase& other)
            : allocator_(AllocTraits::select_on_container_copy_construction(other.allocator_)) {
        }

        AllocatorAwareBase(AllocatorAwareBase&& other) noexcept
            : allocator_(std::move(other.allocator_)) {
        }

        AllocatorAwareBase& operator=(const AllocatorAwareBase& other) {
            if constexpr (AllocTraits::propagate_on_container_copy_assignment::value) {
                if (allocator_ != other.allocator_) {
                    // Need to deallocate with old allocator before assignment
                    clear_and_deallocate();
                    allocator_ = other.allocator_;
                }
            }
            return *this;
        }

        AllocatorAwareBase& operator=(AllocatorAwareBase&& other) noexcept {
            if constexpr (AllocTraits::propagate_on_container_move_assignment::value) {
                allocator_ = std::move(other.allocator_);
            }
            return *this;
        }

        // Allocation helpers
        [[nodiscard]] pointer allocate(size_type n) {
            return AllocTraits::allocate(allocator_, n);
        }

        void deallocate(pointer p, size_type n) noexcept {
            if (p) {
                AllocTraits::deallocate(allocator_, p, n);
            }
        }

        template<typename... Args>
        void construct(pointer p, Args&&... args) {
            AllocTraits::construct(allocator_, p, std::forward<Args>(args)...);
        }

        void destroy(pointer p) noexcept {
            AllocTraits::destroy(allocator_, p);
        }

        // Bulk operations with optimization
        void construct_range(pointer first, size_type count) {
            if constexpr (std::is_default_constructible_v<T>) {
                for (size_type i = 0; i < count; ++i) {
                    construct(first + i);
                }
            }
        }

        void construct_range(pointer first, size_type count, const T& value) {
            for (size_type i = 0; i < count; ++i) {
                construct(first + i, value);
            }
        }

        template<typename InputIt>
        void construct_range(pointer first, InputIt src_first, InputIt src_last) {
            pointer current = first;
            for (auto it = src_first; it != src_last; ++it, ++current) {
                construct(current, *it);
            }
        }

        void destroy_range(pointer first, pointer last) noexcept {
            for (pointer p = first; p != last; ++p) {
                destroy(p);
            }
        }

        // Move/copy helpers with optimization
        void move_construct_range(pointer dest, pointer src, size_type count) {
            if constexpr (Traits::IsTriviallyRelocatable_v<T>) {
                std::memcpy(dest, src, count * sizeof(T));
            }
            else {
                for (size_type i = 0; i < count; ++i) {
                    construct(dest + i, std::move(src[i]));
                }
            }
        }

        void copy_construct_range(pointer dest, const_pointer src, size_type count) {
            if constexpr (std::is_trivially_copy_constructible_v<T>) {
                std::memcpy(dest, src, count * sizeof(T));
            }
            else {
                for (size_type i = 0; i < count; ++i) {
                    construct(dest + i, src[i]);
                }
            }
        }

        // Swap with allocator propagation
        void swap_allocator_aware(AllocatorAwareBase& other) noexcept {
            if constexpr (AllocTraits::propagate_on_container_swap::value) {
                std::swap(allocator_, other.allocator_);
            }
        }

        // Virtual method for cleanup (to be overridden by derived classes)
        virtual void clear_and_deallocate() = 0;

    public:
        // Public allocator access
        allocator_type get_allocator() const noexcept {
            return allocator_;
        }

        // Allocator comparison
        bool allocator_equals(const AllocatorAwareBase& other) const noexcept {
            return allocator_ == other.allocator_;
        }
    };

    // Allocator selection based on type characteristics
    template<typename T>
    struct OptimalAllocator {
        using type = std::conditional_t<
            sizeof(T) <= 64,  // Small objects benefit from pool allocation
            PoolAllocator<T>,
            DefaultAllocator<T>
        >;
    };

    template<typename T>
    using OptimalAllocator_t = typename OptimalAllocator<T>::type;

} // namespace Akhanda::Containers