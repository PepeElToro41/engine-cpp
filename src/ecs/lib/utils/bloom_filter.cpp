#include <cstdint>
#include <cstddef>

#include "bloom_filter.hpp"

constexpr std::size_t FILTER_SIZE = sizeof(std::uint64_t) * 8;

std::uint64_t mix(std::uint64_t element) {
    element += 0x9E3779B97F4A7C15;
	element = (element ^ (element >> 30)) * 0xBF58476D1CE4E5B9;
	element = (element ^ (element >> 27)) * 0x94D049BB133111EB;
	element = element ^ (element >> 31);
	return element;
}

void BloomFilter::add(std::uint64_t value) {
    std::uint64_t hash1 = mix(value);
    std::uint64_t hash2 = mix(hash1);

    std::uint64_t bit1 = hash1 % FILTER_SIZE;
    std::uint64_t bit2 = hash2 % FILTER_SIZE;
    
    this->bitset |= (1ull << bit1);
    this->bitset |= (1ull << bit2);
}

bool BloomFilter::test(const BloomFilter& other) const {
    return (this->bitset & other.bitset) == other.bitset;
}