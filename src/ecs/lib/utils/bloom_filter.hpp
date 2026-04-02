#include <cstdint>


struct BloomFilter {
    std::uint64_t bitset;

    void add(std::uint64_t value);
    bool test(const BloomFilter& other) const;
};
