#include "templates/dynamic_array.hpp"
#include "core/memory/memory.hpp"
#include "core/logging.hpp"
#include "defines.hpp"
#include "platform/platform.hpp"
#include "ecs/entity_index.hpp"

void EntityIndexPage::allocate(BaseAllocator* allocator) {
    this->records = MEMORY::alloc<EntityRecord>(allocator, ENTITY_INDEX::PAGE_SIZE);
}

void EntityIndexPage::free(BaseAllocator* allocator) {
    MEMORY::free(allocator, this->records);
	this->records = nullptr;
}

EntityIndexPage* EntityIndex::ensure_page(const usz page_index) {
    if (page_index < this->sparse_pages.size) {
        return this->sparse_pages.get_ref(page_index); 
    }

    const usz missing_pages = (page_index - this->sparse_pages.size) + 1;
    for (usz i = 0; i < missing_pages; i++) {
        EntityIndexPage empty_page{};
        empty_page.allocate(this->allocator);
        this->sparse_pages.push(empty_page);
    }

    return this->sparse_pages.get_ref(page_index);
}

EntityId EntityIndex::new_entity(EntityRecord** out_record) {
    if (this->alive_count < this->dense_list.size - 1) {
        // recycling dead entity
        const usz dense = this->alive_count++;

        const EntityId entity = this->dense_list.get(dense);
        const EntityIdLow entity_id = ECS::ENTITY_ID(entity);

        const usz page_index = ENTITY_INDEX::get_page_index(entity_id);
        const usz page_offset = ENTITY_INDEX::get_page_offset(entity_id);

        const EntityIndexPage* page = this->ensure_page(page_index);

    	PLATFORM::memzero(page->records + page_offset, sizeof(EntityRecord));
        page->records[page_offset].dense = dense;
		out_record[0] = &page->records[page_offset];
        return entity;
    }

    const EntityId next_id = this->next_entity++;
    this->dense_list.push(next_id);

    const usz dense = this->alive_count++;
    const usz page_index = ENTITY_INDEX::get_page_index(next_id);
    const usz page_offset =  ENTITY_INDEX::get_page_offset(next_id);

    const EntityIndexPage* page = this->ensure_page(page_index);
	
	PLATFORM::memzero(page->records + page_offset, sizeof(EntityRecord));
    page->records[page_offset].dense = dense;
	out_record[0] = &page->records[page_offset];
    return next_id;
}

void EntityIndex::delete_entity(const EntityId entity, EntityRecord* record) {
    if (record->dense == 0) {
        return;
    }

    this->dense_swap_recycle(entity, record);
}

EntityRecord* EntityIndex::entity_record_get(const EntityId entity) const {
    const EntityIdLow entity_low = ECS::ENTITY_ID(entity);
    const usz page_index = ENTITY_INDEX::get_page_index(entity_low);
    const usz page_offset = ENTITY_INDEX::get_page_offset(entity_low);

    const EntityIndexPage* page = this->get_valid_page(page_index);
	if (!page) return nullptr;
	return &page->records[page_offset];
}

EntityRecord* EntityIndex::entity_record_get_alive(const EntityId entity) const {
    EntityRecord* record = this->entity_record_get(entity);
    if (!record) {
        LOG_FATAL("[ECS]: Entity record not found")
        std::abort();
        return nullptr;
    }
    const EntityId alive = this->dense_list.get(record->dense);
    if (alive != entity) {
        LOG_FATAL("[ECS]: Entity record has a different generation")
        std::abort();
        return nullptr;
    }

    return record;
}

EntityRecord* EntityIndex::entity_record_ensure(const EntityId entity) {
    const EntityIdLow entity_low = ECS::ENTITY_ID(entity);
    const usz page_index = ENTITY_INDEX::get_page_index(entity_low);
    const usz page_offset = ENTITY_INDEX::get_page_offset(entity_low);

    const EntityIndexPage* page = this->ensure_page(page_index);
    return &page->records[page_offset];
}

void EntityIndex::free() {
    for (EntityIndexPage page : this->sparse_pages) {
        page.free(this->allocator);
    }
    this->sparse_pages.free();
    this->dense_list.free();
}

void EntityIndex::dense_swap_recycle(const EntityId entity, EntityRecord* record) {
    const usz dense = record->dense;
    const usz latest_alive_dense = --this->alive_count;
    
    if(dense != latest_alive_dense) {
        const EntityId latest_alive = this->dense_list.get(latest_alive_dense);
        this->dense_list.set(dense, latest_alive);
        this->dense_list.set(latest_alive_dense, ENTITY_INDEX::increment_generation(entity));
    
        EntityRecord* latest_record = this->entity_record_get(latest_alive);
    
        latest_record->dense = dense;
        record->dense = latest_alive_dense;
    } else {
        this->dense_list.set(dense, ENTITY_INDEX::increment_generation(entity));
    }
}

