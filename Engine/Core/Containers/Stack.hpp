// Stack.hpp
// Akhanda Game Engine - High-Performance Stack Implementation
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include "AllocatorAware.hpp"
#include "ContainerTraits.hpp"
#include <utility>
#include <initializer_list>
#include <iterator>

namespace Akhanda::Containers {

    template<typename T, typename Allocator = DefaultAllocator<T>>
    class Stack : public AllocatorAwareBase<T, Allocator> {
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

        // Iterator for stack traversal (top to bottom)
        class const_iterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = const T*;
            using reference = const T&;

            const_iterator() noexcept : ptr_(nullptr) {}
            explicit const_iterator(const_pointer ptr) noexcept : ptr_(ptr) {}

            reference operator*() const noexcept { return *ptr_; }
            pointer operator->() const noexcept { return ptr_; }
            reference operator[](difference_type n) const noexcept { return ptr_[n]; }

            const_iterator& operator++() noexcept { --ptr_; return *this; } // Move toward bottom
            const_iterator operator++(int) noexcept { const_iterator tmp = *this; --ptr_; return tmp; }
            const_iterator& operator--() noexcept { ++ptr_; return *this; } // Move toward top
            const_iterator operator--(int) noexcept { const_iterator tmp = *this; ++ptr_; return tmp; }

            const_iterator& operator+=(difference_type n) noexcept { ptr_ -= n; return *this; }
            const_iterator& operator-=(difference_type n) noexcept { ptr_ += n; return *this; }
            const_iterator operator+(difference_type n) const noexcept { return const_iterator(ptr_ - n); }
            const_iterator operator-(difference_type n) const noexcept { return const_iterator(ptr_ + n); }
            difference_type operator-(const const_iterator& other) const noexcept { return other.ptr_ - ptr_; }

            bool operator==(const const_iterator& other) const noexcept { return ptr_ == other.ptr_; }
            bool operator!=(const const_iterator& other) const noexcept { return ptr_ != other.ptr_; }
            bool operator<(const const_iterator& other) const noexcept { return ptr_ > other.ptr_; }
            bool operator<=(const const_iterator& other) const noexcept { return ptr_ >= other.ptr_; }
            bool operator>(const const_iterator& other) const noexcept { return ptr_ < other.ptr_; }
            bool operator>=(const const_iterator& other) const noexcept { return ptr_ <= other.ptr_; }

        private:
            const_pointer ptr_;
        };

        class iterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T*;
            using reference = T&;

            iterator() noexcept : ptr_(nullptr) {}
            explicit iterator(pointer ptr) noexcept : ptr_(ptr) {}

            reference operator*() const noexcept { return *ptr_; }
            pointer operator->() const noexcept { return ptr_; }
            reference operator[](difference_type n) const noexcept { return ptr_[n]; }

            iterator& operator++() noexcept { --ptr_; return *this; } // Move toward bottom
            iterator operator++(int) noexcept { iterator tmp = *this; --ptr_; return tmp; }
            iterator& operator--() noexcept { ++ptr_; return *this; } // Move toward top
            iterator operator--(int) noexcept { iterator tmp = *this; ++ptr_; return tmp; }

            iterator& operator+=(difference_type n) noexcept { ptr_ -= n; return *this; }
            iterator& operator-=(difference_type n) noexcept { ptr_ += n; return *this; }
            iterator operator+(difference_type n) const noexcept { return iterator(ptr_ - n); }
            iterator operator-(difference_type n) const noexcept { return iterator(ptr_ + n); }
            difference_type operator-(const iterator& other) const noexcept { return other.ptr_ - ptr_; }

            bool operator==(const iterator& other) const noexcept { return ptr_ == other.ptr_; }
            bool operator!=(const iterator& other) const noexcept { return ptr_ != other.ptr_; }
            bool operator<(const iterator& other) const noexcept { return ptr_ > other.ptr_; }
            bool operator<=(const iterator& other) const noexcept { return ptr_ >= other.ptr_; }
            bool operator>(const iterator& other) const noexcept { return ptr_ < other.ptr_; }
            bool operator>=(const iterator& other) const noexcept { return ptr_ <= other.ptr_; }

            operator const_iterator() const noexcept { return const_iterator(ptr_); }

        private:
            pointer ptr_;
        };

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    private:
        pointer data_;
        size_type size_;
        size_type capacity_;

        static constexpr size_type min_capacity_ = GrowthTraits::min_capacity;
        static constexpr float growth_factor_ = GrowthTraits::factor;

    public:
        // Constructors
        Stack() noexcept(noexcept(Allocator()))
            : Base(), data_(nullptr), size_(0), capacity_(0) {}

        explicit Stack(const Allocator& alloc) noexcept
            : Base(alloc), data_(nullptr), size_(0), capacity_(0) {}

        explicit Stack(size_type initial_capacity, const Allocator& alloc = Allocator())
            : Base(alloc), data_(nullptr), size_(0), capacity_(0) {
            reserve(initial_capacity);
        }

        Stack(std::initializer_list<T> init, const Allocator& alloc = Allocator())
            : Base(alloc), data_(nullptr), size_(0), capacity_(0) {
            reserve(init.size());
            for (const auto& item : init) {
                push(item);
            }
        }

        template<typename InputIt>
        Stack(InputIt first, InputIt last, const Allocator& alloc = Allocator())
            : Base(alloc), data_(nullptr), size_(0), capacity_(0) {
            const auto count = std::distance(first, last);
            if (count > 0) {
                reserve(static_cast<size_type>(count));
                for (auto it = first; it != last; ++it) {
                    push(*it);
                }
            }
        }

        // Copy constructor
        Stack(const Stack& other)
            : Base(other), data_(nullptr), size_(0), capacity_(0) {
            if (other.size_ > 0) {
                reserve(other.size_);
                this->copy_construct_range(data_, other.data_, other.size_);
                size_ = other.size_;
            }
        }

        Stack(const Stack& other, const Allocator& alloc)
            : Base(alloc), data_(nullptr), size_(0), capacity_(0) {
            if (other.size_ > 0) {
                reserve(other.size_);
                this->copy_construct_range(data_, other.data_, other.size_);
                size_ = other.size_;
            }
        }

        // Move constructor
        Stack(Stack&& other) noexcept
            : Base(std::move(other)), data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }

        Stack(Stack&& other, const Allocator& alloc) noexcept
            : Base(alloc) {
            if (this->allocator_equals(other)) {
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;
                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            } else {
                data_ = nullptr;
                size_ = 0;
                capacity_ = 0;
                if (other.size_ > 0) {
                    reserve(other.size_);
                    this->move_construct_range(data_, other.data_, other.size_);
                    size_ = other.size_;
                }
            }
        }

        // Destructor
        ~Stack() {
            clear_and_deallocate();
        }

        // Assignment operators
        Stack& operator=(const Stack& other) {
            if (this != &other) {
                Base::operator=(other);
                clear();
                if (other.size_ > 0) {
                    if (capacity_ < other.size_) {
                        reallocate(other.size_);
                    }
                    this->copy_construct_range(data_, other.data_, other.size_);
                    size_ = other.size_;
                }
            }
            return *this;
        }

        Stack& operator=(Stack&& other) noexcept {
            if (this != &other) {
                clear_and_deallocate();
                
                if (this->allocator_equals(other)) {
                    Base::operator=(std::move(other));
                    data_ = other.data_;
                    size_ = other.size_;
                    capacity_ = other.capacity_;
                    other.data_ = nullptr;
                    other.size_ = 0;
                    other.capacity_ = 0;
                } else {
                    Base::operator=(std::move(other));
                    if (other.size_ > 0) {
                        reserve(other.size_);
                        this->move_construct_range(data_, other.data_, other.size_);
                        size_ = other.size_;
                    }
                }
            }
            return *this;
        }

        Stack& operator=(std::initializer_list<T> init) {
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
        reference top() noexcept {
            return data_[size_ - 1];
        }

        const_reference top() const noexcept {
            return data_[size_ - 1];
        }

        // Access nth element from top (0 = top, 1 = second from top, etc.)
        reference peek(size_type depth) noexcept {
            return data_[size_ - 1 - depth];
        }

        const_reference peek(size_type depth) const noexcept {
            return data_[size_ - 1 - depth];
        }

        // Iterators (top to bottom traversal)
        iterator begin() noexcept {
            return empty() ? end() : iterator(data_ + size_ - 1);
        }

        const_iterator begin() const noexcept {
            return empty() ? end() : const_iterator(data_ + size_ - 1);
        }

        const_iterator cbegin() const noexcept {
            return begin();
        }

        iterator end() noexcept {
            return iterator(data_ - 1);
        }

        const_iterator end() const noexcept {
            return const_iterator(data_ - 1);
        }

        const_iterator cend() const noexcept {
            return end();
        }

        // Reverse iterators (bottom to top traversal)
        reverse_iterator rbegin() noexcept {
            return reverse_iterator(end());
        }

        const_reverse_iterator rbegin() const noexcept {
            return const_reverse_iterator(end());
        }

        const_reverse_iterator crbegin() const noexcept {
            return const_reverse_iterator(end());
        }

        reverse_iterator rend() noexcept {
            return reverse_iterator(begin());
        }

        const_reverse_iterator rend() const noexcept {
            return const_reverse_iterator(begin());
        }

        const_reverse_iterator crend() const noexcept {
            return const_reverse_iterator(begin());
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
                } else {
                    reallocate(size_);
                }
            }
        }

        // Modifiers
        void clear() noexcept {
            this->destroy_range(data_, data_ + size_);
            size_ = 0;
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
            this->construct(data_ + size_, std::forward<Args>(args)...);
            ++size_;
        }

        void pop() noexcept {
            if (!empty()) {
                --size_;
                this->destroy(data_ + size_);
            }
        }

        // Try to pop (returns false if empty, useful for algorithms)
        bool try_pop(T& out_value) {
            if (empty()) {
                return false;
            }
            out_value = std::move(top());
            pop();
            return true;
        }

        void swap(Stack& other) noexcept {
            if (this != &other) {
                this->swap_allocator_aware(other);
                std::swap(data_, other.data_);
                std::swap(size_, other.size_);
                std::swap(capacity_, other.capacity_);
            }
        }

        // Stack-specific operations
        
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

        // Pop elements into container
        template<typename OutputContainer>
        void pop_into(OutputContainer& container, size_type count) {
            count = std::min(count, size_);
            container.reserve(container.size() + count);
            
            for (size_type i = 0; i < count; ++i) {
                container.push_back(std::move(top()));
                pop();
            }
        }

        // Check if stack has at least n elements
        bool has_at_least(size_type n) const noexcept {
            return size_ >= n;
        }

        // Check stack depth
        size_type depth() const noexcept {
            return size_;
        }

        // Specialized stack operations for algorithms
        
        // Duplicate top element
        void dup() {
            if (!empty()) {
                push(top());
            }
        }

        // Swap top two elements
        void swap_top() noexcept {
            if (size_ >= 2) {
                std::swap(data_[size_ - 1], data_[size_ - 2]);
            }
        }

        // Rotate top n elements (bring nth element to top)
        void rotate_top(size_type n) noexcept {
            if (n > 0 && n < size_) {
                std::rotate(data_ + size_ - n - 1, data_ + size_ - n, data_ + size_);
            }
        }

        // Drop nth element from top
        void drop(size_type depth) {
            if (depth < size_) {
                const size_type index = size_ - 1 - depth;
                this->destroy(data_ + index);
                // Move elements down to fill gap
                for (size_type i = index; i < size_ - 1; ++i) {
                    this->move_construct_range(data_ + i, data_ + i + 1, 1);
                    this->destroy(data_ + i + 1);
                }
                --size_;
            }
        }

        // Stack frame operations for call stacks
        struct StackFrame {
            size_type start_size;
            
            explicit StackFrame(const Stack& stack) : start_size(stack.size()) {}
        };

        StackFrame push_frame() const {
            return StackFrame(*this);
        }

        void pop_frame(const StackFrame& frame) noexcept {
            if (frame.start_size <= size_) {
                pop_n(size_ - frame.start_size);
            }
        }

        // Comparison operators
        bool operator==(const Stack& other) const {
            return size_ == other.size_ && std::equal(data_, data_ + size_, other.data_);
        }

        bool operator!=(const Stack& other) const {
            return !(*this == other);
        }

        bool operator<(const Stack& other) const {
            return std::lexicographical_compare(data_, data_ + size_, other.data_, other.data_ + other.size_);
        }

        bool operator<=(const Stack& other) const {
            return !(other < *this);
        }

        bool operator>(const Stack& other) const {
            return other < *this;
        }

        bool operator>=(const Stack& other) const {
            return !(*this < other);
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

            pointer new_data = this->allocate(new_capacity);
            
            if (data_) {
                this->move_construct_range(new_data, data_, size_);
                this->destroy_range(data_, data_ + size_);
                this->deallocate(data_, capacity_);
            }
            
            data_ = new_data;
            capacity_ = new_capacity;
        }
    };

    // Non-member functions
    template<typename T, typename Alloc>
    void swap(Stack<T, Alloc>& lhs, Stack<T, Alloc>& rhs) noexcept {
        lhs.swap(rhs);
    }

    // Specialized stack types for common use cases
    template<typename T>
    using CallStack = Stack<T, PoolAllocator<T, 64>>; // For algorithm call stacks

    template<typename T>
    using UndoStack = Stack<T, DefaultAllocator<T>>; // For undo/redo systems

    template<typename T>
    using WorkStack = Stack<T, PoolAllocator<T, 128>>; // For temporary work items

} // namespace Akhanda::Containers