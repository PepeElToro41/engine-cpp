#pragma once

#include "ecs/ecs_types.hpp"
#include "defines.hpp"

using EntityId = u64;
using EntityIdLow = u64;
using EntityGeneration = u64;

namespace ECS {
	constexpr u64 ENTITY_BITS = 32;
	constexpr u64 ENTITY_SIZE = 1ull << ENTITY_BITS;
	constexpr u64 ENTITY_MASK = ENTITY_SIZE - 1;
	
	constexpr u64 GENERATION_BITS = 32;
	constexpr u64 GENERATION_SIZE = 1ull << GENERATION_BITS;
	
	constexpr u64 PAIR_FLAG = 1 << 63;
	constexpr Id  MAX_COMPONENT_ID = 1024;
	
	constexpr Id NAME				= MAX_COMPONENT_ID + 1;
	constexpr Id COMPONENT			= MAX_COMPONENT_ID + 2;
	constexpr Id WILDCARD			= MAX_COMPONENT_ID + 3;
	
	constexpr Id ON_DELETE_TARGET	= MAX_COMPONENT_ID + 4;
	constexpr Id ON_DELETE			= MAX_COMPONENT_ID + 5;
	
	constexpr Id DELETE				= MAX_COMPONENT_ID + 6;
	constexpr Id REMOVE				= MAX_COMPONENT_ID + 7;
	
	constexpr Id EXCLUSIVE			= MAX_COMPONENT_ID + 8;
	constexpr Id FORCE_TAG			= MAX_COMPONENT_ID + 9;
	constexpr Id TRAVERSABLE		= MAX_COMPONENT_ID + 10;
	
	constexpr Id REST				= MAX_COMPONENT_ID + 11;
	
	inline EntityIdLow ENTITY_ID(const EntityId id) {
		return id & ECS::ENTITY_MASK;
	}
	inline bool IS_PAIR(const Id id) {
		return (id & PAIR_FLAG);
	}
	inline Id PAIR(const Id rel, const Id tgt) {
		return (ENTITY_ID(rel) << ENTITY_BITS | ENTITY_ID(tgt)) | PAIR_FLAG;
	}
	inline EntityIdLow PAIR_FIRST(const Id id) {
		return id >> ENTITY_BITS;
	}
	inline EntityIdLow PAIR_SECOND(const Id id) {
		return id & ENTITY_MASK;
	}
}