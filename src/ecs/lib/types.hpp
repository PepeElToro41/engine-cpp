#ifndef ECS_TYPES_HPP
#define ECS_TYPES_HPP

#include <cstddef>
#include <cstdint>
#include <string>

struct Range {
    std::size_t start;
    std::size_t end;

    bool contains(std::size_t index) {
        return index >= start && index < end;
    };
};

struct TypeInfo {
    std::size_t type_size;
    std::size_t alignment;

    Entity component;
    std::string name;
};

template <typename T>
using ArrPtr = T*;

using Bitflags = uint64_t;
using ArchetypeId = uint64_t;
using Entity = uint64_t;
using EntityLow = uint64_t;
using ComponentId = uint64_t;
using Id = uint64_t;
using Pair = uint64_t;

#endif