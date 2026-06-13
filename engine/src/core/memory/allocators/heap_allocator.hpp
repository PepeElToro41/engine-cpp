#pragma once

#include "defines.hpp"
#include "core/memory/allocation.hpp"
#include "core/memory/memory.hpp"

struct HeapAllocator : BaseAllocator {
    MemoryTag memory_tag;
    
    explicit HeapAllocator(const MemoryTag tag = MemoryTag::UNKNOWN) {
        this->memory_tag = tag;
    }
	
    void* allocate(const usz size, const usz alignment, const bool zero) override {
        return ALLOCATION::malloc(size, alignment, zero, this->memory_tag);
    }
    void* reallocate(void* ptr, const usz size, const usz alignment) override {
        return ALLOCATION::realloc(ptr, size, alignment, this->memory_tag);
    }
    void free(void* ptr) override;
};

extern HeapAllocator HEAP_ALLOCATOR;