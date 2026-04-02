#ifndef ENTITY_INDEX_HPP
#define ENTITY_INDEX_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <optional>

#include "entity.hpp"
#include "types.hpp"


struct EntityRecord;

namespace entity_index {
    constexpr std::uint64_t ENTITY_BITS = 32;
    constexpr std::uint64_t ENTITY_SIZE = 1ull << ENTITY_BITS;
    constexpr std::uint64_t ENTITY_MASK = ENTITY_SIZE - 1;

    constexpr std::uint64_t GENERATION_BITS = 10;
    constexpr std::uint64_t GENERATION_SIZE = 1ull << GENERATION_BITS;
    constexpr std::uint64_t GENERATION_MASK = GENERATION_SIZE - 1;

    constexpr std::uint64_t ENTITY_PAGE_BITS = 10;
    constexpr std::uint64_t ENTITY_PAGE_SIZE = 1ull << ENTITY_PAGE_BITS;
    constexpr std::uint64_t ENTITY_PAGE_MASK = ENTITY_PAGE_SIZE - 1;


    inline EntityLow entity_id_low(Entity entity) {
        return entity & ENTITY_MASK;
    };
    inline std::size_t sparse_generation(Entity entity) {
        return entity >> ENTITY_BITS;
    };
    inline Entity append_generation(EntityLow entity, std::size_t generation) {
        return entity | (generation << ENTITY_BITS);
    };
    inline std::size_t get_page_index(Entity entity) {
        return entity >> ENTITY_PAGE_BITS;
    }
    inline std::size_t get_page_offset(Entity entity) {
        return entity & ENTITY_PAGE_MASK;
    };

    Entity increment_generation(Entity entity) {
        if (entity > ENTITY_MASK) {
            EntityLow entity_low = entity_id_low(entity);
            std::size_t new_generation = sparse_generation(entity) + 1;
            if (new_generation >= GENERATION_SIZE) {
                return append_generation(entity_low, 1);
            }

            return append_generation(entity_low, new_generation);
        }
        return append_generation(entity, 1);
    };
}


struct EntityPage {
    EntityRecord records[entity_index::ENTITY_PAGE_SIZE];

    bool is_empty() const;
};

struct EntityIndex {
    std::vector<EntityPage> sparse_pages;
    std::vector<Entity> dense_list;

    std::optional<Range> entity_range = {};

    std::size_t alive_count = 0;
    std::size_t next_entity = 1;

    EntityIndex() {
        // sparse arrays will point to zero when the id doesn't exist
	    // this means that dense array is one-indexed
        this->dense_list.push_back(0);
    };


    Entity new_entity();
    void make_alive(Entity entity);
    void delete_entity(Entity entity, EntityRecord* record);

    void set_range(Range range);
    std::optional<Entity> get_alive(EntityLow entity);

    std::optional<EntityRecord*> entity_record_get_alive(Entity entity);
    std::optional<EntityRecord*> entity_record_get(Entity entity);

    EntityRecord* entity_record_ensure(Entity entity);

private:
    void dense_swap_recycle(Entity entity, EntityRecord* record);
    void dense_swap_delete(Entity entity, EntityRecord* record);

    EntityPage ensure_page(std::size_t page_index);
    std::optional<EntityPage> get_valid_page(std::size_t page_index) const;
};


#endif