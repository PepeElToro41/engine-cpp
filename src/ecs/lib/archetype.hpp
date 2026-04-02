#ifndef ARCHETYPE_HPP
#define ARCHETYPE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <bitset>
#include <optional>

#include "utils/bloom_filter.hpp"
#include "types.hpp"

struct World;
struct EntityRecord;

struct ArchetypeType {
    std::uint64_t cached_hash;
    bool is_cached;
    std::vector<Id> ids;

    ArchetypeType clone(
        std::allocator<Id> allocator, 
        std::size_t reserve = 0
    );
};

struct ArchetypeTypeHash {
    std::uint64_t operator()(ArchetypeType& type);
};

namespace archetype {
    constexpr std::size_t BITSET_MATCH_SIZE = 1024;
    constexpr std::size_t ARCHETYPE_PREALLOCATED_SIZE = 16;
}

struct ArchetypeColumn {
    ArrPtr<void> data;
    TypeInfo type_info;
};

struct ArchetypeData {
    ArrPtr<Entity> entities;
    ArrPtr<ArchetypeColumn> columns;

    std::size_t columns_count;
    std::size_t rows_count;
    std::size_t rows_capacity;
};

struct Archetype {
    World* world;

    ArchetypeId archetype_id;
    ArchetypeType archetype_type;

    bool alive;

    std::unordered_map<Id, std::size_t> columns_index;
    std::unordered_map<Id, ArchetypeColumn*, ArchetypeTypeHash> columns_map;

    ArchetypeData data;
    
    BloomFilter bloom_filter;
    std::bitset<archetype::BITSET_MATCH_SIZE> bitmask;

    std::unordered_map<Id, Archetype*> forward_edges;
    std::unordered_map<Id, Archetype*> backwards_edges;

    std::optional<ArchetypeColumn*> get_column(Id id) const;

    bool has_id(Id id) const;
    void ensure_capacity(std::size_t capacity);
    void mark_alive();
    void mark_dead();
};


namespace archetype {
    Archetype* create_archetype(World* world, ArchetypeType archetype_type);
    Archetype* ensure_archetype(World* world, ArchetypeType archetype_type);

    Archetype* traverse_add(World* world, Archetype* source, Id entity);
    Archetype* traverse_remove(World* world, Archetype* source, Id entity);

    void insert_entity(World* world, Archetype* archetype, Entity entity, EntityRecord* record);
    void delete_entity(World* world, Archetype* archetype, Entity entity, EntityRecord* record);
    void move_entity(World* world, Archetype* source, Archetype* destination, Entity entity, EntityRecord* record);

    void write_column(ArchetypeColumn* column, std::size_t row, const void* data);
    void* read_column(const ArchetypeColumn* column, std::size_t row);
}

#endif