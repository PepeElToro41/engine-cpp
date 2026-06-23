#pragma once

#include "defines.hpp"
#include "platform/platform.hpp"
#include "ecs/ecs.hpp"
#include "ecs/ecs_types.hpp"
#include "templates/hashmap.hpp"
#include "templates/dynamic_array.hpp"
#include "core/memory/memory.hpp"
#include "ecs/utils/bloom_filter.hpp"

struct World;
struct EntityRecord;


namespace ARCHETYPE {
	constexpr usz BITSET_MATCH_SIZE = 512;
	constexpr usz PREALLOCATED_SIZE = 16;
}

using ArchetypeBitset = std::bitset<ARCHETYPE::BITSET_MATCH_SIZE>;

struct ArchetypeType {
    // this array of ids must be sorted
    Id* ids = nullptr;
    usz id_count = 0;

	ArchetypeType() = default;
	ArchetypeType(Id* ids_array, const usz count) : 
		ids(ids_array), 
		id_count(count) 
	{}
	
    ArchetypeType clone(
        BaseAllocator* allocator,
        usz reserve = 0
    ) const;
    ArchetypeType insert(BaseAllocator* allocator, Id id) const;
    ArchetypeType remove(BaseAllocator* allocator, Id id) const;
    void free(BaseAllocator* allocator);
	
	inline u64 hash() const {
		u64 hashed_number = 2166136261u;

		for (usz i = 0; i < this->id_count; i++) {
			hashed_number ^= this->ids[i];
			hashed_number *= 16777619u;
		}
		return hashed_number;
	}
	inline bool equals(const ArchetypeType& other) const {
		if (this->id_count != other.id_count) {
			return false;
		}

		for (usz i = 0; i < this->id_count; i++) {
			if (this->ids[i] != other.ids[i]) {
				return false;
			}
		}

		return true;
	}
    inline bool operator==(const ArchetypeType& other) const {
        return this->equals(other);
    }
};

template<>
struct std::hash<ArchetypeType> {
	usz operator()(const ArchetypeType& type) const noexcept {
		return type.hash();
	}
};

struct ArchetypeColumn {
    void* data;
    TypeInfo* type_info;

    // ReSharper disable once CppMemberFunctionMayBeConst
    inline void write(const usz row, const void* set_data) {
        const usz length = this->type_info->length;
        char* column_data = (char*)this->data;
        PLATFORM::memcpy(&column_data[row * length], set_data, length);
    }
    inline void* read(const usz row) const {
        const usz length = this->type_info->length;
        char* column_data = (char*)this->data;
        return &column_data[row * length];
    }
};

struct ArchetypeData {
    EntityId* entities;
    ArchetypeColumn* columns;

    usz column_count;
    usz entity_count;
    usz entity_capacity;
};

struct ArchetypeMatcher {
	ArchetypeBitset with_id_bitmask;
	ArchetypeBitset without_id_bitmask;
	
	BloomFilter with_bloom;
	DynamicArray<Id> with_ids;
	DynamicArray<Id> without_ids;
	
	u8 with_outer_bitmask;
	u8 without_outer_bitmask;
};

struct ArchetypeObserver {
	ArchetypeMatcher matcher;
};

struct Archetype {
    World* world;
    BaseAllocator* allocator;
    ArchetypeId archetype_id;
    ArchetypeType archetype_type;

    HashMap<Id, usz> columns_index;
    HashMap<Id, ArchetypeColumn*> columns_map;

    bool alive;
    ArchetypeData data;
	
	BloomFilter bloom_filter;
	ArchetypeBitset id_bitmask;
	u8 outer_bitmask;

    HashMap<Id, Archetype*> forward_edges;
    HashMap<Id, Archetype*> backwards_edges;
	HashMap<Id, Id> swaped_edges;

	inline ArchetypeColumn* get_column(const Id id) const {
		const auto column = this->columns_map.get(id);
		if (column.has_value()) {
			return column.value();
		}
		return nullptr;
	}
    
	inline bool has_id(const Id id) const {
		return this->columns_map.contains(id);
	}
    void mark_alive();
    void mark_dead();
    void ensure_capacity(usz capacity);
    
    void insert_entity(World* world, EntityId entity, EntityRecord* record);
    void delete_entity(const World* world, EntityId entity, EntityRecord* record);
    void move_entity(const World* world, Archetype* destination, EntityId entity, EntityRecord* record);
    
    Archetype* traverse_add(World* world, Id id);
    Archetype* traverse_swap(World* world, Id old_id, Id new_id, usz column);
    Archetype* traverse_remove(World* world, Id id);
	
	inline bool match(const ArchetypeMatcher& matcher) const {
		const u16 outer_bitmask = this->outer_bitmask;
		const ArchetypeBitset id_bitmask = this->id_bitmask;
		const BloomFilter bloom_filter = this->bloom_filter;

		if (!bloom_filter.test(matcher.with_bloom)) {
			return false;
		}

		if (matcher.with_outer_bitmask != 0) {
			if ((matcher.with_outer_bitmask & outer_bitmask) != outer_bitmask) {
				return false;
			}
			/* TODO: Potential optimization, bitmask could contain a pair of u64 bitset and index,
			   that way it doesnt need to check 8 bitmasks
			*/
			if ((matcher.with_id_bitmask & id_bitmask) != matcher.with_id_bitmask) {
				return false;
			}
		}
		if (matcher.without_outer_bitmask != 0) {
			if ((matcher.without_outer_bitmask & outer_bitmask) != 0) {
				return false;
			}
			/* TODO: Potential optimization, bitmask could contain a pair of u64 bitset and index,
			   that way it doesnt need to check 8 bitmasks
			*/
			if ((matcher.without_id_bitmask & id_bitmask).any()) {
				return false;
			}
		}

		for (usz i = 0; i < matcher.with_ids.size; i++) {
			const Id id = matcher.with_ids.get(i);
			if (!this->columns_map.contains(id)) {
				return false;
			}
		}
		for (usz i = 0; i < matcher.without_ids.size; i++) {
			const Id id = matcher.without_ids.get(i);
			if (columns_map.contains(id)) {
				return false;
			}
		}

		return true;
	}
	
	void destroy();

    static Archetype* create_archetype(World* world, ArchetypeType archetype_type);
    static Archetype* ensure_archetype(World* world, ArchetypeType archetype_type);
};