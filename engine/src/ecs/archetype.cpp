#include "defines.hpp"
#include "archetype.hpp"
#include "templates/hashmap.hpp"
#include "core/memory/memory.hpp"
#include "core/memory/allocators/temporal_allocator.hpp"
#include "platform/platform.hpp"
#include "ecs/ecs_types.hpp"
#include "ecs/world.hpp"
#include "ecs/entity_index.hpp"
#include "ecs/component_record.hpp"

#include <assert.h>

ArchetypeType ArchetypeType::clone(BaseAllocator* allocator, const usz reserve) const {
    const usz count = this->id_count + reserve;
    const ArchetypeType new_type(
        MEMORY::alloc<Id>(allocator, count),
        count
    );
    
    return new_type;
}

ArchetypeType ArchetypeType::insert(BaseAllocator* allocator, const Id id) const {
    const ArchetypeType new_type = this->clone(allocator, 1);

    // TODO: use binary search
    for (usz i = 0; i < this->id_count; i++) {
        if (this->ids[i] > id) {
            PLATFORM::memcpy(&new_type.ids[i + 1], &new_type.ids[i], (this->id_count - i) * sizeof(Id));
            new_type.ids[i] = id;
            break;
        }
    }

    return new_type;
}

ArchetypeType ArchetypeType::remove(BaseAllocator* allocator, const Id id) const {
    // TODO: use binary search
    Id* ids = MEMORY::alloc<Id>(allocator, this->id_count - 1);
    usz pushed = 0;
    for (usz i = 0; i < this->id_count; i++) {
        if (this->ids[i] != id) {
            ids[pushed++] = this->ids[i];
        }
    }

    return {ids, this->id_count - 1};
}

void ArchetypeType::free(BaseAllocator* allocator) {
    MEMORY::free(allocator, this->ids);
	this->ids = nullptr;
}

void Archetype::mark_alive() {
    this->alive = true;
}
void Archetype::mark_dead() {
    this->alive = false;
}

void Archetype::ensure_capacity(const usz capacity) {
    if (this->data.entity_capacity >= capacity) return;

    usz new_capacity = this->data.entity_capacity > 0
        ? this->data.entity_capacity * 2
        : ARCHETYPE::PREALLOCATED_SIZE;

    while (new_capacity < capacity) {
        new_capacity *= 2;
    }

    new_capacity = MATH::next_power_of_two(new_capacity);

    this->data.entities = MEMORY::realloc<EntityId>(this->allocator, this->data.entities, new_capacity);

    for (usz i = 0; i < this->data.column_count; i++) {
        ArchetypeColumn* column = &this->data.columns[i];
        if (column->data == nullptr) continue;

        TypeInfo* type_info = column->type_info;
        column->data = MEMORY::mrealloc(this->allocator, column->data, new_capacity * type_info->length, type_info->alignment);
    }
}

void Archetype::insert_entity(World* world, const EntityId entity, EntityRecord* record) {
    const usz destination_row = this->data.entity_count++;
    this->ensure_capacity(destination_row);
    this->data.entities[destination_row] = entity;
    
    record->archetype = this;
    record->archetype_row = destination_row;
    
    if (destination_row == 0) {
        this->mark_alive();
    }
}

void Archetype::delete_entity(const World* world, EntityId entity, EntityRecord* record) {
    const usz row = record->archetype_row;
    const usz last_row = --this->data.entity_count;
    
    if (row != last_row) {
        const EntityId swapped = this->data.entities[last_row];
        EntityRecord* swapped_record = world->entity_index.entity_record_get(swapped);
        swapped_record->archetype_row = row;
        this->data.entities[row] = swapped;

        for (usz i = 0; i < this->data.column_count; i++) {
            ArchetypeColumn* column = &this->data.columns[i];
            if (column->data == nullptr) continue;

            const void* last_value = column->read(last_row);
            column->write(row, last_value);
        }
    }

    record->archetype = world->root_archetype;
    record->archetype_row = 0;

    if (last_row == 0) {
        this->mark_dead();
    }
}

void Archetype::move_entity(const World* world, Archetype* destination, EntityId entity, EntityRecord* record) {
    const usz source_row = record->archetype_row;
    const usz source_last_row = --this->data.entity_count;
    const usz destination_row = destination->data.entity_count++;

    destination->ensure_capacity(destination_row + 1);

    if (source_row != source_last_row) {
        const EntityId swapped = this->data.entities[source_last_row];
        EntityRecord* swapped_record = world->entity_index.entity_record_get(swapped);
        swapped_record->archetype_row = source_row;
        this->data.entities[source_row] = swapped;

        for (usz i = 0; i < this->data.column_count; i++) {
            ArchetypeColumn* column = &this->data.columns[i];
            if (column->data == nullptr) continue;

            const Id id = this->archetype_type.ids[i];

            ArchetypeColumn* destination_column = destination->get_column(id);
            if (destination_column != nullptr) {
                const void* old_value = column->read(source_row);
                destination_column->write(destination_row, old_value);
            }

            const void* last_value = column->read(source_last_row);
            column->write(source_row, last_value);
        }
    } else {
        for (usz i = 0; i < this->data.column_count; i++) {
            const ArchetypeColumn* column = &this->data.columns[i];
            if (column->data == nullptr) continue;

            const Id id = this->archetype_type.ids[i];

            ArchetypeColumn* destination_column = destination->get_column(id);
            if (destination_column != nullptr) {
                const void* old_value = column->read(source_row);
                destination_column->write(destination_row, old_value);
            }
        }

        if (source_last_row == 0) {
            this->mark_dead();
        }
    }

    record->archetype = destination;
    record->archetype_row = destination_row;

    if (destination_row == 0) {
        destination->mark_alive();
    }
}

static bool append_to_pair(ComponentRecord* record, const ArchetypeId archetype_id, const usz index) {
	if (!record->columns_index.contains(archetype_id)) {
		record->columns_index.set(archetype_id, index);
		record->pair_record->pairs_count.set(archetype_id, 1);
		return true;
	} else {
		u32* count_ref = record->pair_record->pairs_count.get_ref(archetype_id);
		(*count_ref)++;
		return false;
	}
}

Archetype* Archetype::create_archetype(World* world, const ArchetypeType archetype_type) {
	Archetype* new_archetype = world->archetypes.new_element();
	const ArchetypeId archetype_id = world->archetypes.get_latest_alive();
	new_archetype->archetype_id = archetype_id;
	new_archetype->archetype_type = archetype_type.clone(world->allocator);
	new_archetype->allocator = world->allocator;
	
	const usz columns_count = archetype_type.id_count;
	
	if (columns_count > 0) {	
		EntityId* archetype_entities = MEMORY::alloc<EntityId>(world->allocator, ARCHETYPE::PREALLOCATED_SIZE, false);
		ArchetypeColumn* archetype_columns = MEMORY::alloc<ArchetypeColumn>(world->allocator, columns_count);
		
		for (usz i = 0; i < columns_count; i++) {
			const Id id = archetype_type.ids[i];
			ComponentRecord* record = ComponentRecord::component_record_ensure(world, id);
			++record->archetype_count;
		
			if (record->flags & ComponentRecordFlags::IS_COMPONENT) {
				TypeInfo* type_info = record->type_info;
				
				void* column_data = MEMORY::malloc(world->allocator, type_info->length * ARCHETYPE::PREALLOCATED_SIZE, type_info->alignment, false);
				archetype_columns[i] = {
					.data =	column_data,
					.type_info = type_info
				};
				new_archetype->columns_map.set(id, &archetype_columns[i]);
			} else {
				archetype_columns[i] = {
					.data = nullptr,
					.type_info = nullptr
				};
				new_archetype->columns_map.set(id, nullptr);
			}
			
			new_archetype->columns_index.set(id, i);
			record->columns_index.set(archetype_id, i);
			
			if (ECS::IS_PAIR(id)) {
				Id relation_wildcard = ECS::PAIR(ECS::PAIR_FIRST(id), ECS::WILDCARD);
				Id target_wildcard = ECS::PAIR(ECS::WILDCARD, ECS::PAIR_SECOND(id));
				
				auto can_relation_record =  world->component_index.get(relation_wildcard);
				assert(can_relation_record.has_value());
				ComponentRecord* relation_record = can_relation_record.value();
				++relation_record->archetype_count;
				
				const bool relation_appended = append_to_pair(relation_record, archetype_id, i);
				if (relation_appended) {
					new_archetype->columns_map.set(relation_wildcard, &archetype_columns[i]);
					new_archetype->columns_index.set(relation_wildcard, i);
				}
				
				auto can_target_record =  world->component_index.get(target_wildcard);
				assert(can_target_record.has_value());
				ComponentRecord* target_record = can_target_record.value();
				++target_record->archetype_count;		
			
				const bool target_appended = append_to_pair(target_record, archetype_id, i);
				if (target_appended) {
					new_archetype->columns_map.set(target_wildcard, &archetype_columns[i]);
					new_archetype->columns_index.set(target_wildcard, i);
				}
			}
		}
		
		new_archetype->data.entities = archetype_entities;
		new_archetype->data.columns = archetype_columns;
		
		new_archetype->data.column_count = columns_count;
		new_archetype->data.entity_count = 0;
		new_archetype->data.entity_capacity = ARCHETYPE::PREALLOCATED_SIZE;
	} else {
		// this is root archetype, no entities are saved here
		new_archetype->data.entities = nullptr;
		new_archetype->data.columns = nullptr;
		new_archetype->data.column_count = 0;
		new_archetype->data.entity_count = 0;
		new_archetype->data.entity_capacity = 0;
	}
	
	world->archetype_index.set(new_archetype->archetype_type, new_archetype);
	return new_archetype;
}

Archetype* Archetype::ensure_archetype(World* world, const ArchetypeType archetype_type) {
    auto existing_archetype = world->archetype_index.get(archetype_type);
    if (existing_archetype.has_value()) {
        return existing_archetype.value();
    }

    return Archetype::create_archetype(world, archetype_type);
}


void Archetype::destroy() {
	if (this == this->world->root_archetype) return;
	const Archetype* current_allocated = this->world->archetypes.get_element(this->archetype_id);
	if (!current_allocated) {
		LOG_FATAL("[ECS]: Attempted to destroy an archetype twice")
	}
	if (current_allocated != this) {
		LOG_FATAL("[ECS]: Cannot destroy archetype. Another archetype already exists in the same location")
	}
	if (this->data.entity_count > 0) {
		LOG_FATAL("[ECS]: Attempted to destroy an archetype that contains entities")
	}
	
	this->forward_edges.each([this](const Id id, Archetype* other) {
		other->backwards_edges.remove(id);
		
		if (ECS::IS_PAIR(id)) {
			const auto swapped_id = this->swaped_edges.get(id);
			if (swapped_id.has_value()) {
				other->swaped_edges.remove(swapped_id.value());
				other->forward_edges.remove(swapped_id.value());
			}
		}
		
	});
	this->backwards_edges.each([this](const Id id, Archetype* other) {
		other->forward_edges.remove(id);
	});
	
	this->forward_edges.free();
	this->backwards_edges.free();
	this->world->archetype_index.remove(this->archetype_type);
	
	for (usz i = 0; i < this->archetype_type.id_count; i++) {
		const Id id = this->archetype_type.ids[i];
		
		ComponentRecord* record = ComponentRecord::component_record_get(world, id);
		--record->archetype_count;
		
		if (record->archetype_count <= 0) {
			record->destroy(this->world);
		}
	}
	
	this->world->archetypes.delete_element(this->archetype_id);
}

Archetype* Archetype::traverse_add(World* world, const Id id) {
    const auto destination_edge = this->forward_edges.get(id);
    if (destination_edge.has_value()) {
        return destination_edge.value();
    }

    TemporalAllocator temp = TemporalAllocator::create();
    const ArchetypeType destination_type = this->archetype_type.insert(&temp, id);

    Archetype* destination = Archetype::ensure_archetype(world, destination_type);

    this->forward_edges.set(id, destination);
    destination->backwards_edges.set(id, this);
    return destination;
}

Archetype* Archetype::traverse_swap(World* world, const Id old_id, const Id new_id, const usz old_column) {
    const auto destination_edge = this->forward_edges.get(new_id);
    if (destination_edge.has_value()) {
        return destination_edge.value();
    }

    TemporalAllocator temp = TemporalAllocator::create();
    const ArchetypeType destination_type = this->archetype_type.clone(&temp);

    destination_type.ids[old_column] = new_id;

    Archetype* destination = Archetype::ensure_archetype(world, destination_type);
    this->forward_edges.set(new_id, destination);
	this->swaped_edges.set(new_id, old_id);
	
	destination->forward_edges.set(old_id, this);
	destination->swaped_edges.set(old_id, new_id);
	
    return destination;
}

Archetype* Archetype::traverse_remove(World* world, Id id) {
    const auto destination_edge = this->backwards_edges.get(id);
    if (destination_edge.has_value()) {
        return destination_edge.value();
    }

    TemporalAllocator temp = TemporalAllocator::create();
    const ArchetypeType destination_type = this->archetype_type.remove(&temp, id);

    Archetype* destination = Archetype::ensure_archetype(world, destination_type);

    this->backwards_edges.set(id, destination);
    destination->forward_edges.set(id, this);
    return destination;
}