#pragma once

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <source_location>

#include "defines.hpp"

#define PANIC(msg) \
    do { \
        std::cerr << "PANIC at " << __FILE__ << ":" << __LINE__ \
                  << " (" << __func__ << "): " << msg << "\n"; \
        std::abort(); \
    } while(0)

namespace MATH {
    template <typename T>
    inline T next_power_of_two(T value) {
        if (value <= 1) return (T)2;

        T result = value - 1;

        result |= result >> 1;
        result |= result >> 2;
        result |= result >> 4;

        if constexpr (sizeof(T) >= 2)  result |= result >> 8;
        if constexpr (sizeof(T) >= 4)  result |= result >> 16;
        if constexpr (sizeof(T) >= 8)  result |= result >> 32;
        if constexpr (sizeof(T) >= 16) result |= result >> 64;

        return result + 1;
    }

    // INFO: alignment must be a power of two
    inline usz next_aligned(usz value, usz alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }
}
