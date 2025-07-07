// Vector.hpp
// Akhanda Game Engine - High-Performance Dynamic Array
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include "AllocatorAware.hpp"
#include "ContainerTraits.hpp"
#include <initializer_list>
#include <iterator>
#include <algorithm>
#include <utility>

namespace Akhanda::Containers {

    template<typename T, typename Allocator = DefaultAllocator<T>>
    class Vector : public AllocatorAwareBase<T, Allocator> {
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

        // Iterator types
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

            iterator& operator++() noexcept { ++ptr_; return *this; }
            iterator operator++(int) noexcept { iterator tmp = *this; ++ptr_; return tmp; }
            iterator& operator--() noexcept { --ptr_; return *this; }
            iterator operator--(int) noexcept { iterator tmp = *this; --ptr_; return tmp; }

            iterator& operator+=(difference_type n) noexcept { ptr_ += n; return *this; }
            iterator& operator-=(difference_type n) noexcept { ptr_ -= n; return *this; }
            iterator operator+(difference_type n) const noexcept { return iterator(ptr_ + n); }
            iterator operator-(difference_type n) const noexcept { return iterator(ptr_ - n); }
            difference_type operator-(const iterator& other) const noexcept { return ptr_ - other.ptr_; }

            bool operator==(const iterator& other) const noexcept { return ptr_ == other.ptr_; }
            bool operator!=(const iterator& other) const noexcept { return ptr_ != other.ptr_; }
            bool operator<(const iterator& other) const noexcept { return ptr_ < other.ptr_; }
            bool operator<=(const iterator& other) const noexcept { return ptr_ <= other.ptr_; }
            bool operator>(const iterator& other) const noexcept { return ptr_ > other.ptr_; }
            bool operator>=(const iterator& other) const noexcept { return ptr_ >= other.ptr_; }

        private:
            pointer ptr_;
        };

        class const_iterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = const T*;
            using reference = const T&;

            const_iterator() noexcept : ptr_(nullptr) {}
            explicit const_iterator(const_pointer ptr) noexcept : ptr_(ptr) {}
            const_iterator(const iterator& it) noexcept : ptr_(it.operator->()) {}

            reference operator*() const noexcept { return *ptr_; }
            pointer operator->() const noexcept { return ptr_; }
            reference operator[](difference_type n) const noexcept { return ptr_[n]; }

            const_iterator& operator++() noexcept { ++ptr_; return *this; }
            const_iterator operator++(int) noexcept { const_iterator tmp = *this; ++ptr_; return tmp; }
            const_iterator& operator--() noexcept { --ptr_; return *this; }
            const_iterator operator--(int) noexcept { const_iterator tmp = *this; --ptr_; return tmp; }

            const_iterator& operator+=(difference_type n) noexcept { ptr_ += n; return *this; }
            const_iterator& operator-=(difference_type n) noexcept { ptr_ -= n; return *this; }
            const_iterator operator+(difference_type n) const noexcept { return const_iterator(ptr_ + n); }
            const_iterator operator-(difference_type n) const noexcept { return const_iterator(ptr_ - n); }
            difference_type operator-(const const_iterator& other) const noexcept { return ptr_ - other.ptr_; }

            bool operator==(const const_iterator& other) const noexcept { return ptr_ == other.ptr_; }
            bool operator!=(const const_iterator& other) const noexcept { return ptr_ != other.ptr_; }
            bool operator<(const const_iterator& other) const noexcept { return ptr_ < other.ptr_; }
            bool operator<=(const const_iterator& other) const noexcept { return ptr_ <= other.ptr_; }
            bool operator>(const const_iterator& other) const noexcept { return ptr_ > other.ptr_; }
            bool operator>=(const const_iterator& other) const noexcept { return ptr_ >= other.ptr_; }

        private:
            const_pointer ptr_;
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
        Vector() noexcept(noexcept(Allocator()))
            : Base(), data_(nullptr), size_(0), capacity_(0) {
        }

        explicit Vector(const Allocator& alloc) noexcept
            : Base(alloc), data_(nullptr), size_(0), capacity_(0) {
        }

        explicit Vector(size_type count, const Allocator& alloc = Allocator())
            : Base(alloc), data_(nullptr), size_(0), capacity_(0) {
            resize(count);
        }

        Vector(size_type count, const T& value, const Allocator& alloc = Allocator())
            : Base(alloc), data_(nullptr), size_(0), capacity_(0) {
            assign(count, value);
        }

        template<typename InputIt>
        Vector(InputIt first, InputIt last, const Allocator& alloc = Allocator())
            : Base(alloc), data_(nullptr), size_(0), capacity_(0) {
            assign(first, last);
        }

        Vector(std::initializer_list<T> init, const Allocator& alloc = Allocator())
            : Base(alloc), data_(nullptr), size_(0), capacity_(0) {
            assign(init.begin(), init.end());
        }

        // Copy constructor
        Vector(const Vector& other)
            : Base(other), data_(nullptr), size_(0), capacity_(0) {
            if (other.size_ > 0) {
                reserve(other.size_);
                this->copy_construct_range(data_, other.data_, other.size_);
                size_ = other.size_;
            }
        }

        Vector(const Vector& other, const Allocator& alloc)
            : Base(alloc), data_(nullptr), size_(0), capacity_(0) {
            if (other.size_ > 0) {
                reserve(other.size_);
                this->copy_construct_range(data_, other.data_, other.size_);
                size_ = other.size_;
            }
        }

        // Move constructor
        Vector(Vector&& other) noexcept
            : Base(std::move(other)), data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }

        Vector(Vector&& other, const Allocator& alloc) noexcept
            : Base(alloc) {
            if (this->allocator_equals(other)) {
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;
                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }
            else {
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
        ~Vector() {
            clear_and_deallocate();
        }

        // Assignment operators
        Vector& operator=(const Vector& other) {
            if (this != &other) {
                Base::operator=(other);
                assign(other.begin(), other.end());
            }
            return *this;
        }

        Vector& operator=(Vector&& other) noexcept {
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
                }
                else {
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

        Vector& operator=(std::initializer_list<T> init) {
            assign(init.begin(), init.end());
            return *this;
        }

        // Element access
        reference at(size_type pos) {
            if (pos >= size_) {
                throw OutOfRangeException();
            }
            return data_[pos];
        }

        const_reference at(size_type pos) const {
            if (pos >= size_) {
                throw OutOfRangeException();
            }
            return data_[pos];
        }

        reference operator[](size_type pos) noexcept {
            return data_[pos];
        }

        const_reference operator[](size_type pos) const noexcept {
            return data_[pos];
        }

        reference front() noexcept {
            return data_[0];
        }

        const_reference front() const noexcept {
            return data_[0];
        }

        reference back() noexcept {
            return data_[size_ - 1];
        }

        const_reference back() const noexcept {
            return data_[size_ - 1];
        }

        T* data() noexcept {
            return data_;
        }

        const T* data() const noexcept {
            return data_;
        }

        // Iterators
        iterator begin() noexcept { return iterator(data_); }
        const_iterator begin() const noexcept { return const_iterator(data_); }
        const_iterator cbegin() const noexcept { return const_iterator(data_); }

        iterator end() noexcept { return iterator(data_ + size_); }
        const_iterator end() const noexcept { return const_iterator(data_ + size_); }
        const_iterator cend() const noexcept { return const_iterator(data_ + size_); }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

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

        void reserve(size_type new_cap) {
            if (new_cap <= capacity_) return;

            if (new_cap > max_size()) {
                throw CapacityException();
            }

            reallocate(new_cap);
        }

        size_type capacity() const noexcept {
            return capacity_;
        }

        void shrink_to_fit() {
            if (size_ < capacity_) {
                reallocate(size_);
            }
        }

        // Modifiers
        void clear() noexcept {
            this->destroy_range(data_, data_ + size_);
            size_ = 0;
        }

        iterator insert(const_iterator pos, const T& value) {
            return emplace(pos, value);
        }

        iterator insert(const_iterator pos, T&& value) {
            return emplace(pos, std::move(value));
        }

        iterator insert(const_iterator pos, size_type count, const T& value) {
            const size_type index = pos - cbegin();
            insert_space(index, count);

            for (size_type i = 0; i < count; ++i) {
                this->construct(data_ + index + i, value);
            }

            size_ += count;
            return iterator(data_ + index);
        }

        template<typename InputIt>
        iterator insert(const_iterator pos, InputIt first, InputIt last) {
            const size_type index = pos - cbegin();
            const size_type count = std::distance(first, last);

            insert_space(index, count);

            size_type i = 0;
            for (auto it = first; it != last; ++it, ++i) {
                this->construct(data_ + index + i, *it);
            }

            size_ += count;
            return iterator(data_ + index);
        }

        iterator insert(const_iterator pos, std::initializer_list<T> init) {
            return insert(pos, init.begin(), init.end());
        }

        template<typename... Args>
        iterator emplace(const_iterator pos, Args&&... args) {
            const size_type index = pos - cbegin();

            if (size_ == capacity_) {
                const size_type new_capacity = calculate_growth(1);
                reallocate_and_insert(new_capacity, index, std::forward<Args>(args)...);
            }
            else {
                insert_space(index, 1);
                this->construct(data_ + index, std::forward<Args>(args)...);
                ++size_;
            }

            return iterator(data_ + index);
        }

        iterator erase(const_iterator pos) {
            return erase(pos, pos + 1);
        }

        iterator erase(const_iterator first, const_iterator last) {
            const size_type start_index = first - cbegin();
            const size_type end_index = last - cbegin();
            const size_type count = end_index - start_index;

            if (count == 0) return iterator(data_ + start_index);

            // Destroy elements in range
            this->destroy_range(data_ + start_index, data_ + end_index);

            // Move remaining elements
            const size_type elements_to_move = size_ - end_index;
            if (elements_to_move > 0) {
                this->move_construct_range(data_ + start_index, data_ + end_index, elements_to_move);
                this->destroy_range(data_ + start_index + elements_to_move, data_ + size_);
            }

            size_ -= count;
            return iterator(data_ + start_index);
        }

        void push_back(const T& value) {
            emplace_back(value);
        }

        void push_back(T&& value) {
            emplace_back(std::move(value));
        }

        template<typename... Args>
        reference emplace_back(Args&&... args) {
            if (size_ == capacity_) {
                const size_type new_capacity = calculate_growth(1);
                reallocate_and_emplace_back(new_capacity, std::forward<Args>(args)...);
            }
            else {
                this->construct(data_ + size_, std::forward<Args>(args)...);
                ++size_;
            }
            return back();
        }

        void pop_back() noexcept {
            if (size_ > 0) {
                --size_;
                this->destroy(data_ + size_);
            }
        }

        void resize(size_type count) {
            if (count < size_) {
                this->destroy_range(data_ + count, data_ + size_);
                size_ = count;
            }
            else if (count > size_) {
                if (count > capacity_) {
                    reserve(count);
                }
                this->construct_range(data_ + size_, count - size_);
                size_ = count;
            }
        }

        void resize(size_type count, const T& value) {
            if (count < size_) {
                this->destroy_range(data_ + count, data_ + size_);
                size_ = count;
            }
            else if (count > size_) {
                if (count > capacity_) {
                    reserve(count);
                }
                this->construct_range(data_ + size_, count - size_, value);
                size_ = count;
            }
        }

        void assign(size_type count, const T& value) {
            clear();
            if (count > capacity_) {
                reserve(count);
            }
            this->construct_range(data_, count, value);
            size_ = count;
        }

        template<typename InputIt>
        void assign(InputIt first, InputIt last) {
            clear();
            const size_type count = std::distance(first, last);
            if (count > capacity_) {
                reserve(count);
            }
            this->construct_range(data_, first, last);
            size_ = count;
        }

        void assign(std::initializer_list<T> init) {
            assign(init.begin(), init.end());
        }

        void swap(Vector& other) noexcept {
            if (this != &other) {
                this->swap_allocator_aware(other);
                std::swap(data_, other.data_);
                std::swap(size_, other.size_);
                std::swap(capacity_, other.capacity_);
            }
        }

        // Comparison operators
        bool operator==(const Vector& other) const {
            return size_ == other.size_ && std::equal(begin(), end(), other.begin());
        }

        bool operator!=(const Vector& other) const {
            return !(*this == other);
        }

        bool operator<(const Vector& other) const {
            return std::lexicographical_compare(begin(), end(), other.begin(), other.end());
        }

        bool operator<=(const Vector& other) const {
            return !(other < *this);
        }

        bool operator>(const Vector& other) const {
            return other < *this;
        }

        bool operator>=(const Vector& other) const {
            return !(*this < other);
        }

    private:
        void clear_and_deallocate() override {
            if (data_) {
                this->destroy_range(data_, data_ + size_);
                this->deallocate(data_, capacity_);
                data_ = nullptr;
                size_ = 0;
                capacity_ = 0;
            }
        }

        size_type calculate_growth(size_type additional) const {
            const size_type required = size_ + additional;
            if (required > max_size()) {
                throw CapacityException();
            }

            if (capacity_ == 0) {
                return std::max(min_capacity_, required);
            }

            const size_type geometric_growth = static_cast<size_type>(capacity_ * growth_factor_);
            return std::max(geometric_growth, required);
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

        template<typename... Args>
        void reallocate_and_emplace_back(size_type new_capacity, Args&&... args) {
            pointer new_data = this->allocate(new_capacity);

            if (data_) {
                this->move_construct_range(new_data, data_, size_);
                this->destroy_range(data_, data_ + size_);
                this->deallocate(data_, capacity_);
            }

            this->construct(new_data + size_, std::forward<Args>(args)...);

            data_ = new_data;
            capacity_ = new_capacity;
            ++size_;
        }

        template<typename... Args>
        void reallocate_and_insert(size_type new_capacity, size_type index, Args&&... args) {
            pointer new_data = this->allocate(new_capacity);

            if (data_) {
                // Move elements before insertion point
                this->move_construct_range(new_data, data_, index);
                // Move elements after insertion point
                this->move_construct_range(new_data + index + 1, data_ + index, size_ - index);
                // Clean up old data
                this->destroy_range(data_, data_ + size_);
                this->deallocate(data_, capacity_);
            }

            // Construct new element at insertion point
            this->construct(new_data + index, std::forward<Args>(args)...);

            data_ = new_data;
            capacity_ = new_capacity;
            ++size_;
        }

        void insert_space(size_type index, size_type count) {
            if (size_ + count > capacity_) {
                const size_type new_capacity = calculate_growth(count);
                reallocate(new_capacity);
            }

            // Move elements to make space
            const size_type elements_to_move = size_ - index;
            if (elements_to_move > 0) {
                this->move_construct_range(data_ + index + count, data_ + index, elements_to_move);
                this->destroy_range(data_ + index, data_ + index + std::min(count, elements_to_move));
            }
        }
    };

    // Non-member functions
    template<typename T, typename Alloc>
    void swap(Vector<T, Alloc>& lhs, Vector<T, Alloc>& rhs) noexcept {
        lhs.swap(rhs);
    }

} // namespace Akhanda::Containers