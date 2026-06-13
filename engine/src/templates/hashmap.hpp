#pragma once

#include <optional>
#include <utility>

#include "helpers.hpp"
#include "defines.hpp"
#include "core/memory/memory.hpp"

namespace HASHMAP {
	constexpr usz DEFAULT_INITIAL_CAPACITY = 16;
	constexpr float DEFAULT_LOAD_FACTOR = 0.75f;
}

template <typename Key, typename Value>
struct HashEntry {
	usz hash;
	Key key;
	Value value;
	HashEntry* next = nullptr;
};

template <typename Key, typename Value>
struct HashMap {
	usz count = 0;
	float load_factor;
	
	using Entry = HashEntry<Key, Value>;
	
	std::hash<Key> hasher;
	BaseAllocator* allocator;
	Entry** buckets = nullptr;
	
	explicit HashMap(
		BaseAllocator* allocator,
		const float load_factor = HASHMAP::DEFAULT_LOAD_FACTOR
	) : 
		load_factor(load_factor),
		allocator(allocator)
	{}
	
	void init(usz initial_capacity = HASHMAP::DEFAULT_INITIAL_CAPACITY) {
		this->resize(initial_capacity);
	}
	
	bool contains(Key key) const {
		if (!this->count) return false;
		
		u64 hash = this->hasher(key);
		u64 index = this->bucket_for(hash);
		
		for (Entry* entry = this->buckets[index]; entry != nullptr; entry = entry->next) {
			if (entry->hash == hash && entry->key == key) {
				return true;
			}
		}
		return false;
	}
	
	std::optional<Value> get(Key key) const {
		if (!this->count) return {};
		
		u64 hash = this->hasher(key);
		u32 index = this->bucket_for(hash);
        
		for (Entry* entry = this->buckets[index]; entry != nullptr; entry = entry->next) {
			if (entry->hash == hash && entry->key == key) {
				return entry->value;
			}
		}
		return {};
	}
	
	std::optional<Value*> get_ref(Key key) const {
		if (!this->count) return {};
		
		u64 hash = this->hasher(key);
		u32 index = this->bucket_for(hash);
        
		for (Entry* entry = this->buckets[index]; entry != nullptr; entry = entry->next) {
			if (entry->hash == hash && entry->key == key) {
				return &entry->value;
			}
		}
		return {};
	}
	
	bool set(Key key, Value value) {
		u64 hash = this->hasher(key);
		
		if (this->buckets != nullptr) {
			const usz index = this->bucket_for(key);
			
			for (Entry* entry = this->buckets[index]; entry != nullptr; entry = entry->next) {
				if (entry->hash == hash && entry->key == key) {
					entry->value = value;
					return false;
				}
			}
		}
		
		if (this->count++ >= this->threshold) {
			this->resize(this->capacity);
		}
		const usz index = this->bucket_for(key);
		const Entry* new_entry = MEMORY::alloc<Entry>(this->allocator);
		
		new_entry->hash = hash;
		new_entry->key = key;
		new_entry->value = value;
		new_entry->next = this->buckets[index];
		this->buckets[index] = new_entry;
		return true;
	}
	
	bool remove(Key key) {
		if (!this->count) return false;
		if (this->buckets != nullptr) return false;
		
		u64 hash = this->hasher(key);
		u32 index = this->bucket_for(hash);
		
		Entry* previous = nullptr;
		Entry* entry = this->buckets[index];
		
		while (entry != nullptr) {
			Entry* next = entry->next;
			
			if (entry->hash == hash && entry->key == key) {
				--this->count;

				if (previous != nullptr) {
					previous->next = next;
				} else {
					this->buckets[index] = next;
				}

				this->free_entry(entry);
				return true;
			}

			previous = entry;
			entry = next;
		}
		return false;
	}
	
	template <typename Callback>
	void each(Callback&& callback) {
		for (u32 i = 0; i < this->capacity; i++) {
			Entry* entry = this->buckets[i];
			while (entry != nullptr) {
				callback(entry->key, entry->value);
				entry = entry->next;
			}
		}
	}
	
	template <typename Callback>
	void each_ref(Callback&& callback) {
		for (u32 i = 0; i < this->capacity; i++) {
			Entry* entry = this->buckets[i];
			while (entry != nullptr) {
				callback(entry->key, &entry->value);
				entry = entry->next;
			}
		}
	}
	
	void free() {
		if (!this->count) return;
		if (this->buckets == nullptr) return;
		
		for (usz i = 0; i < this->capacity; i++) {
			Entry* entry = this->buckets[i];
			while (entry != nullptr) {
				Entry* next = entry->next;
				MEMORY::free(this->allocator, entry);
				entry = next;
			}
		}
		MEMORY::free(this->allocator, this->buckets);
	}
	
	void clear() {
		if (!this->count) return;
		if (this->buckets == nullptr) return;
		
		for (usz i = 0; i < this->capacity; i++) {
			Entry* entry = this->buckets[i];
			while (entry != nullptr) {
				Entry* next = entry->next;
				MEMORY::free(this->allocator, entry);
				entry = next;
			}
			this->buckets[i] = nullptr;
		}
	}
	
	void resize(usz new_capacity) {
		if (!new_capacity) return;
		if (this->capacity >= new_capacity) return;
		
		new_capacity = MATH::next_power_of_two(new_capacity);
		this->capacity = new_capacity;
		this->threshold = (u64)((float)new_capacity * this->load_factor);
		
		if (this->buckets) {
			Entry** old_buckets = this->buckets;
			Entry** new_buckets = MEMORY::alloc<Entry*>(this->capacity);
			
			for (usz i = 0; i < this->capacity; i++) {
				Entry* entry = old_buckets[i];
				while (entry != nullptr) {
					Entry* next = entry->next;
					const usz index = this->bucket_for(entry->hash);
					entry->next = new_buckets[index];
					new_buckets[index] = entry;
					entry = next;
				}
			}
			
			this->buckets = new_buckets;
			MEMORY::free(this->allocator, old_buckets);
		} else {
			this->buckets = MEMORY::alloc<Entry*>(this->allocator, new_capacity);
		}
	}
	
private:
	usz capacity = 0;
	usz threshold = 0;
	
	void free_entry(Entry* entry) {
		MEMORY::free(this->allocator, entry);
	}
	
	inline usz bucket_for(u64 hash) {
		return hash & (this->capacity - 1);
	}
};