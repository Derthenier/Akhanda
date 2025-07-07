// Queue.hpp
// Akhanda Game Engine - High-Performance Circular Buffer Queue
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include "AllocatorAware.hpp"
#include "ContainerTraits.hpp"
#include <utility>
#include <initializer_list>
#include <iterator>

namespace Akhanda::Containers {

    template<typename T, typename Allocator = DefaultAllocator<T>>
    class Queue : public AllocatorAwareBase<T, Allocator> {
    private:
        using Base = AllocatorAwareBase<T, Allocator>;
        using AllocTraits = typename Base::AllocTraits;
        using GrowthTraits = Traits::GrowthStrategy<T>;

    public:
        // Type aliases
        using value_type = T;
        using allocator_type = Allocator;
        using size_type = typename Base::size_type;
        using difference_type = typename Base::difference_type;
        using reference = typename Base::reference;
        using const_reference = typename Base::const_reference;
        using pointer = typename Base::pointer;
        using const_pointer = typename Base::const_pointer;

        // Iterator for queue traversal (front to back)
        class const_iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = const T*;
            using reference = const T&;

            const_iterator() noexcept : data_(nullptr), index_(0), capacity_(0) {}

            const_iterator(const_pointer data, size_type index, size_type front, size_type capacity) noexcept
                : data_(data), index_(index), front_index_(front), capacity_(capacity) {
            }

            reference operator*() const noexcept {
                return data_[(front_index_ + index_) % capacity_];
            }

            pointer operator->() const noexcept {
                return &data_[(front_index_ + index_) % capacity_];
            }

            const_iterator& operator++() noexcept {
                ++index_;
                return *this;
            }

            const_iterator operator++(int) noexcept {
                const_iterator tmp = *this;
                ++index_;
                return tmp;
            }

            bool operator==(const const_iterator& other) const noexcept {
                return index_ == other.index_;
            }

            bool operator!=(const const_iterator& other) const noexcept {
                return !(*this == other);
            }

        private:
            const_pointer data_;
            size_type index_;
            size_type front_index_;
            size_type capacity_;
        };

        using iterator = const_iterator; // Queue is typically read-only traversal

    private:
        pointer data_;
        size_type capacity_;
        size_type front_index_;  // Index of front element
        size_type back_index_;   // Index where next element will be inserted
        size_type size_;

        static constexpr size_type min_capacity_ = GrowthTraits::min_capacity;
        static constexpr float growth_factor_ = GrowthTraits::factor;

    public:
        // Constructors
        Queue() noexcept(noexcept(Allocator()))
            : Base(), data_(nullptr), capacity_(0), front_index_(0), back_index_(0), size_(0) {
        }

        explicit Queue(const Allocator& alloc) noexcept
            : Base(alloc), data_(nullptr), capacity_(0), front_index_(0), back_index_(0), size_(0) {
        }

        explicit Queue(size_type initial_capacity, const Allocator& alloc = Allocator())
            : Base(alloc), data_(nullptr), capacity_(0), front_index_(0), back_index_(0), size_(0) {
            reserve(initial_capacity);
        }

        Queue(std::initializer_list<T> init, const Allocator& alloc = Allocator())
            : Base(alloc), data_(nullptr), capacity_(0), front_index_(0), back_index_(0), size_(0) {
            reserve(init.size());
            for (const auto& item : init) {
                push(item);
            }
        }

        template<typename InputIt>
        Queue(InputIt first, InputIt last, const Allocator& alloc = Allocator())
            : Base(alloc), data_(nullptr), capacity_(0), front_index_(0), back_index_(0), size_(0) {
            const auto count = std::distance(first, last);
            if (count > 0) {
                reserve(static_cast<size_type>(count));
                for (auto it = first; it != last; ++it) {
                    push(*it);
                }
            }
        }

        // Copy constructor
        Queue(const Queue& other)
            : Base(other), data_(nullptr), capacity_(0), front_index_(0), back_index_(0), size_(0) {
            if (other.size_ > 0) {
                reserve(other.size_);
                copy_from_other(other);
            }
        }

        Queue(const Queue& other, const Allocator& alloc)
            : Base(alloc), data_(nullptr), capacity_(0), front_index_(0), back_index_(0), size_(0) {
            if (other.size_ > 0) {
                reserve(other.size_);
                copy_from_other(other);
            }
        }

        // Move constructor
        Queue(Queue&& other) noexcept
            : Base(std::move(other)), data_(other.data_), capacity_(other.capacity_)
            , front_index_(other.front_index_), back_index_(other.back_index_), size_(other.size_) {
            other.data_ = nullptr;
            other.capacity_ = 0;
            other.front_index_ = 0;
            other.back_index_ = 0;
            other.size_ = 0;
        }

        Queue(Queue&& other, const Allocator& alloc) noexcept
            : Base(alloc) {
            if (this->allocator_equals(other)) {
                data_ = other.data_;
                capacity_ = other.capacity_;
                front_index_ = other.front_index_;
                back_index_ = other.back_index_;
                size_ = other.size_;
                other.data_ = nullptr;
                other.capacity_ = 0;
                other.front_index_ = 0;
                other.back_index_ = 0;
                other.size_ = 0;
            }
            else {
                data_ = nullptr;
                capacity_ = 0;
                front_index_ = 0;
                back_index_ = 0;
                size_ = 0;
                if (other.size_ > 0) {
                    reserve(other.size_);
                    move_from_other(std::move(other));
                }
            }
        }

        // Destructor
        ~Queue() {
            clear_and_deallocate();
        }

        // Assignment operators
        Queue& operator=(const Queue& other) {
            if (this != &other) {
                Base::operator=(other);
                clear();
                if (other.size_ > 0) {
                    if (capacity_ < other.size_) {
                        reallocate(other.size_);
                    }
                    copy_from_other(other);
                }
            }
            return *this;
        }

        Queue& operator=(Queue&& other) noexcept {
            if (this != &other) {
                clear_and_deallocate();

                if (this->allocator_equals(other)) {
                    Base::operator=(std::move(other));
                    data_ = other.data_;
                    capacity_ = other.capacity_;
                    front_index_ = other.front_index_;
                    back_index_ = other.back_index_;
                    size_ = other.size_;
                    other.data_ = nullptr;
                    other.capacity_ = 0;
                    other.front_index_ = 0;
                    other.back_index_ = 0;
                    other.size_ = 0;
                }
                else {
                    Base::operator=(std::move(other));
                    if (other.size_ > 0) {
                        reserve(other.size_);
                        move_from_other(std::move(other));
                    }
                }
            }
            return *this;
        }

        Queue& operator=(std::initializer_list<T> init) {
            clear();
            if (capacity_ < init.size()) {
                reallocate(init.size());
            }
            for (const auto& item : init) {
                push(item);
            }
            return *this;
        }

        // Element access
        reference front() noexcept {
            return data_[front_index_];
        }

        const_reference front() const noexcept {
            return data_[front_index_];
        }

        reference back() noexcept {
            const size_type back_pos = (back_index_ + capacity_ - 1) % capacity_;
            return data_[back_pos];
        }

        const_reference back() const noexcept {
            const size_type back_pos = (back_index_ + capacity_ - 1) % capacity_;
            return data_[back_pos];
        }

        // Iterators (for range-based loops and algorithms)
        const_iterator begin() const noexcept {
            return const_iterator(data_, 0, front_index_, capacity_);
        }

        const_iterator end() const noexcept {
            return const_iterator(data_, size_, front_index_, capacity_);
        }

        const_iterator cbegin() const noexcept {
            return begin();
        }

        const_iterator cend() const noexcept {
            return end();
        }

        // Capacity
        bool empty() const noexcept {
            return size_ == 0;
        }

        size_type size() const noexcept {
            return size_;
        }

        size_type max_size() const noexcept {
            return AllocTraits::max_size(this->allocator_);
        }

        size_type capacity() const noexcept {
            return capacity_;
        }

        void reserve(size_type new_capacity) {
            if (new_capacity > capacity_) {
                reallocate(new_capacity);
            }
        }

        void shrink_to_fit() {
            if (size_ < capacity_) {
                if (size_ == 0) {
                    clear_and_deallocate();
                }
                else {
                    reallocate(size_);
                }
            }
        }

        // Modifiers
        void clear() noexcept {
            while (!empty()) {
                pop();
            }
            front_index_ = 0;
            back_index_ = 0;
        }

        void push(const T& value) {
            emplace(value);
        }

        void push(T&& value) {
            emplace(std::move(value));
        }

        template<typename... Args>
        void emplace(Args&&... args) {
            ensure_space_available();

            this->construct(data_ + back_index_, std::forward<Args>(args)...);
            back_index_ = (back_index_ + 1) % capacity_;
            ++size_;
        }

        void pop() noexcept {
            if (!empty()) {
                this->destroy(data_ + front_index_);
                front_index_ = (front_index_ + 1) % capacity_;
                --size_;
            }
        }

        void swap(Queue& other) noexcept {
            if (this != &other) {
                this->swap_allocator_aware(other);
                std::swap(data_, other.data_);
                std::swap(capacity_, other.capacity_);
                std::swap(front_index_, other.front_index_);
                std::swap(back_index_, other.back_index_);
                std::swap(size_, other.size_);
            }
        }

        // Queue-specific operations

        // Batch operations for better performance
        template<typename InputIt>
        void push_range(InputIt first, InputIt last) {
            const auto count = std::distance(first, last);
            if (count > 0) {
                ensure_space_for(static_cast<size_type>(count));
                for (auto it = first; it != last; ++it) {
                    emplace(*it);
                }
            }
        }

        // Pop multiple elements at once
        void pop_n(size_type count) noexcept {
            count = std::min(count, size_);
            for (size_type i = 0; i < count; ++i) {
                pop();
            }
        }

        // Peek at nth element from front (0-indexed)
        const_reference peek(size_type index) const noexcept {
            const size_type actual_index = (front_index_ + index) % capacity_;
            return data_[actual_index];
        }

        // Check if queue has at least n elements
        bool has_at_least(size_type n) const noexcept {
            return size_ >= n;
        }

        // Wait-free operations for single producer/consumer scenarios

        // Try to push (returns false if no space, useful for lock-free scenarios)
        bool try_push(const T& value) {
            if (size_ >= capacity_) {
                return false;
            }
            push(value);
            return true;
        }

        bool try_push(T&& value) {
            if (size_ >= capacity_) {
                return false;
            }
            push(std::move(value));
            return true;
        }

        // Try to pop (returns false if empty, useful for lock-free scenarios)
        bool try_pop(T& out_value) {
            if (empty()) {
                return false;
            }
            out_value = std::move(front());
            pop();
            return true;
        }

        // Comparison operators
        bool operator==(const Queue& other) const {
            if (size_ != other.size_) return false;

            auto it1 = begin();
            auto it2 = other.begin();

            for (; it1 != end(); ++it1, ++it2) {
                if (!(*it1 == *it2)) {
                    return false;
                }
            }
            return true;
        }

        bool operator!=(const Queue& other) const {
            return !(*this == other);
        }

    private:
        void clear_and_deallocate() override {
            if (data_) {
                clear();
                this->deallocate(data_, capacity_);
                data_ = nullptr;
                capacity_ = 0;
            }
        }

        void ensure_space_available() {
            if (size_ >= capacity_) {
                const size_type new_capacity = calculate_growth();
                reallocate(new_capacity);
            }
        }

        void ensure_space_for(size_type additional) {
            if (size_ + additional > capacity_) {
                const size_type new_capacity = std::max(
                    calculate_growth(),
                    size_ + additional
                );
                reallocate(new_capacity);
            }
        }

        size_type calculate_growth() const {
            if (capacity_ == 0) {
                return min_capacity_;
            }

            const size_type geometric_growth = static_cast<size_type>(capacity_ * growth_factor_);
            return std::max(geometric_growth, capacity_ + 1);
        }

        void reallocate(size_type new_capacity) {
            if (new_capacity == 0) {
                clear_and_deallocate();
                return;
            }

            // Ensure minimum capacity
            new_capacity = std::max(new_capacity, min_capacity_);

            pointer new_data = this->allocate(new_capacity);

            if (data_) {
                // Copy/move existing elements in order
                if constexpr (Traits::IsTriviallyRelocatable_v<T>) {
                    copy_elements_trivial(new_data);
                }
                else {
                    copy_elements_complex(new_data);
                }

                // Cleanup old data
                clear();
                this->deallocate(data_, capacity_);
            }

            data_ = new_data;
            capacity_ = new_capacity;
            front_index_ = 0;
            back_index_ = size_;
        }

        void copy_elements_trivial(pointer new_data) {
            if (size_ == 0) return;

            if (front_index_ + size_ <= capacity_) {
                // Contiguous range
                std::memcpy(new_data, data_ + front_index_, size_ * sizeof(T));
            }
            else {
                // Wrapped around
                const size_type first_part = capacity_ - front_index_;
                const size_type second_part = size_ - first_part;

                std::memcpy(new_data, data_ + front_index_, first_part * sizeof(T));
                std::memcpy(new_data + first_part, data_, second_part * sizeof(T));
            }
        }

        void copy_elements_complex(pointer new_data) {
            size_type dest_index = 0;
            for (size_type i = 0; i < size_; ++i) {
                const size_type src_index = (front_index_ + i) % capacity_;
                this->construct(new_data + dest_index, std::move(data_[src_index]));
                ++dest_index;
            }
        }

        void copy_from_other(const Queue& other) {
            front_index_ = 0;
            back_index_ = 0;
            size_ = 0;

            for (const auto& item : other) {
                emplace(item);
            }
        }

        void move_from_other(Queue&& other) {
            front_index_ = 0;
            back_index_ = 0;
            size_ = 0;

            // Move elements from other queue
            while (!other.empty()) {
                emplace(std::move(other.front()));
                other.pop();
            }
        }
    };

    // Non-member functions
    template<typename T, typename Alloc>
    void swap(Queue<T, Alloc>& lhs, Queue<T, Alloc>& rhs) noexcept {
        lhs.swap(rhs);
    }

    // Specialized queue types for common use cases
    template<typename T>
    using JobQueue = Queue<T, PoolAllocator<T, 256>>; // For job system

    template<typename T>
    using EventQueue = Queue<T, PoolAllocator<T, 128>>; // For event processing

    template<typename T>
    using MessageQueue = Queue<T, DefaultAllocator<T>>; // For message passing

} // namespace Akhanda::Containers