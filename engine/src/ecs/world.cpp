#include "ecs/ecs.hpp"
#include "ecs/world.hpp"
#include "ecs/archetype.hpp"
#include "core/logging.hpp"

void World::init() {
	this->root_archetype = Archetype::create_archetype(this, {});
	
	for (int i = 0; i < ECS::REST; i++) {
		this->new_entity();
	}
	ENTITY::add_id(this, ECS::NAME, ECS::COMPONENT);
	ENTITY::add_id(this, ECS::REST, ECS::COMPONENT);
}

void World::destroy() {
	this->component_type_index.free();
	this->type_components_lookup.free();
	
	this->entity_index.free();
	this->archetypes.free();
	this->component_records.free();
	
	this->archetype_index.free();
	this->component_index.free();
}

ComponentId World::new_component_id() {
	const ComponentId component_id = ++this->next_component_id;
	if (component_id > WORLD::MAX_COMPONENT_ID) {
		LOG_FATAL("Max components reached")
		std::abort();
	}
	
	return component_id;
}

EntityId World::tag() {
	const EntityId tag_id = ++this->next_component_id;
	if (tag_id > WORLD::MAX_COMPONENT_ID) {
		LOG_FATAL("Max components reached")
		std::abort();
	}
	
	return tag_id;
}

EntityId World::new_entity() {
	EntityRecord* new_record;
	const EntityId new_entity = this->entity_index.new_entity(&new_record);
	new_record->archetype = this->root_archetype;
	new_record->archetype_row = 0;
	
	return new_entity;
}

void World::delete_entity(const EntityId entity) {
	EntityRecord* record = this->entity_index.entity_record_get_alive(entity);
	// TODO: delete entity from the archetype
	this->entity_index.delete_entity(entity, record);
}