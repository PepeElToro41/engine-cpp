#include "heap_allocator.hpp"

void HeapAllocator::free(void* ptr) {
    ALLOCATION::free(ptr);
}

HeapAllocator HEAP_ALLOCATOR = HeapAllocator();