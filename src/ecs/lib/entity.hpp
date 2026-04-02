#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <cstddef>
#include <cstdint>

#include "types.hpp"

struct World;


namespace entity {
    inline bool is_pair(Id id);
    inline Id ecs_pair(Id first, Id second);
    inline EntityLow pair_first(Id pair);
    inline EntityLow pair_second(Id pair);


    void entity_add_id(World* world, Entity entity, Id id);
    void entity_set_id(World* world, Entity entity, Id id, void* data);
    
    bool entity_has_id(World* world, Id entity, Id id);
    void* entity_get_id(World* world, Id entity, Id id);

    void entity_remove_id(World* world, Entity entity, Id id);
}

struct EntityRecord {
    Archetype* archetype;

    std::size_t archetype_row;
    std::size_t dense;

    bool deleting;
};


#endif