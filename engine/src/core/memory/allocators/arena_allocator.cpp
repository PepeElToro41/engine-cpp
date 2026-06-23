#include "defines.hpp"
#include "core/memory/memory.hpp"
#include "core/memory/allocation.hpp"
#include "platform/platform.hpp"
#include "core/logging.hpp"
#include "arena_allocator.hpp"

void* ArenaAllocator::allocate(const usz size, const usz alignment, const bool zero) {
    if (this->data == nullptr) {
        this->data = (char*)ALLOCATION::malloc(this->arena_size, alignof(max_align_t), false);
    }
    
    const usz aligned_offset = (this->offset + alignment - 1) & ~(alignment - 1);
    this->offset = aligned_offset + size;
    if (this->offset >= this->arena_size) {
        LOG_FATAL("Memory arena out of memory")
        return nullptr;
    }

    void* block = &this->data[aligned_offset];
    if (zero) {
        PLATFORM::memzero(block, size);
    }
    return block;
}

void* ArenaAllocator::reallocate(void* ptr, const usz size, const usz alignment) {
    LOG_FATAL("Tried to reallocate memory in an arena")
    return this->allocate(size, alignment, true);
}

void ArenaAllocator::free(void* ptr) {}