#pragma once

#include "core/memory/memory.hpp"
#include "templates/hashmap.hpp"
#include "templates/sparse_list.hpp"
#include "core/logging.hpp"
#include "ecs/ecs_types.hpp"
#include "ecs/archetype.hpp"
#include "ecs/component_record.hpp"
#include "ecs/entity_index.hpp"
#include "ecs/entity.hpp"

struct World {
	BaseAllocator* allocator;
    Archetype* root_archetype = nullptr;
	Id next_component_id = 0;
	
    EntityIndex entity_index;
    SparseList<Archetype> archetypes;
    SparseList<ComponentRecord> component_records;
    
    HashMap<usz, Id> type_components_lookup;
    HashMap<Id, TypeInfo> component_type_index;

    HashMap<ArchetypeType, Archetype*> archetype_index;
    HashMap<Id, ComponentRecord*> component_index;
	
	explicit World(BaseAllocator* allocator) : 
		allocator(allocator),
	
		entity_index(allocator),
		archetypes(allocator),
		component_records(allocator),
	
		type_components_lookup(allocator),
		component_type_index(allocator),
	
		archetype_index(allocator),	
		component_index(allocator)
	{};
	
	void init();
	void destroy();
	ComponentId new_component_id();
	EntityId tag();
	
	EntityId new_entity();
	void delete_entity(EntityId entity);
	
	
	template <typename T>
	ComponentId component() {
		TypeInfo type_info = {
			.length = sizeof(T),
			.alignment = alignof(T),
		};
		
		const ComponentId component = this->new_component_id();
		this->type_components_lookup.set(typeid(T).hash_code(), component);
		return component;
	}
	
	inline Id id_from_type(const usz type_hash) const {
		const auto id = this->type_components_lookup.get(type_hash);
		if (!id.has_value()) {
			LOG_ERROR("[ECS]: Attempted to use a non registered component")
			return 0;
		}
		return id.value();
	}
	
	template <typename T>
	void entity_set(const EntityId entity, const T* data) {
		if (const Id id = this->id_from_type(typeid(T).hash_code()); id ) {
			ENTITY::set_id(this, entity, id, data);
		}
	}
	
	template <typename T>
	bool entity_has(const EntityId entity) const {
		if (const Id id = this->id_from_type(typeid(T).hash_code()); id ) {
			return ENTITY::has_id(this, entity, id);
		}
		return false;
	}
	
	template <typename T>
	T* entity_get(const EntityId entity) const {
		if (const Id id = this->id_from_type(typeid(T).hash_code()); id ) {
			return (T*)ENTITY::get_id(this, entity, id);
		}
		return nullptr;
	}
	
	bool entity_has(const EntityId entity, const Id id) const {
		return ENTITY::has_id(this, entity, id);
	}
	void entity_add(const EntityId entity, const Id id) {
		ENTITY::add_id(this, entity, id);
	}
};