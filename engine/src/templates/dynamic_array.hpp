#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <algorithm>

#include "helpers.hpp"
#include "export.h"
#include "defines.hpp"
#include "core/memory/allocators/heap_allocator.hpp"
#include "core/memory/memory.hpp"
#include "platform/platform.hpp"

namespace DYNAMIC_ARRAY {
    constexpr usz DEFAULT_CAPACITY = 16;
}

template <typename T>
struct ENGINE_API DynamicArray {
    usz size = 0;
    BaseAllocator* allocator;

    T* begin() { 
        return this->entries; 
    }
    T* end() { 
        return this->entries + this->size;
    }

    const T* begin() const { 
        return this->entries; 
    }
    const T* end() const { 
        return this->entries + this->size;
    }

    explicit DynamicArray() {
        this->allocator = &HEAP_ALLOCATOR;
    }
    explicit DynamicArray(BaseAllocator* allocator) :
		allocator(allocator)
	{}
    void init(const usz start_capacity = DYNAMIC_ARRAY::DEFAULT_CAPACITY) {
        this->reserve(start_capacity);
    }
    void init(const T* begin, const T* end) {
        const usz span = end - begin;
        this->reserve(span);
        PLATFORM::memcpy(this->entries, begin, span * sizeof(T));
    }

    void free() {
        this->clear();
        MEMORY::free(this->allocator, this->entries);
    }

    void insert(usz index, T value) {
        this->reserve(1);
        PLATFORM::memcpy(&this->entries[index + 1], &this->entries[index], (this->size - index) * sizeof(T));
        this->entries[index] = value;
        ++this->size;
    }

    void remove(usz index) {
        PLATFORM::memcpy(&this->entries[index], &this->entries[index + 1], (this->size - index - 1) * sizeof(T));
        --this->size;
    }

    inline void set(usz index, T value) {
        this->entries[index] = value;
    }
    
    inline T get(usz index) const {
        return this->entries[index];
    }
    inline T last() const {
        return this->entries[this->size - 1];
    }

    inline T* get_ref(usz index) const {
        return &this->entries[index];
    }
    inline T* last_ref() const {
        return &this->entries[this->size - 1];
    }

    inline void clear() {
        this->size = 0;
    }

    void push(T value) {
        this->reserve(1);
        this->entries[this->size++] = value;
    }

    inline T pop() {
        return this->entries[--this->size];
    }
    inline T* pop_ref() {
        return &this->entries[--this->size];
    }

    DynamicArray<T> clone(BaseAllocator* allocator) {
        DynamicArray<T> new_array(allocator);
        if (this->size) {
            new_array.init(this->size);
            PLATFORM::memcpy(new_array.entries, this->entries, this->size * sizeof(T));
        }
        return new_array;
    }
    DynamicArray<T> clone() {
        return this->clone(this->allocator);
    }

    void reserve(usz count) {
        usz new_size = this->size + count;
        if (this->capacity >= new_size) return;

        usz new_capacity = this->capacity ? this->capacity * 2 : DYNAMIC_ARRAY::DEFAULT_CAPACITY;
        while (new_capacity < new_size) {
            new_capacity *= 2;
        }
        this->ensure_capacity(new_capacity);
    }
    inline T* array() {
        return this->entries;
    }

    inline void iter(void(*callback)(usz, T&)) {
        for (usz i = 0; i < this->size; ++i) {
            callback(i, this->entries[i]);
        }
    }

    void fill(usz count) {
        this->ensure_capacity(count);
        this->size = std::max(count, this->size);
    }

    void ensure_capacity(usz new_capacity) {
        if (!new_capacity) return;
        if (this->capacity >= new_capacity) return;

        new_capacity = MATH::next_power_of_two(new_capacity);
  
        if (this->entries) {
            this->entries = MEMORY::realloc<T>(this->allocator, this->entries, new_capacity);
        } else {
            this->entries = MEMORY::alloc<T>(this->allocator, new_capacity);
        }
        this->capacity = new_capacity;
    }
	
private:
	usz capacity = 0;
	T* entries = nullptr;
};  