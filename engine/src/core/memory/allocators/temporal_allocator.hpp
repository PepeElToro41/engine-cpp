#pragma once

#include "defines.hpp"
#include "core/memory/memory.hpp"
#include "arena_allocator.hpp"


inline constexpr usz MAIN_ARENA_BYTES = MEMORY::MB * 4;
inline thread_local char MAIN_ARENA_BUFFER[MAIN_ARENA_BYTES] = {};
inline thread_local ArenaAllocator MAIN_ARENA(MAIN_ARENA_BUFFER, MAIN_ARENA_BYTES);

// TODO: allow overflow page 

struct TemporalAllocator : BaseAllocator {
    usz arena_mark;

    TemporalAllocator(const TemporalAllocator&)            = delete;
    TemporalAllocator& operator=(const TemporalAllocator&) = delete;
    TemporalAllocator& operator=(TemporalAllocator&& other) noexcept {
        if (this != &other) {
            this->arena_mark = other.arena_mark;
            other.arena_mark = SIZE_MAX;
        }
        return *this;
    }
    
    void* allocate(const usz size, const usz alignment, const bool zero) override {
        if (this->arena_mark == SIZE_MAX) {
            this->arena_mark = MAIN_ARENA.offset;
        }
        return MAIN_ARENA.allocate(size, alignment, zero);
    }

    void* reallocate(void* ptr, const usz size, const usz alignment) override {
        return MAIN_ARENA.reallocate(ptr, size, alignment);
    }

    void free(void* ptr) override {}

    explicit TemporalAllocator(const usz arena_mark) noexcept
    : arena_mark(arena_mark) {}
	
    TemporalAllocator(TemporalAllocator&& other) noexcept 
    : arena_mark(other.arena_mark) {
        other.arena_mark = SIZE_MAX; // sentinel: moved-from
    }
    
    ~TemporalAllocator() override {
        if(this->arena_mark != SIZE_MAX) {
            MAIN_ARENA.reset_to(this->arena_mark);
        }
    }
    
    [[nodiscard]] inline static TemporalAllocator create() noexcept {
        return TemporalAllocator{MAIN_ARENA.offset};
    };
};