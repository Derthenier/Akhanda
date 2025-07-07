// ContainerTraits.hpp
// Akhanda Game Engine - Container Type Traits and Concepts
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include <concepts>
#include <type_traits>
#include <iterator>
#include <memory>

namespace Akhanda::Containers::Traits {

    // Type trait to detect if a type is a container
    template<typename T>
    struct IsContainer : std::false_type {};

    template<typename T>
    constexpr bool IsContainer_v = IsContainer<T>::value;

    // Type trait to detect allocator type
    template<typename T>
    struct HasAllocator : std::false_type {};

    template<typename T>
        requires requires { typename T::allocator_type; }
    struct HasAllocator<T> : std::true_type {};

    template<typename T>
    constexpr bool HasAllocator_v = HasAllocator<T>::value;

    // Type trait for SIMD-friendly types
    template<typename T>
    struct IsSIMDFriendly : std::bool_constant<
        std::is_arithmetic_v<T> &&
        (sizeof(T) == 4 || sizeof(T) == 8) &&
        std::is_trivially_copyable_v<T>
    > {
    };

    template<typename T>
    constexpr bool IsSIMDFriendly_v = IsSIMDFriendly<T>::value;

    // Type trait for trivial types that can use memcpy
    template<typename T>
    struct IsTriviallyRelocatable : std::bool_constant<
        std::is_trivially_move_constructible_v<T>&&
        std::is_trivially_destructible_v<T>
    > {
    };

    template<typename T>
    constexpr bool IsTriviallyRelocatable_v = IsTriviallyRelocatable<T>::value;

    // Iterator type detection
    template<typename T>
    struct IteratorTraits {
        using iterator_category = typename std::iterator_traits<T>::iterator_category;
        using value_type = typename std::iterator_traits<T>::value_type;
        using difference_type = typename std::iterator_traits<T>::difference_type;
        using pointer = typename std::iterator_traits<T>::pointer;
        using reference = typename std::iterator_traits<T>::reference;
    };

    // Allocator rebind trait
    template<typename Allocator, typename T>
    struct AllocatorRebind {
        using type = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
    };

    template<typename Allocator, typename T>
    using AllocatorRebind_t = typename AllocatorRebind<Allocator, T>::type;

    // Container size type selection
    template<typename T>
    struct SizeType {
        using type = std::conditional_t<
            sizeof(T) >= 8,
            std::uint32_t,  // Use 32-bit for large objects
            std::size_t     // Use size_t for smaller objects
        >;
    };

    template<typename T>
    using SizeType_t = typename SizeType<T>::type;

    // Optimal alignment calculation
    template<typename T>
    struct OptimalAlignment {
        static constexpr std::size_t value = std::max({
            alignof(T),
            IsSIMDFriendly_v<T> ? 32u : alignof(T),  // AVX2 alignment for SIMD types
            sizeof(T) >= 64 ? 64u : alignof(T)       // Cache line alignment for large objects
            });
    };

    template<typename T>
    constexpr std::size_t OptimalAlignment_v = OptimalAlignment<T>::value;

    // Memory layout selection for containers
    template<typename T>
    struct PreferSoALayout : std::bool_constant<
        IsSIMDFriendly_v<T> &&
        sizeof(T) <= 16  // Small SIMD-friendly types benefit from SoA
    > {
    };

    template<typename T>
    constexpr bool PreferSoALayout_v = PreferSoALayout<T>::value;

    // Hash quality trait
    template<typename T>
    struct HasHighQualityHash : std::false_type {};

    // Specialize for fundamental types
    template<>
    struct HasHighQualityHash<int> : std::true_type {};
    template<>
    struct HasHighQualityHash<std::size_t> : std::true_type {};
    template<>
    struct HasHighQualityHash<float> : std::false_type {};  // Float hashing is poor
    template<>
    struct HasHighQualityHash<double> : std::false_type {}; // Double hashing is poor

    template<typename T>
    constexpr bool HasHighQualityHash_v = HasHighQualityHash<T>::value;

    // Growth strategy selection based on type characteristics
    template<typename T>
    struct GrowthStrategy {
        static constexpr float factor = []() {
            if constexpr (sizeof(T) <= 8) {
                return 2.0f;  // Aggressive growth for small types
            }
            else if constexpr (sizeof(T) <= 64) {
                return 1.5f;  // Moderate growth for medium types
            }
            else {
                return 1.25f; // Conservative growth for large types
            }
            }();

        static constexpr std::size_t min_capacity = []() {
            if constexpr (sizeof(T) <= 4) {
                return 16;    // Higher minimum for small types
            }
            else if constexpr (sizeof(T) <= 32) {
                return 8;     // Standard minimum
            }
            else {
                return 4;     // Lower minimum for large types
            }
            }();
    };

    // Concepts for container requirements
    template<typename T>
    concept ContainerElement = requires {
        requires std::is_destructible_v<T>;
        requires (std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>);
    };

    template<typename T>
    concept DefaultConstructibleElement = ContainerElement<T> && requires {
        requires std::is_default_constructible_v<T>;
    };

    template<typename T>
    concept ComparableElement = ContainerElement<T> && requires(const T & a, const T & b) {
        { a == b } -> std::convertible_to<bool>;
    };

    template<typename T>
    concept OrderedElement = ComparableElement<T> && requires(const T & a, const T & b) {
        { a < b } -> std::convertible_to<bool>;
        { a <= b } -> std::convertible_to<bool>;
        { a > b } -> std::convertible_to<bool>;
        { a >= b } -> std::convertible_to<bool>;
    };

    template<typename T>
    concept HashableElement = ContainerElement<T> && requires(const T & t) {
        std::hash<T>{}(t);
    };

    // Allocator concepts
    template<typename Alloc>
    concept StandardAllocator = requires(Alloc & a, typename Alloc::value_type * p, std::size_t n) {
        typename Alloc::value_type;
        { a.allocate(n) } -> std::same_as<typename Alloc::value_type*>;
        a.deallocate(p, n);
    };

    template<typename Alloc>
    concept StatefulAllocator = StandardAllocator<Alloc> && requires {
        requires !std::allocator_traits<Alloc>::is_always_equal::value;
    };

    // Container operation concepts
    template<typename Container, typename T>
    concept PushBackable = requires(Container & c, T && value) {
        c.push_back(std::forward<T>(value));
    };

    template<typename Container, typename T>
    concept EmplaceBackable = requires(Container & c, T && value) {
        c.emplace_back(std::forward<T>(value));
    };

    template<typename Container>
    concept RandomAccessible = requires(Container & c, std::size_t i) {
        c[i];
        c.at(i);
    };

    template<typename Container>
    concept Reservable = requires(Container & c, std::size_t n) {
        c.reserve(n);
        c.capacity();
    };

    // Memory operation selection traits
    template<typename T>
    struct OptimalMoveStrategy {
        static constexpr bool use_memcpy = IsTriviallyRelocatable_v<T>;
        static constexpr bool use_move_iterator = !use_memcpy && std::move_constructible_v<T>;
        static constexpr bool use_copy = !use_memcpy && !use_move_iterator;
    };

    template<typename T>
    struct OptimalCopyStrategy {
        static constexpr bool use_memcpy = std::is_trivially_copy_constructible_v<T>;
        static constexpr bool use_copy_constructor = !use_memcpy;
    };

    // Cache optimization traits
    template<typename T>
    struct CacheOptimization {
        static constexpr std::size_t cache_line_size = 64;
        static constexpr std::size_t elements_per_cache_line = cache_line_size / sizeof(T);
        static constexpr bool fits_in_cache_line = sizeof(T) <= cache_line_size;
        static constexpr bool prefer_prefetch = sizeof(T) >= 32;
    };

} // namespace Akhanda::Containers::Traits