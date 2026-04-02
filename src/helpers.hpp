#include <stdexcept>
#include <cstddef>

constexpr void unreachable(const char* msg) {
    throw std::logic_error(msg);
}

// TODO: create my own hashmap because std::unordered_map sucks

template <typename T>
inline T next_power_of_two(T value) {
    if (value <= 1) return (T)1;
    if (value == 2) return (T)2;

    T result = value - 1;

    result |= result >> 1;
    result |= result >> 2;
    result |= result >> 4;

    if (sizeof(T) >= 2)  result |= result >> 8; 
	if (sizeof(T) >= 4)  result |= result >> 16;
	if (sizeof(T) >= 8)  result |= result >> 32;
	if (sizeof(T) >= 16) result |= result >> 64;

    return result + 1;
}