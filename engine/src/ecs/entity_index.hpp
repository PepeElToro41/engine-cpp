#pragma once

#include <stdlib.h>

#include "templates/dynamic_array.hpp"
#include "core/memory/memory.hpp"
#include "defines.hpp"
#include "ecs/ecs.hpp"

struct Archetype;

namespace ENTITY_INDEX {
    constexpr usz PAGE_SIZE_BITS = 10; // 1024 pages
    constexpr usz PAGE_SIZE = 1ull << PAGE_SIZE_BITS;
    constexpr usz PAGE_MASK = PAGE_SIZE - 1;

    inline EntityGeneration entity_generation(const EntityId id) {
        return id >> ECS::ENTITY_BITS;
    }
    inline EntityId append_generation(const EntityIdLow id, const EntityGeneration generation) {
        return id | (generation << ECS::ENTITY_BITS);
    }
    inline usz get_page_index(const EntityId id) {
        return id >> PAGE_SIZE_BITS;
    }
    inline usz get_page_offset(const EntityId id) {
        return id & PAGE_MASK;
    }

    inline EntityId increment_generation(const EntityId entity) {
        if (entity > ECS::ENTITY_SIZE) {
            const EntityIdLow id = ECS::ENTITY_ID(entity);
            const EntityGeneration new_generation = entity_generation(entity) + 1;

            if (new_generation <= ECS::GENERATION_SIZE) {
                return append_generation(id, new_generation);
            }
            return append_generation(id, 1);
        }
        return append_generation(entity, 1);
    }
}

struct EntityRecord {
    usz dense;
    usz archetype_row;
    Archetype* archetype;
};


struct EntityIndexPage {
    EntityRecord* records = nullptr;

	inline bool empty() const {
		return this->records == nullptr;
	}
    void allocate(BaseAllocator* allocator);
    void free(BaseAllocator* allocator);
};

struct EntityIndex {
    BaseAllocator* allocator;
    DynamicArray<EntityIndexPage> sparse_pages;
    DynamicArray<u64> dense_list;

    u64 alive_count = 0;
    u64 next_entity = 0;

    explicit EntityIndex(BaseAllocator* allocator) :
        allocator(allocator),
        sparse_pages(allocator),
        dense_list(allocator)
    {};

	inline EntityIndexPage* get_valid_page(const u64 page_index) const {
		if (page_index >= this->sparse_pages.size) return nullptr;
		
		EntityIndexPage* page = this->sparse_pages.get_ref(page_index);
		if (page->empty()) return nullptr;
		
		return page;
	}
    EntityIndexPage* ensure_page(const u64 page_index);

    EntityId new_entity(EntityRecord** out_record);
    void delete_entity(const EntityId entity, EntityRecord* record);
	inline EntityId entity_get_alive(const EntityIdLow entity_low) const {
		const usz page_index = ENTITY_INDEX::get_page_index(entity_low);
		const usz page_offset = ENTITY_INDEX::get_page_offset(entity_low);

		const EntityIndexPage* page = this->get_valid_page(page_index);
		if (!page) return 0;
		
		const usz dense = page->records[page_offset].dense;
		if (dense >= this->alive_count) return 0;
		
		return this->dense_list.get(dense);
	};
	inline EntityId entity_get_any(const EntityIdLow entity_low) const {
		const usz page_index = ENTITY_INDEX::get_page_index(entity_low);
		const usz page_offset = ENTITY_INDEX::get_page_offset(entity_low);

		const EntityIndexPage* page = this->get_valid_page(page_index);
		if (!page) return 0;
		
		const usz dense = page->records[page_offset].dense;
		return this->dense_list.get(dense);
	};
    
    EntityRecord* entity_record_get(const EntityId entity) const;
    EntityRecord* entity_record_get_alive(const EntityId entity) const;
    EntityRecord* entity_record_ensure(const EntityId entity);
	

    void free();

private: 
    void dense_swap_recycle(EntityId entity, EntityRecord* record);
};