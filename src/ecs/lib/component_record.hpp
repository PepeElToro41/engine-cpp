#ifndef COMPONENT_RECORD_HPP
#define COMPONENT_RECORD_HPP
 
#include <unordered_map>
#include <cstddef>

#include "types.hpp"
#include "utils/sparse_list.hpp"


struct World;

namespace component_record {
    constexpr Bitflags FLAG_IS_PAIR            = 1 << 0;
    constexpr Bitflags FLAG_IS_EXCLUSIVE       = 1 << 1;
    constexpr Bitflags FLAG_HAS_DELETE         = 1 << 2;
    constexpr Bitflags FLAG_IS_TRAVERSABLE     = 1 << 3;
    constexpr Bitflags FLAG_IS_COMPONENT       = 1 << 4;
    constexpr Bitflags FLAG_IS_TGT_WILDCARD    = 1 << 5;
    constexpr Bitflags FLAG_IS_REL_WILDCARD    = 1 << 6;
}

struct PairRecord {
    std::unordered_map<ArchetypeId, std::size_t> pairs_count;

    // all pairs with (R, *)
    std::unordered_map<Pair, ComponentRecord*> first_records;
    // all pairs with (*, T)
    std::unordered_map<Pair, ComponentRecord*> second_records;
    // all pairs with (R, T) where R has Traversable trait
    std::unordered_map<Pair, ComponentRecord*> trav_records;
};


struct ComponentRecord {
    std::unordered_map<ArchetypeId, std::size_t> columns_index;
    SparseId sparse_id;

    std::size_t archetype_count;
    TypeInfo type_info;
    Bitflags flags;
    PairRecord pair_record;
};

namespace component_record {
    ComponentRecord* component_record_create(World* world, ComponentId component);
    ComponentRecord* component_record_ensure(World* world, ComponentId component);
}

#endif