#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>

#include "../helpers.hpp"
#include "entity_index.hpp"
#include "entity.hpp"


EntityPage alloc_page() {
    EntityRecord* records = new EntityRecord[entity_index::ENTITY_PAGE_SIZE];
    
    EntityPage page = {
        .records = records,
    };
    return page;
}

EntityPage get_empty_page() {
    EntityPage page = {
        .records = nullptr,
    };

    return page;
}


bool EntityPage::is_empty() const {
    return this->records == nullptr;
}

std::optional<EntityPage> EntityIndex::get_valid_page(std::size_t page_index) const {
    if (page_index < this->sparse_pages.size()) {
        EntityPage page = this->sparse_pages[page_index];
        if (page.is_empty()) {
            return {};
        }
        return page;
    }
    return {};
}

EntityPage EntityIndex::ensure_page(std::size_t page_index) {
    if (page_index < this->sparse_pages.size()) {
        EntityPage page = this->sparse_pages[page_index];

        if (page.is_empty()) {
            EntityPage new_page = alloc_page();
            this->sparse_pages[page_index] = new_page;
            return new_page;
        }

        return page;
    }

    std::size_t missing_pages = page_index - this->sparse_pages.size();

    for (std::size_t i = 0; i < missing_pages; i++) {
        // intermediate pages are lazily allocated
        EntityPage empty_page = get_empty_page();
        this->sparse_pages.push_back(empty_page);
    }

    EntityPage page = alloc_page();
    this->sparse_pages.push_back(page);

    return page;
}

std::optional<EntityRecord*> EntityIndex::entity_record_get(Entity entity) {
    EntityLow entity_id = entity_index::entity_id_low(entity);
    std::size_t page_index = entity_index::get_page_index(entity_id);
    std::size_t page_offset = entity_index::get_page_offset(entity_id);

    std::optional<EntityPage> page = this->get_valid_page(page_index);
    if (!page.has_value()) {
        return {};
    }

    return &page.value().records[page_offset];
}

std::optional<EntityRecord*> EntityIndex::entity_record_get_alive(Entity entity) {
    std::optional<EntityRecord*> record = this->entity_record_get(entity);
    if (!record.has_value()) {
        return {};
    }

    Entity alive = this->dense_list[record.value()->dense];
    if (alive != entity) {
        // generation missmatch
        return {};
    }

    return record.value();
}

EntityRecord* EntityIndex::entity_record_ensure(Entity entity) {
    EntityLow entity_id = entity_index::entity_id_low(entity);
    std::size_t page_index = entity_index::get_page_index(entity_id);
    std::size_t page_offset = entity_index::get_page_offset(entity_id);

    EntityPage page = this->ensure_page(page_index);
    return &page.records[page_offset];
}

void EntityIndex::dense_swap_recycle(Entity entity, EntityRecord* record) {
    std::size_t dense = record->dense;
    std::size_t alive_count = this->alive_count;

    Entity last_alive = this->dense_list[alive_count];
    
    if (entity != last_alive) {
        this->dense_list[dense] = last_alive;
        this->dense_list[alive_count] = entity_index::increment_generation(entity);

        record->dense = alive_count;
        auto swapped_record = this->entity_record_get(last_alive);
        if (!swapped_record.has_value()) {
            unreachable("cannot get entity record");
        }
        swapped_record.value()->dense = dense;
    }

    this->alive_count--;
}

void EntityIndex::dense_swap_delete(Entity entity, EntityRecord* record) {
    std::size_t dense = record->dense;
    Entity last_alive = this->dense_list[this->alive_count];
    Entity last = this->dense_list.back();
    this->dense_list.pop_back();

    if (entity != last_alive) {
        this->dense_list[dense] = last_alive;

        auto swapped_record = this->entity_record_get(last_alive);
        if (!swapped_record.has_value()) {
            unreachable("cannot get entity record");
        }
        swapped_record.value()->dense = dense;
    }

    if (last_alive != last) {
        this->dense_list[this->alive_count] = last;
        auto last_record = this->entity_record_get(last);
        if (!last_record.has_value()) {
            unreachable("cannot get entity record");
        }
        last_record.value()->dense = this->alive_count;
    }

    record->dense = 0;
    this->alive_count--;
}


Entity EntityIndex::new_entity() {
    if (this->alive_count < this->dense_list.size() - 1) {
        // recycling dead entity
        this->alive_count++;
        return this->dense_list[this->alive_count];
    }

    // creating new entity
    Entity next_entity = this->next_entity++;
    this->dense_list.push_back(next_entity);

    if (this->entity_range.has_value()) {
        bool in_range = this->entity_range.value().contains(next_entity);

        if (!in_range) {
            unreachable("new entity is outside of range");
        }
    }

    this->alive_count++;
    std::size_t dense = this->alive_count;

    EntityRecord new_record;
    new_record.archetype = nullptr;
    new_record.archetype_row = 0;
    new_record.dense = dense;

    std::size_t page_index = entity_index::get_page_index(next_entity);
    std::size_t page_offset = entity_index::get_page_offset(next_entity);

    EntityPage page = this->ensure_page(page_index);
    page.records[page_offset] = new_record;

    return next_entity;
}

void EntityIndex::make_alive(Entity entity) {
    EntityRecord* record = this->entity_record_ensure(entity);
    std::size_t dense = record->dense;
    
    if (dense != 0) {
        if (dense <= this->alive_count) {
            // entity is already alive
            // overwriting its generation
            this->dense_list[dense] = entity;
            return;
        }
    
        this->alive_count++;
        std::size_t next_alive_dense = this->alive_count;
        if (dense == next_alive_dense) {
            // entity is the next one to be alive
            // just by incrementing alive_count will make it alive
            // overwriting its generation
            this->dense_list[dense] = entity;
            return;
        }
    
        // swapping with the first dead entity
        Entity first_dead = this->dense_list[next_alive_dense];
    
        this->dense_list[dense] = first_dead;
        this->dense_list[next_alive_dense] = entity;
    
        auto dead_record = this->entity_record_get(first_dead);
        if (!dead_record.has_value()) {
            unreachable("cannot get entity record");
        }
        dead_record.value()->dense = dense;
        record->dense = next_alive_dense;
    } else {
        if (this->alive_count == this->dense_list.size() - 1) {
            // no more dead entities, pushing the new entity
            this->dense_list.push_back(entity);
            this->alive_count++;
            std::size_t next_alive_dense = this->alive_count;
            record->dense = next_alive_dense;
        } else {
            // moving first dead entity at the end
            // and replacing it with the new entity
            this->alive_count++;
            std::size_t next_alive_dense = this->alive_count;
    
            Entity first_dead = this->dense_list[next_alive_dense];
    
            this->dense_list.push_back(first_dead);
            this->dense_list[next_alive_dense] = entity;
    
            auto first_dead_record = this->entity_record_get(first_dead);
            if (!first_dead_record.has_value()) {
                unreachable("cannot get entity record");
            }
    
            first_dead_record.value()->dense = this->dense_list.size() - 1;
            record->dense = next_alive_dense;
        }
    }
}

void EntityIndex::delete_entity(Entity entity, EntityRecord* record) {
    EntityLow entity_id = entity_index::entity_id_low(entity);
    if (record->dense == 0) {
        return;
    }

    if (this->entity_range.has_value() && !this->entity_range.value().contains(entity)) {
        // entity is not in range, do not recycle
        this->dense_swap_delete(entity, record);
    } else {
        this->dense_swap_recycle(entity, record);
    }
}

void EntityIndex::set_range(Range range) {
    this->entity_range = range;
}

std::optional<Entity> EntityIndex::get_alive(EntityLow entity) {
    auto record = this->entity_record_get(entity);
    if (!record.has_value()) {
        return {};
    }
    if (record.value()->dense == 0) {
        return {};
    }

    return this->dense_list[record.value()->dense];
}