
#include "ecs/component_record.hpp"
#include "ecs/ecs.hpp"
#include "ecs/world.hpp"
#include "ecs/ecs_types.hpp"
#include "ecs/entity.hpp"
#include "core/logging.hpp"

static TypeInfo* get_type_info(const World* world, const Id id, const Id target) {
	if (target && ENTITY::has_id(world, id, ECS::FORCE_TAG)) {
		return nullptr;
	}
	
	bool is_component = ENTITY::has_id(world, id, ECS::COMPONENT);
	Id type_source = id;
	
	if (!is_component && target) {
		is_component = ENTITY::has_id(world, target, ECS::COMPONENT);
		if (is_component) {
			type_source = target;
		}
	}
	if (!is_component) return nullptr;
	
	const auto info = world->component_type_index.get_ref(type_source);
	if (info.has_value()) {
		return info.value();
	}
	LOG_ERROR("[ECS]: Component does not have a type info linked to it")
	return nullptr;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void ComponentRecord::destroy(World* world) {
	MEMORY::free(world->allocator, this->pair_record);
	
	world->component_index.remove(this->id);
	world->component_records.delete_element(this->sparse_id);
}

ComponentRecord* ComponentRecord::component_record_create(World* world, const Id id) {
	ComponentRecord* new_record = world->component_records.new_element();
	new_record->sparse_id = world->component_records.get_latest_alive();
	new_record->archetype_count = 0;
	new_record->id = id;
	
	bool is_pair = ECS::IS_PAIR(id);
	bool is_exclusive = false;
	bool has_delete = false;
	bool is_traversable = false;
	
	Id relation = id;
	Id target = 0;
	
	if (is_pair) {
		relation = world->entity_index.entity_get_alive(ECS::PAIR_FIRST(id));
		target = world->entity_index.entity_get_alive(ECS::PAIR_SECOND(id));
		
		if (!relation) {
			LOG_FATAL("[ECS]: Cannot get alive relation for component record pair")
		}
		if (!target) {
			LOG_FATAL("[ECS]: Cannot get alive target for component record pair")
		}
		
		has_delete = ENTITY::has_id(world, relation, ECS::PAIR(ECS::ON_DELETE_TARGET, ECS::DELETE));
		is_exclusive = ENTITY::has_id(world, relation, ECS::EXCLUSIVE);
		is_traversable = ENTITY::has_id(world, relation, ECS::TRAVERSABLE);
		
		new_record->pair_record = MEMORY::alloc<PairRecord>(world->allocator);
	} else {
		new_record->pair_record = nullptr;
	}
	
	TypeInfo* type_info = get_type_info(world, relation, target);
	const bool is_component = type_info != nullptr;
	
	new_record->type_info = type_info;
	new_record->flags = 0
		| (is_component ? ComponentRecordFlags::IS_COMPONENT : 0)
		| (is_exclusive ? ComponentRecordFlags::HAS_DELETE : 0)
		| (has_delete ? ComponentRecordFlags::HAS_DELETE : 0)
		| (is_traversable ? ComponentRecordFlags::IS_TRAVERSABLE : 0);
	
	world->component_index.set(id, new_record);
	return new_record;
}


ComponentRecord* ComponentRecord::component_record_ensure(World* world, const Id id) {
	const auto record = world->component_index.get(id);
	if (record.has_value()) {
		return record.value();
	}
	if (ECS::IS_PAIR(id)) {
		ComponentRecord* new_record = ComponentRecord::component_record_create(world, id);
		const Id relation_wildcard = ECS::PAIR(ECS::PAIR_FIRST(id), ECS::WILDCARD);
		const Id target_wildcard = ECS::PAIR(ECS::WILDCARD, ECS::PAIR_SECOND(id));
		
		if (relation_wildcard == id || target_wildcard == id) return new_record;
		
		const ComponentRecord* relation_record = ComponentRecord::component_record_ensure(world, relation_wildcard);
		relation_record->pair_record->first_records.set(id, new_record);
		
		const ComponentRecord* target_record = ComponentRecord::component_record_ensure(world, id);
		target_record->pair_record->second_records.set(id, new_record);
		
		if (relation_record->flags & ComponentRecordFlags::IS_TRAVERSABLE) {
			target_record->pair_record->trav_records.set(id, new_record);
		}
		
		return new_record;
	} else {
		return ComponentRecord::component_record_create(world, id);
	}
} 

ComponentRecord* ComponentRecord::component_record_get(const World* world, const Id id) {
	const auto record = world->component_index.get(id);
	if (record.has_value()) {
		return record.value();
	}
	return nullptr;
}

