// Core.Containers.ixx
// Akhanda Game Engine - Container Library Module Interface
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

module;

// Standard library includes for module implementation
#include <concepts>
#include <type_traits>
#include <memory>
#include <initializer_list>
#include <iterator>
#include <functional>
#include <cstddef>
#include <cstdint>

export module Akhanda.Core.Containers;

// Import dependencies
import Akhanda.Core.Memory;
import Akhanda.Core.Math;

export namespace Akhanda::Containers {
    // Forward declarations
    template<typename T, typename Allocator>
    class Vector;

    template<typename T, std::size_t N>
    class StaticArray;

    template<typename Key, typename Value, typename Hash, typename Allocator>
    class HashMap;

    template<typename T, typename Allocator>
    class Queue;

    template<typename T, typename Allocator>
    class Stack;

    // Container traits and concepts
    template<typename T>
    concept Hashable = requires(const T & t) {
        std::hash<T>{}(t);
    };

    template<typename T>
    concept Comparable = requires(const T & a, const T & b) {
        { a == b } -> std::convertible_to<bool>;
        { a != b } -> std::convertible_to<bool>;
    };

    template<typename T>
    concept Movable = std::move_constructible<T> && std::is_move_constructible_v<T>;

    template<typename T>
    concept Copyable = std::copy_constructible<T> && std::is_copy_assignable_v<T>;

    template<typename Container>
    concept RandomAccessContainer = requires(Container & c, std::size_t i) {
        c[i];
        c.size();
        c.empty();
        c.data();
    };

    template<typename Container>
    concept SequentialContainer = requires(Container & c, typename Container::value_type value) {
        c.push_back(value);
        c.pop_back();
        c.front();
        c.back();
        c.begin();
        c.end();
    };

    // Iterator concepts
    template<typename Iter>
    concept ContiguousIterator = std::contiguous_iterator<Iter>;

    template<typename Iter>
    concept BidirectionalIterator = std::bidirectional_iterator<Iter>;

    // Allocator concepts
    template<typename Alloc>
    concept AllocatorCompatible = requires(Alloc & alloc, std::size_t n) {
        typename Alloc::value_type;
        alloc.allocate(n);
        alloc.deallocate(nullptr, n);
    };

    // Container configuration
    struct ContainerConfig {
        static constexpr std::size_t DefaultCapacity = 8;
        static constexpr std::size_t MaxCapacity = std::numeric_limits<std::size_t>::max() / 2;
        static constexpr float GrowthFactor = 1.5f;
        static constexpr std::size_t SIMDAlignment = 32; // AVX2 alignment
    };

    // Exception types for containers
    class ContainerException : public std::exception {
    public:
        explicit ContainerException(const char* message) : message_(message) {}
        const char* what() const noexcept override { return message_; }
    private:
        const char* message_;
    };

    class OutOfRangeException : public ContainerException {
    public:
        OutOfRangeException() : ContainerException("Container index out of range") {}
    };

    class CapacityException : public ContainerException {
    public:
        CapacityException() : ContainerException("Container capacity exceeded") {}
    };

    class EmptyContainerException : public ContainerException {
    public:
        EmptyContainerException() : ContainerException("Operation on empty container") {}
    };

    // Utility functions
    template<typename T>
    constexpr std::size_t CalculateGrowth(std::size_t currentCapacity) noexcept {
        if (currentCapacity == 0) return ContainerConfig::DefaultCapacity;

        const std::size_t newCapacity = static_cast<std::size_t>(
            currentCapacity * ContainerConfig::GrowthFactor
            );

        return (newCapacity > ContainerConfig::MaxCapacity)
            ? ContainerConfig::MaxCapacity
            : newCapacity;
    }

    template<typename T>
    constexpr std::size_t CalculateAlignedSize(std::size_t size) noexcept {
        const std::size_t alignment = std::max(alignof(T), ContainerConfig::SIMDAlignment);
        const std::size_t total_bytes = size * sizeof(T);
        // Align up using bit manipulation: (value + alignment - 1) & ~(alignment - 1)
        const std::size_t aligned_bytes = (total_bytes + alignment - 1) & ~(alignment - 1);
        return aligned_bytes / sizeof(T);
    }

    // Hash utilities for HashMap
    template<Hashable T>
    struct DefaultHash {
        std::size_t operator()(const T& key) const noexcept {
            return std::hash<T>{}(key);
        }
    };

    // Memory utilities
    template<typename T>
    void DestroyRange(T* first, T* last) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (T* ptr = first; ptr != last; ++ptr) {
                ptr->~T();
            }
        }
    }

    template<typename T>
    void UninitializedMove(T* source, T* dest, std::size_t count) noexcept {
        if constexpr (std::is_trivially_move_constructible_v<T>) {
            std::memcpy(dest, source, count * sizeof(T));
        }
        else {
            for (std::size_t i = 0; i < count; ++i) {
                new (dest + i) T(std::move(source[i]));
            }
        }
    }

    template<typename T>
    void UninitializedCopy(const T* source, T* dest, std::size_t count) {
        if constexpr (std::is_trivially_copy_constructible_v<T>) {
            std::memcpy(dest, source, count * sizeof(T));
        }
        else {
            for (std::size_t i = 0; i < count; ++i) {
                new (dest + i) T(source[i]);
            }
        }
    }

    template<typename T>
    void UninitializedFill(T* dest, std::size_t count, const T& value) {
        for (std::size_t i = 0; i < count; ++i) {
            new (dest + i) T(value);
        }
    }

    template<typename T>
    void UninitializedDefaultConstruct(T* dest, std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) {
            new (dest + i) T();
        }
    }
}