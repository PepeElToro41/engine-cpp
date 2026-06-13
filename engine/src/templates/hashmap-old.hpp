#pragma once

#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "helpers.hpp"
#include "defines.hpp"
#include "core/memory/memory.hpp"

namespace HASHMAP {
    constexpr usz DEFAULT_INITIAL_CAPACITY = 16;
    constexpr float DEFAULT_LOAD_FACTOR = 0.75f;
}


template <typename Key, typename Value> 
struct HashEntry {
    usz hash;
    Key key;
    Value data;
    HashEntry* next = nullptr;
};

template <
    typename Key,
    typename Value,     
    typename Hasher         = std::hash<Key>,
    typename KeyEqual       = std::equal_to<Key>
>
struct HashMap {
    u32 count = 0;
    u32 capacity = 0;
    u32 threshold;
    float load_factor;
    KeyEqual key_equal = {};
    Hasher hasher = {};

    using Entry = HashEntry<const Key, Value>;

    BaseAllocator* allocator;
    Entry* buckets;

    explicit HashMap(
        BaseAllocator* allocator,
        u32 initial_capacity = HASHMAP::DEFAULT_INITIAL_CAPACITY,
        float load_factor = HASHMAP::DEFAULT_LOAD_FACTOR
    ) {
        initial_capacity = MATH::next_power_of_two(initial_capacity);
        
        this->allocator = allocator;
        this->load_factor = load_factor;
        this->threshold = (u32)(initial_capacity * load_factor);
        this->buckets = MEMORY::alloc<Entry*>(allocator, initial_capacity);
    }
    
    bool contains(Key key) const {
        if (!this->count) {
            return false;
        }
        u64 hash = this->hasher(key);
        u32 index = this->bucket_for(hash);
        
        for (Entry* entry = this->buckets[index]; entry != nullptr; entry = entry->next) {
            if (entry->hash == hash && key_equal(entry->key, key)) {
                return true;
            }
        }
        return false;
    }


    std::optional<Value*> get_ref(Key key) const {        
        if (!this->count) {
            return {};
        }
        u64 hash = this->hasher(key);
        u32 index = this->bucket_for(hash);
        
        for (Entry* entry = this->buckets[index]; entry != nullptr; entry = entry->next) {
            if (entry->hash == hash && key_equal(entry->key, key)) {
                return &entry->data;
            }
        }
        return {};
    }

    std::optional<Value> get(Key key) const {
        auto value_ref = this->get_ref(key);
        if (value_ref.has_value()) {
            return *value_ref;
        }
        return {};
    }

    bool set(Key key, Value value) {
        u64 hash = this->hasher(key);
        u32 index = this->bucket_for(key);

        for (Entry* entry = this->buckets[index]; entry != nullptr; entry = entry->next) {
            if (entry->hash == hash && key_equal(entry->key, key)) {
                entry->value = value;
                return false;
            }
        }

        Entry* new_entry = MEMORY::alloc<Entry>(this->allocator);
        new_entry->hash = hash;
        new_entry->key = key;
        new_entry->value = value;
        new_entry->next = this->buckets[index];

        this->buckets[index] = new_entry;
        if (this->count++ >= this->threshold) {
            this->resize(this->capacity * 2);
        }
        return true;
    }

    bool remove(Key key) {
        if (!this->count) {
            return false;
        }

        u64 hash = this->hasher(key);
        u32 index = this->bucket_for(hash);

        Entry* previous = nullptr;
        Entry* entry = this->buckets[index];

        while (entry != nullptr) {
            Entry* next = entry->next;

            if (entry->hash == hash && key_equal(entry->key, key)) {
                --this->count;

                if (previous != nullptr) {
                    previous->next = next;
                } else {
                    this->buckets[index] = next;
                }

                this->free_entry(entry);
                return true;
            }

            previous = entry;
            entry = next;
        }
        return false;
    }

    void each(std::function<void(Key, Value)> callback) {
        for (u32 i = 0; i < this->capacity; i++) {
            Entry* entry = this->buckets[i];
            while (entry != nullptr) {
                callback(entry->key, entry->value);
                entry = entry->next;
            }
        }
    }

    void each_ref(std::function<void(Key, Value*)> callback) {
        for (u32 i = 0; i < this->capacity; i++) {
            Entry* entry = this->buckets[i];
            while (entry != nullptr) {
                callback(entry->key, &entry->value);
                entry = entry->next;
            }
        }
    }

    void free() {
        for (u32 i = 0; i < this->capacity; i++) {
            Entry* bucket = this->buckets[i];
            // TODO: free
        }
    }

private:
    void resize(u32 new_capacity) {
        Entry* old_buckets = this->buckets;
        Entry* new_buckets = MEMORY::alloc<Entry*>(this->allocator, new_capacity);
        this->capacity = new_capacity;

        for (u32 i = 0; i < this->capacity; i++) {
            Entry* entry = this->buckets[i];
            while (entry != nullptr) {
                Entry* next = entry->next;
                const u32 index = this->bucket_for(entry->hash);
                entry->next = new_buckets[index];
                new_buckets[index] = entry;
                
                entry = next;
            }
        }

        this->buckets = new_buckets;
        MEMORY::free(this->allocator, old_buckets);
        this->threshold = (u32)(new_capacity * this->load_factor);
    }

    void free_entry(Entry* entry) {
        MEMORY::free(this->allocator, entry);
    }

    inline u32 bucket_for(u64 hash) {
        return hash & (this->capacity - 1);
    }
};