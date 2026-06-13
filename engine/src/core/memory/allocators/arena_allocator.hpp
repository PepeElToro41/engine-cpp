#pragma once

#include "defines.hpp"
#include "core/memory/memory.hpp"
#include "core/memory/allocation.hpp"

struct ArenaAllocator : BaseAllocator {
    size_t arena_size;
    char* data = nullptr;
    size_t offset = 0;

    explicit ArenaAllocator(const usz arena_size) {
        this->arena_size = arena_size;
        this->data = (char*)ALLOCATION::malloc(this->arena_size, MEMORY::MAX_ALIGN, false);
    }
    ArenaAllocator(void* buffer, const usz buffer_len) {
        this->arena_size = buffer_len;
        this->data = (char*)buffer;
    }

    inline void reset()  {
    	this->offset = 0;
    }
    inline void reset_to(const usz new_offset) {
        this->offset = new_offset;
    }

    void* allocate(const usz size, const usz alignment, const bool zero) override;
    void* reallocate(void* ptr, const usz size, const usz alignment) override;
    void free(void* ptr) override;
};  