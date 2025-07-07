// HashMap.hpp
// Akhanda Game Engine - High-Performance Robin Hood Hash Table
// Copyright (c) 2025 Aditya Vennelakanti. All rights reserved.

#pragma once

#include "AllocatorAware.hpp"
#include "ContainerTraits.hpp"
#include <functional>
#include <utility>
#include <initializer_list>
#include <iterator>

namespace Akhanda::Containers {

    template<
        typename Key,
        typename Value,
        typename Hash = DefaultHash<Key>,
        typename KeyEqual = std::equal_to<Key>,
        typename Allocator = DefaultAllocator<std::pair<const Key, Value>>
    >
    class HashMap : public AllocatorAwareBase<std::pair<const Key, Value>, Allocator> {
    private:
        using Base = AllocatorAwareBase<std::pair<const Key, Value>, Allocator>;
        using NodeAllocator = typename Base::AllocTraits::template rebind_alloc<struct Node>;
        using NodeAllocTraits = std::allocator_traits<NodeAllocator>;

    public:
        // Type aliases
        using key_type = Key;
        using mapped_type = Value;
        using value_type = std::pair<const Key, Value>;
        using size_type = typename Base::size_type;
        using difference_type = typename Base::difference_type;
        using hasher = Hash;
        using key_equal = KeyEqual;
        using allocator_type = Allocator;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = typename Base::pointer;
        using const_pointer = typename Base::const_pointer;

    private:
        // Robin Hood hashing node structure
        struct Node {
            value_type kv_pair;
            uint32_t probe_distance;  // Distance from ideal position
            bool occupied;

            Node() : probe_distance(0), occupied(false) {}

            template<typename K, typename V>
            Node(K&& key, V&& value, uint32_t distance)
                : kv_pair(std::forward<K>(key), std::forward<V>(value))
                , probe_distance(distance)
                , occupied(true) {
            }

            void clear() {
                if (occupied) {
                    kv_pair.~value_type();
                    occupied = false;
                    probe_distance = 0;
                }
            }
        };

        NodeAllocator node_allocator_;
        Node* buckets_;
        size_type bucket_count_;
        size_type size_;
        size_type max_load_factor_percent_;  // As percentage (75 = 75%)
        hasher hash_function_;
        key_equal key_eq_;

        static constexpr size_type min_bucket_count_ = 8;
        static constexpr size_type default_max_load_factor_ = 75; // 75%
        static constexpr uint32_t max_probe_distance_ = 255; // Prevent infinite loops

    public:
        // Iterator implementation
        class iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = HashMap::value_type;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;

            iterator() noexcept : buckets_(nullptr), bucket_index_(0), bucket_count_(0) {}

            iterator(Node* buckets, size_type index, size_type count) noexcept
                : buckets_(buckets), bucket_index_(index), bucket_count_(count) {
                advance_to_occupied();
            }

            reference operator*() const noexcept {
                return buckets_[bucket_index_].kv_pair;
            }

            pointer operator->() const noexcept {
                return &buckets_[bucket_index_].kv_pair;
            }

            iterator& operator++() noexcept {
                ++bucket_index_;
                advance_to_occupied();
                return *this;
            }

            iterator operator++(int) noexcept {
                iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const iterator& other) const noexcept {
                return bucket_index_ == other.bucket_index_;
            }

            bool operator!=(const iterator& other) const noexcept {
                return !(*this == other);
            }

        private:
            Node* buckets_;
            size_type bucket_index_;
            size_type bucket_count_;

            void advance_to_occupied() noexcept {
                while (bucket_index_ < bucket_count_ && !buckets_[bucket_index_].occupied) {
                    ++bucket_index_;
                }
            }
        };

        class const_iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = HashMap::value_type;
            using difference_type = std::ptrdiff_t;
            using pointer = const value_type*;
            using reference = const value_type&;

            const_iterator() noexcept : buckets_(nullptr), bucket_index_(0), bucket_count_(0) {}

            const_iterator(const Node* buckets, size_type index, size_type count) noexcept
                : buckets_(buckets), bucket_index_(index), bucket_count_(count) {
                advance_to_occupied();
            }

            const_iterator(const iterator& it) noexcept
                : buckets_(it.buckets_), bucket_index_(it.bucket_index_), bucket_count_(it.bucket_count_) {
            }

            reference operator*() const noexcept {
                return buckets_[bucket_index_].kv_pair;
            }

            pointer operator->() const noexcept {
                return &buckets_[bucket_index_].kv_pair;
            }

            const_iterator& operator++() noexcept {
                ++bucket_index_;
                advance_to_occupied();
                return *this;
            }

            const_iterator operator++(int) noexcept {
                const_iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const const_iterator& other) const noexcept {
                return bucket_index_ == other.bucket_index_;
            }

            bool operator!=(const const_iterator& other) const noexcept {
                return !(*this == other);
            }

        private:
            const Node* buckets_;
            size_type bucket_index_;
            size_type bucket_count_;

            void advance_to_occupied() noexcept {
                while (bucket_index_ < bucket_count_ && !buckets_[bucket_index_].occupied) {
                    ++bucket_index_;
                }
            }
        };

    public:
        // Constructors
        HashMap() : HashMap(min_bucket_count_) {}

        explicit HashMap(size_type bucket_count, const Hash& hash = Hash(),
            const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator())
            : Base(alloc), node_allocator_(alloc), buckets_(nullptr), bucket_count_(0), size_(0)
            , max_load_factor_percent_(default_max_load_factor_), hash_function_(hash), key_eq_(equal) {
            rehash(bucket_count);
        }

        explicit HashMap(const Allocator& alloc)
            : HashMap(min_bucket_count_, Hash{}, KeyEqual{}, alloc) {
        }

        template<typename InputIt>
        HashMap(InputIt first, InputIt last, size_type bucket_count = min_bucket_count_,
            const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual(),
            const Allocator& alloc = Allocator())
            : HashMap(bucket_count, hash, equal, alloc) {
            insert(first, last);
        }

        HashMap(std::initializer_list<value_type> init, size_type bucket_count = min_bucket_count_,
            const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual(),
            const Allocator& alloc = Allocator())
            : HashMap(bucket_count, hash, equal, alloc) {
            insert(init.begin(), init.end());
        }

        // Copy constructor
        HashMap(const HashMap& other)
            : Base(other), node_allocator_(other.node_allocator_), buckets_(nullptr)
            , bucket_count_(0), size_(0), max_load_factor_percent_(other.max_load_factor_percent_)
            , hash_function_(other.hash_function_), key_eq_(other.key_eq_) {
            rehash(other.bucket_count_);
            for (const auto& pair : other) {
                insert(pair);
            }
        }

        HashMap(const HashMap& other, const Allocator& alloc)
            : Base(alloc), node_allocator_(alloc), buckets_(nullptr)
            , bucket_count_(0), size_(0), max_load_factor_percent_(other.max_load_factor_percent_)
            , hash_function_(other.hash_function_), key_eq_(other.key_eq_) {
            rehash(other.bucket_count_);
            for (const auto& pair : other) {
                insert(pair);
            }
        }

        // Move constructor
        HashMap(HashMap&& other) noexcept
            : Base(std::move(other)), node_allocator_(std::move(other.node_allocator_))
            , buckets_(other.buckets_), bucket_count_(other.bucket_count_), size_(other.size_)
            , max_load_factor_percent_(other.max_load_factor_percent_)
            , hash_function_(std::move(other.hash_function_)), key_eq_(std::move(other.key_eq_)) {
            other.buckets_ = nullptr;
            other.bucket_count_ = 0;
            other.size_ = 0;
        }

        HashMap(HashMap&& other, const Allocator& alloc)
            : Base(alloc), node_allocator_(alloc) {
            if (this->allocator_equals(other)) {
                buckets_ = other.buckets_;
                bucket_count_ = other.bucket_count_;
                size_ = other.size_;
                max_load_factor_percent_ = other.max_load_factor_percent_;
                hash_function_ = std::move(other.hash_function_);
                key_eq_ = std::move(other.key_eq_);
                other.buckets_ = nullptr;
                other.bucket_count_ = 0;
                other.size_ = 0;
            }
            else {
                buckets_ = nullptr;
                bucket_count_ = 0;
                size_ = 0;
                max_load_factor_percent_ = other.max_load_factor_percent_;
                hash_function_ = other.hash_function_;
                key_eq_ = other.key_eq_;
                rehash(other.bucket_count_);
                for (auto&& pair : other) {
                    insert(std::move(pair));
                }
            }
        }

        // Destructor
        ~HashMap() {
            clear_and_deallocate();
        }

        // Assignment operators
        HashMap& operator=(const HashMap& other) {
            if (this != &other) {
                Base::operator=(other);
                clear();
                hash_function_ = other.hash_function_;
                key_eq_ = other.key_eq_;
                max_load_factor_percent_ = other.max_load_factor_percent_;
                rehash(other.bucket_count_);
                for (const auto& pair : other) {
                    insert(pair);
                }
            }
            return *this;
        }

        HashMap& operator=(HashMap&& other) noexcept {
            if (this != &other) {
                clear_and_deallocate();

                if (this->allocator_equals(other)) {
                    Base::operator=(std::move(other));
                    buckets_ = other.buckets_;
                    bucket_count_ = other.bucket_count_;
                    size_ = other.size_;
                    max_load_factor_percent_ = other.max_load_factor_percent_;
                    hash_function_ = std::move(other.hash_function_);
                    key_eq_ = std::move(other.key_eq_);
                    other.buckets_ = nullptr;
                    other.bucket_count_ = 0;
                    other.size_ = 0;
                }
                else {
                    Base::operator=(std::move(other));
                    hash_function_ = other.hash_function_;
                    key_eq_ = other.key_eq_;
                    max_load_factor_percent_ = other.max_load_factor_percent_;
                    rehash(other.bucket_count_);
                    for (auto&& pair : other) {
                        insert(std::move(pair));
                    }
                }
            }
            return *this;
        }

        HashMap& operator=(std::initializer_list<value_type> init) {
            clear();
            insert(init.begin(), init.end());
            return *this;
        }

        // Element access
        Value& at(const Key& key) {
            auto it = find(key);
            if (it == end()) {
                throw std::out_of_range("HashMap::at: Key not found");
            }
            return it->second;
        }

        const Value& at(const Key& key) const {
            auto it = find(key);
            if (it == end()) {
                throw std::out_of_range("HashMap::at: Key not found");
            }
            return it->second;
        }

        Value& operator[](const Key& key) {
            auto result = try_emplace(key);
            return result.first->second;
        }

        Value& operator[](Key&& key) {
            auto result = try_emplace(std::move(key));
            return result.first->second;
        }

        // Iterators
        iterator begin() noexcept {
            return iterator(buckets_, 0, bucket_count_);
        }

        const_iterator begin() const noexcept {
            return const_iterator(buckets_, 0, bucket_count_);
        }

        const_iterator cbegin() const noexcept {
            return const_iterator(buckets_, 0, bucket_count_);
        }

        iterator end() noexcept {
            return iterator(buckets_, bucket_count_, bucket_count_);
        }

        const_iterator end() const noexcept {
            return const_iterator(buckets_, bucket_count_, bucket_count_);
        }

        const_iterator cend() const noexcept {
            return const_iterator(buckets_, bucket_count_, bucket_count_);
        }

        // Capacity
        bool empty() const noexcept {
            return size_ == 0;
        }

        size_type size() const noexcept {
            return size_;
        }

        size_type max_size() const noexcept {
            return NodeAllocTraits::max_size(node_allocator_);
        }

        // Modifiers
        void clear() noexcept {
            if (buckets_) {
                for (size_type i = 0; i < bucket_count_; ++i) {
                    buckets_[i].clear();
                }
                size_ = 0;
            }
        }

        std::pair<iterator, bool> insert(const value_type& value) {
            return insert_impl(value.first, value.second);
        }

        std::pair<iterator, bool> insert(value_type&& value) {
            return insert_impl(std::move(value.first), std::move(value.second));
        }

        template<typename InputIt>
        void insert(InputIt first, InputIt last) {
            for (auto it = first; it != last; ++it) {
                insert(*it);
            }
        }

        void insert(std::initializer_list<value_type> init) {
            insert(init.begin(), init.end());
        }

        template<typename... Args>
        std::pair<iterator, bool> emplace(Args&&... args) {
            return emplace_impl(std::forward<Args>(args)...);
        }

        template<typename... Args>
        std::pair<iterator, bool> try_emplace(const Key& key, Args&&... args) {
            auto pos = find_position(key);
            if (buckets_[pos].occupied && key_eq_(buckets_[pos].kv_pair.first, key)) {
                return { iterator(buckets_, pos, bucket_count_), false };
            }
            return emplace_at_position(pos, key, std::forward<Args>(args)...);
        }

        template<typename... Args>
        std::pair<iterator, bool> try_emplace(Key&& key, Args&&... args) {
            auto pos = find_position(key);
            if (buckets_[pos].occupied && key_eq_(buckets_[pos].kv_pair.first, key)) {
                return { iterator(buckets_, pos, bucket_count_), false };
            }
            return emplace_at_position(pos, std::move(key), std::forward<Args>(args)...);
        }

        iterator erase(const_iterator pos) {
            size_type bucket_index = pos.bucket_index_;
            if (bucket_index < bucket_count_ && buckets_[bucket_index].occupied) {
                erase_at_position(bucket_index);
            }
            return iterator(buckets_, bucket_index, bucket_count_);
        }

        iterator erase(const_iterator first, const_iterator last) {
            auto current = first;
            while (current != last) {
                current = erase(current);
            }
            return iterator(buckets_, current.bucket_index_, bucket_count_);
        }

        size_type erase(const Key& key) {
            auto it = find(key);
            if (it != end()) {
                erase(it);
                return 1;
            }
            return 0;
        }

        void swap(HashMap& other) noexcept {
            if (this != &other) {
                this->swap_allocator_aware(other);
                std::swap(buckets_, other.buckets_);
                std::swap(bucket_count_, other.bucket_count_);
                std::swap(size_, other.size_);
                std::swap(max_load_factor_percent_, other.max_load_factor_percent_);
                std::swap(hash_function_, other.hash_function_);
                std::swap(key_eq_, other.key_eq_);
            }
        }

        // Lookup
        size_type count(const Key& key) const {
            return find(key) != end() ? 1 : 0;
        }

        iterator find(const Key& key) {
            if (bucket_count_ == 0) return end();

            size_type pos = find_position(key);
            if (buckets_[pos].occupied && key_eq_(buckets_[pos].kv_pair.first, key)) {
                return iterator(buckets_, pos, bucket_count_);
            }
            return end();
        }

        const_iterator find(const Key& key) const {
            if (bucket_count_ == 0) return end();

            size_type pos = find_position(key);
            if (buckets_[pos].occupied && key_eq_(buckets_[pos].kv_pair.first, key)) {
                return const_iterator(buckets_, pos, bucket_count_);
            }
            return end();
        }

        bool contains(const Key& key) const {
            return find(key) != end();
        }

        std::pair<iterator, iterator> equal_range(const Key& key) {
            auto it = find(key);
            return { it, it == end() ? it : std::next(it) };
        }

        std::pair<const_iterator, const_iterator> equal_range(const Key& key) const {
            auto it = find(key);
            return { it, it == end() ? it : std::next(it) };
        }

        // Bucket interface
        size_type bucket_count() const noexcept {
            return bucket_count_;
        }

        size_type max_bucket_count() const noexcept {
            return max_size();
        }

        // Hash policy
        float load_factor() const noexcept {
            return bucket_count_ == 0 ? 0.0f : static_cast<float>(size_) / bucket_count_;
        }

        float max_load_factor() const noexcept {
            return max_load_factor_percent_ / 100.0f;
        }

        void max_load_factor(float ml) {
            max_load_factor_percent_ = static_cast<size_type>(ml * 100);
            if (load_factor() > max_load_factor()) {
                rehash(bucket_count_ + 1);
            }
        }

        void rehash(size_type count) {
            const size_type new_bucket_count = std::max(count,
                static_cast<size_type>(std::ceil(size_ / max_load_factor())));

            if (new_bucket_count != bucket_count_) {
                rehash_impl(new_bucket_count);
            }
        }

        void reserve(size_type count) {
            rehash(static_cast<size_type>(std::ceil(count / max_load_factor())));
        }

        // Observers
        hasher hash_function() const {
            return hash_function_;
        }

        key_equal key_eq() const {
            return key_eq_;
        }

        // Comparison operators
        bool operator==(const HashMap& other) const {
            if (size_ != other.size_) return false;

            for (const auto& pair : *this) {
                auto it = other.find(pair.first);
                if (it == other.end() || !(it->second == pair.second)) {
                    return false;
                }
            }
            return true;
        }

        bool operator!=(const HashMap& other) const {
            return !(*this == other);
        }

    private:
        void clear_and_deallocate() override {
            if (buckets_) {
                clear();
                NodeAllocTraits::deallocate(node_allocator_, buckets_, bucket_count_);
                buckets_ = nullptr;
                bucket_count_ = 0;
            }
        }

        size_type find_position(const Key& key) const {
            if (bucket_count_ == 0) return 0;

            const auto hash_value = hash_function_(key);
            size_type pos = hash_value % bucket_count_;
            uint32_t probe_distance = 0;

            while (buckets_[pos].occupied && probe_distance <= max_probe_distance_) {
                if (key_eq_(buckets_[pos].kv_pair.first, key)) {
                    return pos;
                }

                pos = (pos + 1) % bucket_count_;
                ++probe_distance;
            }

            return pos;
        }

        template<typename K, typename V>
        std::pair<iterator, bool> insert_impl(K&& key, V&& value) {
            ensure_space_available();

            auto pos = find_position(key);
            if (buckets_[pos].occupied && key_eq_(buckets_[pos].kv_pair.first, key)) {
                return { iterator(buckets_, pos, bucket_count_), false };
            }

            return insert_at_position(pos, std::forward<K>(key), std::forward<V>(value));
        }

        template<typename... Args>
        std::pair<iterator, bool> emplace_impl(Args&&... args) {
            // Create temporary pair to extract key
            value_type temp(std::forward<Args>(args)...);
            return insert_impl(std::move(temp.first), std::move(temp.second));
        }

        template<typename K, typename... Args>
        std::pair<iterator, bool> emplace_at_position(size_type pos, K&& key, Args&&... args) {
            ensure_space_available();
            return insert_at_position(pos, std::forward<K>(key), std::forward<Args>(args)...);
        }

        template<typename K, typename V>
        std::pair<iterator, bool> insert_at_position(size_type pos, K&& key, V&& value) {
            const auto hash_value = hash_function_(key);
            size_type ideal_pos = hash_value % bucket_count_;
            uint32_t probe_distance = calculate_probe_distance(ideal_pos, pos);

            // Robin Hood: steal from the rich, give to the poor
            Node new_node(std::forward<K>(key), std::forward<V>(value), probe_distance);

            while (buckets_[pos].occupied) {
                if (buckets_[pos].probe_distance < new_node.probe_distance) {
                    std::swap(buckets_[pos], new_node);
                }
                pos = (pos + 1) % bucket_count_;
                ++new_node.probe_distance;
            }

            buckets_[pos] = std::move(new_node);
            ++size_;

            return { iterator(buckets_, pos, bucket_count_), true };
        }

        void erase_at_position(size_type pos) {
            buckets_[pos].clear();
            --size_;

            // Backward shifting to maintain Robin Hood property
            size_type next_pos = (pos + 1) % bucket_count_;
            while (buckets_[next_pos].occupied && buckets_[next_pos].probe_distance > 0) {
                buckets_[pos] = std::move(buckets_[next_pos]);
                buckets_[pos].probe_distance--;
                buckets_[next_pos].clear();

                pos = next_pos;
                next_pos = (next_pos + 1) % bucket_count_;
            }
        }

        void ensure_space_available() {
            if (should_rehash()) {
                rehash(bucket_count_ * 2);
            }
        }

        bool should_rehash() const {
            return (size_ + 1) * 100 > bucket_count_ * max_load_factor_percent_;
        }

        void rehash_impl(size_type new_bucket_count) {
            // Ensure minimum bucket count and power of 2 for better performance
            new_bucket_count = std::max(new_bucket_count, min_bucket_count_);
            new_bucket_count = next_power_of_two(new_bucket_count);

            Node* old_buckets = buckets_;
            size_type old_bucket_count = bucket_count_;

            // Allocate new buckets
            buckets_ = NodeAllocTraits::allocate(node_allocator_, new_bucket_count);
            bucket_count_ = new_bucket_count;

            // Initialize new buckets
            for (size_type i = 0; i < bucket_count_; ++i) {
                new (buckets_ + i) Node();
            }

            // Move all elements to new buckets
            size_type old_size = size_;
            size_ = 0;

            if (old_buckets) {
                for (size_type i = 0; i < old_bucket_count; ++i) {
                    if (old_buckets[i].occupied) {
                        insert_impl(std::move(old_buckets[i].kv_pair.first),
                            std::move(old_buckets[i].kv_pair.second));
                        old_buckets[i].clear();
                    }
                }

                NodeAllocTraits::deallocate(node_allocator_, old_buckets, old_bucket_count);
            }
        }

        uint32_t calculate_probe_distance(size_type ideal_pos, size_type actual_pos) const {
            if (actual_pos >= ideal_pos) {
                return static_cast<uint32_t>(actual_pos - ideal_pos);
            }
            else {
                return static_cast<uint32_t>(bucket_count_ - ideal_pos + actual_pos);
            }
        }

        static size_type next_power_of_two(size_type n) {
            if (n <= 1) return 1;

            --n;
            n |= n >> 1;
            n |= n >> 2;
            n |= n >> 4;
            n |= n >> 8;
            n |= n >> 16;
            if constexpr (sizeof(size_type) > 4) {
                n |= n >> 32;
            }
            return ++n;
        }
    };

    // Non-member functions
    template<typename K, typename V, typename H, typename E, typename A>
    void swap(HashMap<K, V, H, E, A>& lhs, HashMap<K, V, H, E, A>& rhs) noexcept {
        lhs.swap(rhs);
    }

    // Type aliases for common use cases
    template<typename Key, typename Value>
    using HashMap32 = HashMap<Key, Value, DefaultHash<Key>, std::equal_to<Key>, PoolAllocator<std::pair<const Key, Value>, 32>>;

    template<typename Key, typename Value>
    using StringHashMap = HashMap<Key, Value, DefaultHash<Key>, std::equal_to<Key>, DefaultAllocator<std::pair<const Key, Value>>>;

} // namespace Akhanda::Containers