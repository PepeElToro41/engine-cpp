#include <cstring>
#include <optional>
#include <memory>

#include "archetype.hpp"
#include "entity_index.hpp"
#include "world.hpp"
#include "../helpers.hpp"

std::optional<ArchetypeColumn*> Archetype::get_column(Id id) const {
    auto it = this->columns_map.find(id);
    if (it != this->columns_map.end()) {
        return it->second;
    } else {
        return {};
    }
}

bool Archetype::has_id(Id id) const {
    return this->columns_map.find(id) != columns_map.end();
}

void Archetype::ensure_capacity(std::size_t capacity) {
    ArchetypeData* data = &this->data;
    if (data->rows_capacity >= capacity) return;

    std::size_t new_capacity = data->rows_capacity > 0 
        ? data->rows_capacity * 2 
        : archetype::ARCHETYPE_PREALLOCATED_SIZE;

    while (new_capacity < capacity) {
        new_capacity *= 2;
    }

    new_capacity = next_power_of_two(new_capacity);
    data->entities = (Entity*)std::realloc(data->entities, new_capacity * sizeof(Entity));

    for (std::size_t i = 0; i < data->columns_count; i++) {
        ArchetypeColumn* column = &data->columns[i];
        if (column->data == nullptr) continue;

        TypeInfo type_info = column->type_info;
        column->data = std::realloc(column->data, new_capacity * type_info.type_size);
    }

    data->rows_capacity = new_capacity;
}

void Archetype::mark_alive() {
    this->alive = true;
}

void Archetype::mark_dead() {
    this->alive = false;
}

bool append_to_pair(ComponentRecord* record, ArchetypeId archetype_id, std::size_t index) {
    auto column_index = record->columns_index.find(archetype_id);
    
    if (column_index == record->columns_index.end()) {
        record->pair_record.pairs_count[archetype_id] = 1;
        return true;
    } else {
        record->pair_record.pairs_count[archetype_id] += 1;
        return false;
    }
}

Archetype* archetype::create_archetype(World* world, ArchetypeType archetype_type) {
    SparseList<Archetype>* archetypes_list = &world->archetypes;

    Archetype archetype = {
        .world = world,
        .archetype_type = archetype_type,
        .alive = false,
    };

    // we allocate the archetype because we need the archetype id,
    // but we still edit the archetype in the stac
    // we replace it again at the end
    Archetype* allocated_slot = archetypes_list->new_element({});
    ArchetypeId archetype_id = archetypes_list->get_latest_alive();

    archetype.archetype_id = archetype_id;

    std::vector<Id> components = archetype_type.ids;
    std::size_t components_count = components.size();

    if (components_count > 0) {
        Entity* entities = new Entity[ARCHETYPE_PREALLOCATED_SIZE];
        ArchetypeColumn* columns = new ArchetypeColumn[components_count];

        for (std::size_t index = 0; index < components_count; index++) {
            Id component = components[index];
            ComponentRecord* record = component_record::component_record_ensure(world, component);
            record->archetype_count++;

            record->columns_index.insert({ archetype_id, index });
            archetype.columns_index.insert({ component, index });

            // allocating columns
            if (record->flags & component_record::FLAG_IS_COMPONENT) {
                TypeInfo type_info = record->type_info;
                ArrPtr<void> column_data = std::malloc(type_info.type_size * ARCHETYPE_PREALLOCATED_SIZE);

                columns[index] = {
                    .data = column_data,
                    .type_info = type_info,
                };
                archetype.columns_map.insert({ component, &columns[index] });
            } else {
                archetype.columns_map.insert({ component, nullptr });
            }

            // creating matchers
            if (component > archetype::BITSET_MATCH_SIZE) {
                archetype.bloom_filter.add(component);
            } else {
                archetype.bitmask.set(component - 1);
            }

            // mapping wildcard ids
            if (entity::is_pair(component)) {
                EntityLow relation = entity::pair_first(component);
                EntityLow target = entity::pair_second(component);

                Id relation_wildcard = entity::ecs_pair(relation, world::ECS_WILDCARD);
                Id target_wildcard = entity::ecs_pair(world::ECS_WILDCARD, target);

                ComponentRecord* relation_record = world->component_index.at(relation_wildcard);
                ComponentRecord* target_record = world->component_index.at(target_wildcard);

                bool relation_appended = append_to_pair(relation_record, archetype_id, index);
                bool target_appended = append_to_pair(target_record, archetype_id, index);

                if (relation_appended) {
                    archetype.columns_map.insert({ relation_wildcard, &columns[index] });
                    archetype.columns_index.insert({ relation_wildcard, index });
                }

                if (target_appended) {
                    archetype.columns_map.insert({ target_wildcard, &columns[index] });
                    archetype.columns_index.insert({ target_wildcard, index });
                }
            }
        }

        ArchetypeData data = {
			.entities = entities,
			.columns = columns,
			.columns_count = components_count,
			.rows_count = 0,
			.rows_capacity = ARCHETYPE_PREALLOCATED_SIZE,
		};

        archetype.data = data;
    } else {
        // root archetype, we dont save entities here
        ArchetypeData data = {
            .entities = nullptr,
            .columns = nullptr,
            .columns_count = 0,
            .rows_count = 0,
            .rows_capacity = 0,
        };

        archetype.data = data;
    }

    // we replace the allocated archetype with the one we have in the stack
    allocated_slot[0] = archetype;
    world->archetype_index.insert({ archetype_type, allocated_slot });
    return allocated_slot;
}

Archetype* archetype::ensure_archetype(World* world, ArchetypeType archetype_type) {
    auto existing_archetype = world->archetype_index.find(archetype_type);
    if (existing_archetype != world->archetype_index.end()) {
        return existing_archetype->second;
    }

    return archetype::create_archetype(world, archetype_type);
}

Archetype* archetype::traverse_add(World* world, Archetype* source, Id id) {
    auto destination_edge = source->forward_edges.find(id);
    if (destination_edge != source->forward_edges.end()) {
        return destination_edge->second;
    }
    
    Archetype* destination;
    std::allocator<Id> alloc;
    
    // TODO: use a temporal arena
    ArchetypeType destination_type = source->archetype_type.clone(alloc, 1);
    destination_type.ids.push_back(id);
    
    destination = archetype::ensure_archetype(world, destination_type);
    
    source->forward_edges.insert({ id, destination });
    destination->backwards_edges.insert({ id, source });
    
    return destination;
}

Archetype* archetype::traverse_remove(World* world, Archetype* source, Id id) {
    auto destination_edge = source->backwards_edges.find(id);
    if (destination_edge != source->backwards_edges.end()) {
        return destination_edge->second;
    }
    
    Archetype* destination;
    std::allocator<Id> alloc;
    
    // TODO: use a temporal arena
    auto destination_ids = std::vector<Id>();
    destination_ids.reserve(source->archetype_type.ids.size() - 1);

    for (Id filter_id : source->archetype_type.ids) {
        if (filter_id != id) {
            destination_ids.push_back(filter_id);
        }
    }

    ArchetypeType destination_type = {
        .ids = destination_ids,
    };
    
    destination = archetype::ensure_archetype(world, destination_type);
    
    destination->forward_edges.insert({ id, source });
    source->backwards_edges.insert({ id, destination });
    
    return destination;
} 

void archetype::insert_entity(World* world, Archetype* archetype, Entity entity, EntityRecord* record) { 
    std::size_t destination_row = archetype->data.columns_count;
    archetype->ensure_capacity(destination_row);

    archetype->data.entities[destination_row] = entity; 
    archetype->data.rows_count++;
    
    record->archetype = archetype;
    record->archetype_row = destination_row;
    
    if (destination_row == 0) {
        archetype->mark_alive();
    }
}

void archetype::delete_entity(World* world, Archetype* archetype, Entity entity, EntityRecord* record) {
    std::size_t row = record->archetype_row;
    std::size_t last_row = archetype->data.rows_count - 1;

    Entity swapped = archetype->data.entities[last_row];
    std::vector<ComponentId> components = archetype->archetype_type.ids;

    if (row != last_row) {
        EntityRecord* swapped_record = world->entity_index.entity_record_get(swapped).value();
        swapped_record->archetype_row = row;
        archetype->data.entities[row] = swapped;

        for (std::size_t i = 0; i < components.size(); i++) {
            ArchetypeColumn* column = &archetype->data.columns[i];
            if(column->data == nullptr) continue;

            void* new_slot = archetype::read_column(column, row);
            void* old_slot = archetype::read_column(column, last_row);

            std::memcpy(new_slot, old_slot, column->type_info.type_size);
        }
    } else {
    }
    
    archetype->data.rows_count--;
    record->archetype = world->ROOT_ARCHETYPE;
    record->archetype_row = 0;
    
    if (last_row == 0) {
        archetype->mark_dead();
    }
}

void archetype::move_entity(World* world, Archetype* source, Archetype* destination, Entity entity, EntityRecord* record) {
    std::size_t source_row = record->archetype_row;
    std::size_t source_last_row = source->data.rows_count - 1;  
    std::size_t destination_row = destination->data.rows_count;

    destination->ensure_capacity(destination_row);

    if (source_row != source_last_row) {
        // swapping rows
        Entity swapped = source->data.entities[source_last_row];
        source->data.entities[source_row] = swapped;

        EntityRecord* swapped_record = world->entity_index.entity_record_get(swapped).value();
        swapped_record->archetype_row = source_row;
        
        auto ids = source->archetype_type.ids;
        for (std::size_t i = 0; i < ids.size(); i++) {
            Id id = ids[i];
            ArchetypeColumn* source_column = &source->data.columns[i];
            if (source_column->data == nullptr) continue;

            std::size_t type_size = source_column->type_info.type_size;
            char* source_data = (char*)archetype::read_column(source_column, source_row);

            std::optional<ArchetypeColumn*> destination_column = destination->get_column(id);

            if (destination_column.has_value()) {
                if (destination_column.value()->data == nullptr) continue;

                char* destination_data = (char*)archetype::read_column(destination_column.value(), destination_row);
                std::memcpy(destination_data, source_data, type_size);
            }

            char* swapped_data = (char*)archetype::read_column(source_column, source_last_row);
            std::memcpy(source_data, swapped_data, type_size);
        }
    } else {
        // last column row, copy to destination columns;
        auto ids = source->archetype_type.ids;

        for (std::size_t i = 0; i < ids.size(); i++) {
            Id id = ids[i];
            
            ArchetypeColumn* source_column = &source->data.columns[i];
            if (source_column->data == nullptr) continue;

            std::optional<ArchetypeColumn*> destination_column = destination->get_column(id);

            if (destination_column.has_value()) {
                char* source_data = (char*)archetype::read_column(source_column, source_row);
                char* destination_data = (char*)archetype::read_column(destination_column.value(), destination_row);
                
                std::memcpy(destination_data, source_data, source_column->type_info.type_size);
            }
        }

    }
    
    destination->data.rows_count++;
    source->data.rows_count--;
    
    record->archetype = destination;
    record->archetype_row = destination_row;

    if (source_row == 0) {
        source->mark_dead();
    }
    if (destination_row == 0) {
        destination->mark_alive();
    }
}

void archetype::write_column(ArchetypeColumn* column, std::size_t row, const void* data) {
    std::size_t type_size = column->type_info.type_size;
    char* column_data = (char*)column->data;

    std::memcpy(&column_data[row * type_size], data, type_size);
}

void* archetype::read_column(const ArchetypeColumn* column, std::size_t row) {
    std::size_t type_size = column->type_info.type_size;
    char* column_data = (char*)column->data;

    return &column_data[row * type_size];
}