#pragma once

#include "ecs_types.hpp"
#include "defines.hpp"
#include "core/memory/memory.hpp"
#include "templates/hashmap.hpp"

struct World;

enum ComponentRecordFlags {
	IS_COMPONENT =		1 << 0,
	IS_EXCLUSIVE =		1 << 1,
	HAS_DELETE =		1 << 2,
	IS_TRAVERSABLE =	1 << 3
};

struct ComponentRecord;

struct PairRecord {
	HashMap<Id, u32> pairs_count;
	
	// all pairs with (R, *)
	HashMap<Id, ComponentRecord*> first_records;
	// all pairs with (*, T)
	HashMap<Id, ComponentRecord*> second_records;
	// all pairs with (R, T) where R has Traversable trait
	// this will be present in a (*, T) wildcard record
	HashMap<Id, ComponentRecord*> trav_records;
};

struct ComponentRecord {
    u64 sparse_id;
	u32 archetype_count;
    TypeInfo* type_info;
	Id id;
    
    Bitset flags;
	
	// only allocated in pairs
	PairRecord* pair_record;
	
	void destroy(World* world);
	
	static ComponentRecord* component_record_create(World* world, Id id);
	static ComponentRecord* component_record_ensure(World* world, Id id);
	static ComponentRecord* component_record_get(const World* world, Id id);
};