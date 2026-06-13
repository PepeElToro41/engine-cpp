#include "defines.hpp"
#include "allocation.hpp"
#include "platform/platform.hpp"

void* ALLOCATION::malloc(const usz size, const usz alignment, const bool zero, const MemoryTag tag) {
    if (zero) {
        void* block = PLATFORM::malloc(size, alignment);
        PLATFORM::memzero(block, size);
        return block;
    } else {
        return PLATFORM::malloc(size, alignment);
    }
}

void* ALLOCATION::realloc(void* ptr, const usz size, const usz alignment, MemoryTag tag) {
    return PLATFORM::realloc(ptr, size, alignment);
} 

void ALLOCATION::free(void* ptr) {
    PLATFORM::free(ptr);
}