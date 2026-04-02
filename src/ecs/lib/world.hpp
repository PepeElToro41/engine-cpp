#ifndef WORLD_HPP
#define WORLD_HPP

#include <unordered_map>
#include <typeindex>

#include "utils/sparse_list.hpp"
#include "entity.hpp"
#include "types.hpp"
#include "entity_index.hpp"
#include "archetype.hpp"
#include "component_record.hpp"
#include "../helpers.hpp"


namespace world {
    constexpr std::size_t MAX_COMPONENTS = 1024;

    constexpr std::size_t ECS_WILDCARD 		    = MAX_COMPONENTS + 1;
    constexpr std::size_t ECS_CHILDOF 			= MAX_COMPONENTS + 2;
    constexpr std::size_t ECS_NAME				= MAX_COMPONENTS + 3;
    constexpr std::size_t ECS_COMPONENT 		= MAX_COMPONENTS + 4;
    constexpr std::size_t ECS_FORCE_TAG 		= MAX_COMPONENTS + 5;
    constexpr std::size_t ECS_EXCLUSIVE 		= MAX_COMPONENTS + 6;
    constexpr std::size_t ECS_TRAVERSABLE 		= MAX_COMPONENTS + 7;
    constexpr std::size_t ECS_REST				= MAX_COMPONENTS + 8;
}

struct World {
    Archetype* ROOT_ARCHETYPE;
    ComponentId next_component_id;

    EntityIndex entity_index;
    SparseList<Archetype> archetypes;
    SparseList<ComponentRecord> component_records;

    std::unordered_map<std::type_index, Id> type_component_lookup;
    std::unordered_map<Id, TypeInfo> component_infos;

    std::unordered_map<Id, ComponentRecord*> component_index;
    std::unordered_map<ArchetypeType, Archetype*> archetype_index;

    ComponentId new_component(TypeInfo type_info);
    Entity new_tag(TypeInfo type_info);
    Entity new_entity();
    void make_alive(Entity entity);
    void delete_entity(Entity entity, EntityRecord* record);

    /*          TEMPLATES          */

    template <typename T>
    void entity_add(Entity entity) {
        auto id = this->type_component_lookup.find(typeid(T));
        if (id == this->type_component_lookup.end()) {
            unreachable("component not registered");
        };

        entity::entity_add_id(this, entity, id->second);
    }

    template <typename T>
    void entity_set(Entity entity, T data) {
        auto id = this->type_component_lookup.find(typeid(T));
        if (id == this->type_component_lookup.end()) {
            unreachable("component not registered");
        };

        entity::entity_set_id(this, entity, id->second, &data);
    }

    template <typename T>
    bool entity_has(Entity entity) {
        auto id = this->type_component_lookup.find(typeid(T));
        if (id == this->type_component_lookup.end()) {
            unreachable("component not registered");
        };

        return entity::entity_has_id(this, id->second, id);
    }

    template <typename T>
    T* entity_get(Entity entity) {
        auto id = this->type_component_lookup.find(typeid(T));
        if (id == this->type_component_lookup.end()) {
            unreachable("component not registered");
        };

        return (T*)entity::entity_get_id(this, id->second, id);
    }

    template <typename T>
    void entity_remove(Entity entity) {
        auto id = this->type_component_lookup.find(typeid(T));
        if (id == this->type_component_lookup.end()) {
            unreachable("component not registered");
        };

        entity::entity_remove_id(this, entity, id);
    }

    template <typename T>
    ComponentId component() {
        TypeInfo type_info = {
            .type_size = sizeof(T),
            .alignment = alignof(T),
            .name = typeid(T).name(),
        };

        ComponentId component = this->new_component(type_info);
        type_info.component = component;

        this->type_component_lookup.insert({ typeid(T), component });
        return component;
    }
    
    template <typename T>
    Entity tag() {
        TypeInfo type_info = {
            .type_size = sizeof(T),
            .alignment = alignof(T),
            .name = typeid(T).name(),
        };

        Entity tag = this->new_tag(type_info);
        type_info.component = tag;

        this->type_component_lookup.insert({ typeid(T), tag });
        return tag;
    }
};

#endif