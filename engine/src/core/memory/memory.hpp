#pragma once

#include <cstddef>
#include <cstdlib>

#include "defines.hpp"

enum MemoryTag {
    UNKNOWN,
    RENDERING,
    ECS_WORLD,
    ECS_ARCHETYPES,
    IMAGE_TEXTURE,
    
    MAX_TAG,
};


struct BaseAllocator {
    virtual ~BaseAllocator() = default;

    virtual void* allocate(const usz size, const usz alignment, const bool zero = true) = 0;
    virtual void* reallocate(void* ptr, const usz size, const usz alignment) = 0;
    virtual void free(void* ptr) = 0;
};

namespace MEMORY {
    constexpr usz MAX_ALIGN = alignof(std::max_align_t);

    constexpr usz KB = 1024;
    constexpr usz MB = 1024 * 1024;

    inline void* malloc(BaseAllocator* allocator, const usz size, const usz alignment, const bool zero = true) {
        return allocator->allocate(size, alignment, zero);
    }

    template <typename T>
    inline T* alloc(BaseAllocator* allocator, const usz count = 1, const bool zero = true) {
        void* block = allocator->allocate(count * sizeof(T), alignof(T), zero);
        return (T*)block;
    }

    inline void* mrealloc(BaseAllocator* allocator, void* ptr, const usz size, const usz alignment) {
        return allocator->reallocate(ptr, size, alignment);
    }

    template <typename T>
    inline T* realloc(BaseAllocator* allocator, T* ptr, const usz count) {
        void* block = allocator->reallocate(ptr, count * sizeof(T), alignof(T));
        return (T*)block;
    }

    inline void free(BaseAllocator* allocator, void* ptr) {
        allocator->free(ptr);
    }
}
