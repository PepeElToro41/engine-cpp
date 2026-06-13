#pragma once

#include "defines.hpp"
#include "core/memory/memory.hpp"

namespace ALLOCATION {
    void* malloc(const usz size, const usz alignment, const bool zero, const MemoryTag tag = MemoryTag::UNKNOWN);
    void* realloc(void* ptr, const usz size, const usz alignment, const MemoryTag tag = MemoryTag::UNKNOWN);
    void free(void* ptr);
}