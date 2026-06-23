#include "ecs/entity.hpp"
#include "ecs/entity_index.hpp"
#include "ecs/archetype.hpp"
#include "ecs/world.hpp"

bool ENTITY::has_id(const World* world, const EntityId entity, const Id id) {
	const EntityRecord* record = world->entity_index.entity_record_get_alive(entity);
	
	const Archetype* archetype = record->archetype;
	return archetype->has_id(id);
}

void* ENTITY::get_id(const World* world, const EntityId entity, const Id id) {
	const EntityRecord* record = world->entity_index.entity_record_get_alive(entity);
	
	const Archetype* archetype = record->archetype;
	const ArchetypeColumn* column = archetype->get_column(id);
	return column->read(record->archetype_row);
}

void ENTITY::add_id(World* world, const EntityId entity, const Id id) {
	EntityRecord* record = world->entity_index.entity_record_get_alive(entity);
	Archetype* source = record->archetype;
	
	if (source->has_id(id)) {
		return;
	}
	
	Archetype* destination = source->traverse_add(world, id);
	if (source == world->root_archetype) {
		destination->insert_entity(world, entity, record);
	} else {
		source->move_entity(world, destination, entity, record);
	}
}

void ENTITY::set_id(World* world, const EntityId entity, const Id id, const void* data) {
	EntityRecord* record = world->entity_index.entity_record_get_alive(entity);
	Archetype* source = record->archetype;
	
	if (source->has_id(id)) {
		ArchetypeColumn* column = source->get_column(id);
		if (column != nullptr) {
			column->write(record->archetype_row, data);
		}
		return;
	}
	
	Archetype* destination = source->traverse_add(world, id);
	if (source == world->root_archetype) {
		destination->insert_entity(world, entity, record);
	} else {
		source->move_entity(world, destination, entity, record);
	}
	
	ArchetypeColumn* column = destination->get_column(id);
	if (column != nullptr) {
		column->write(record->archetype_row, data);
	}
}

void ENTITY::remove_id(World* world, EntityId entity, const Id id) {
	EntityRecord* record = world->entity_index.entity_record_get_alive(entity);
	Archetype* source = record->archetype;
	
	if (!source->has_id(id)) {
		return;
	}
	
	Archetype* destination = source->traverse_remove(world, id);
	
	if (destination == world->root_archetype) {
		source->delete_entity(world, entity, record);
	} else {
		source->move_entity(world, destination, entity, record);
	}
}