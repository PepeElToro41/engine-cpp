#include "component_record.hpp"
#include "world.hpp"
#include "types.hpp"

// 

TypeInfo* get_component_type_info(World* world, Id relation, Id target) {
    if (target && entity::entity_has_id(world, relation, world::ECS_FORCE_TAG)) {
	    return nullptr;
	}

    bool is_component = entity::entity_has_id(world, relation, world::ECS_COMPONENT);
	
    if(is_component) {
        auto info = world->component_infos.find(relation);
        if (info != world->component_infos.end()) {
            return &world->component_infos.at(relation);
        }
        return nullptr;
    } else if (target) {
        bool target_is_component = entity::entity_has_id(world, target, world::ECS_COMPONENT);

        if (target_is_component) {
            auto info = world->component_infos.find(target);
            if (info != world->component_infos.end()) {
                return &world->component_infos.at(target);
            }
        }
        return nullptr;
    }

	return nullptr;
}

ComponentRecord* component_record::component_record_create(World* world, Id id) {
    bool is_pair = entity::is_pair(id);
    bool is_exclusive = false;
    bool is_traversable = false;
    bool has_delete = false;

    Entity relation = id;
    Entity target = 0;

    ComponentRecord record = {
        .archetype_count = 0,
        .flags = 0,
    };

    if (is_pair) {
        EntityLow relation_id = entity::pair_first(id);
        EntityLow target_id = entity::pair_second(id);

        relation = world->entity_index.get_alive(relation_id).value();
        target = world->entity_index.get_alive(target_id).value();

        is_exclusive = entity::entity_has_id(world, relation, world::ECS_EXCLUSIVE);
        is_traversable = entity::entity_has_id(world, relation, world::ECS_TRAVERSABLE);
    }

    TypeInfo* type_info = get_component_type_info(world, relation, target);
    bool is_component = type_info != nullptr;

    record.type_info = *type_info;
    record.flags = 0
        | (is_pair ? component_record::FLAG_IS_PAIR : 0)
		| (is_exclusive ? component_record::FLAG_IS_EXCLUSIVE : 0)
		| (has_delete ? component_record::FLAG_HAS_DELETE : 0)
		| (is_component ? component_record::FLAG_IS_COMPONENT : 0)
        | (is_traversable ? component_record::FLAG_IS_TRAVERSABLE : 0);

    SparseList<ComponentRecord>* component_records_list = &world->component_records;

    // we need to allocate the record because we need the pointer to map to other records
    ComponentRecord* allocated_record = component_records_list->new_element(record);
    allocated_record->sparse_id = component_records_list->get_latest_alive();

    if (is_pair) {
        Pair relation_wildcard = entity::ecs_pair(relation, world::ECS_WILDCARD);
        Pair target_wildcard = entity::ecs_pair(world::ECS_WILDCARD, target);

        if (relation_wildcard == id) {
            allocated_record->flags |= component_record::FLAG_IS_REL_WILDCARD;
        } else if (target_wildcard == id) {
            allocated_record->flags |= component_record::FLAG_IS_TGT_WILDCARD;
        } else {
            ComponentRecord* relation_record = component_record::component_record_ensure(world, relation_wildcard);
            ComponentRecord* target_record = component_record::component_record_ensure(world, target_wildcard);

            relation_record->pair_record.first_records.insert({ id, allocated_record });
            target_record->pair_record.second_records.insert({ id, allocated_record });

            if (relation_record->flags & component_record::FLAG_IS_TRAVERSABLE) {
                target_record->pair_record.trav_records.insert({ id, allocated_record });
            }
        }
    }

    return allocated_record;
}


ComponentRecord* component_record::component_record_ensure(World* world, Id id) {
    auto record = world->component_index.find(id);
    if (record != world->component_index.end()) {
        return record->second; 
    }
    return component_record::component_record_create(world, id);
}
