#pragma once

#include "ecs/ecs_types.hpp"

struct World;

namespace ENTITY {
	void add_id(World* world, const EntityId entity, const Id id);
	void set_id(World* world, const EntityId entity, const Id id, const void* data);
	
	bool has_id(const World* world, const EntityId entity, const Id id);
	void* get_id(const World* world, const EntityId entity, const Id id);
	
	void remove_id(World* world, const EntityId entity, const Id id);
}