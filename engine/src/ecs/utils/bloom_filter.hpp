#pragma once

#include "defines.hpp"

constexpr usz BLOOM_FILTER_SIZE = sizeof(usz) * 8;

inline u64 mix(u64 element) {
	element += 0x9E3779B97F4A7C15;
	element = (element ^ (element >> 30)) * 0xBF58476D1CE4E5B9;
	element = (element ^ (element >> 27)) * 0x94D049BB133111EB;
	element = element ^ (element >> 31);
	return element;
}

struct BloomFilter {
	u64 bitset;

	explicit BloomFilter() {
		this->bitset = 0;
	}
	explicit BloomFilter(const u64 bitset) {
		this->bitset = bitset;
	}

	inline void add(const u64 value) {
		const u64 hash1 = mix(value);
		const u64 hash2 = mix(hash1);

		const u64 bit1 = hash1 % BLOOM_FILTER_SIZE;
		const u64 bit2 = hash2 % BLOOM_FILTER_SIZE;
    
		this->bitset |= (1ull << bit1);
		this->bitset |= (1ull << bit2);
	}
	inline bool test(const BloomFilter& other) const {
		return (this->bitset & other.bitset) == other.bitset;
	}
};
