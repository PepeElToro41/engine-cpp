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


template <typename K, typename T>
struct HashEntry {
	usz hash;
	std::pair<K , T> key_value;
	HashEntry* next = nullptr;
};

template <typename K, typename T>
struct HashMap {
	usz count = 0;
	float load_factor;
	
	using Entry = HashEntry<K, T>;
	
	std::hash<K> hasher;
	BaseAllocator* allocator;
	Entry** buckets = nullptr;
	
	usz capacity = 0;
	usz threshold = 0;
	
	explicit HashMap(
		BaseAllocator* allocator,
		const float load_factor = HASHMAP::DEFAULT_LOAD_FACTOR
	) : 
		load_factor(load_factor),
		allocator(allocator)
	{}
	
	void init(const usz initial_capacity = HASHMAP::DEFAULT_INITIAL_CAPACITY) {
		this->resize(initial_capacity);
	}
	
	bool contains(const K& key) const {
		if (!this->count) return false;
		
		u64 hash = this->hasher(key);
		usz index = this->bucket_for(hash);
		
		for (Entry* entry = this->buckets[index]; entry != nullptr; entry = entry->next) {
			if (entry->hash == hash && entry->key_value.first == key) {
				return true;
			}
		}
		return false;
	}
	
	std::optional<T> get(const K& key) const {
		if (!this->count) return {};
		
		u64 hash = this->hasher(key);
		usz index = this->bucket_for(hash);
        
		for (Entry* entry = this->buckets[index]; entry != nullptr; entry = entry->next) {
			if (entry->hash == hash && entry->key_value.first == key) {
				return entry->key_value.second;
			}
		}
		return {};
	}
	
	T* get_ref(const K& key) const {
		if (!this->count) return nullptr;
		
		u64 hash = this->hasher(key);
		usz index = this->bucket_for(hash);
        
		for (Entry* entry = this->buckets[index]; entry != nullptr; entry = entry->next) {
			if (entry->hash == hash && entry->key_value.first == key) {
				return &entry->key_value.second;
			}
		}
		return nullptr;
	}
	
	bool set(const K& key, T value) {
		u64 hash = this->hasher(key);
		
		if (this->buckets != nullptr) {
			const usz index = this->bucket_for(hash);
			
			for (Entry* entry = this->buckets[index]; entry != nullptr; entry = entry->next) {
				if (entry->hash == hash && entry->key_value.first == key) {
					entry->key_value.second = value;
					return false;
				}
			}
		}
		
		if (this->count++ >= this->threshold) {
			this->resize(this->capacity == 0 ? HASHMAP::DEFAULT_INITIAL_CAPACITY : this->capacity * 2);
		}
		const usz index = this->bucket_for(hash);
		Entry* new_entry = MEMORY::alloc<Entry>(this->allocator);
		
		new_entry->hash = hash;
		new_entry->key_value.first = key;
		new_entry->key_value.second = value;
		new_entry->next = this->buckets[index];
		this->buckets[index] = new_entry;
		return true;
	}
	
	bool remove(const K& key) {
		if (!this->count) return false;
		if (this->buckets == nullptr) return false;
		
		u64 hash = this->hasher(key);
		usz index = this->bucket_for(hash);
		
		Entry* previous = nullptr;
		Entry* entry = this->buckets[index];
		
		while (entry != nullptr) {
			Entry* next = entry->next;
			
			if (entry->hash == hash && entry->key_value.first == key) {
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
	
	template <typename C>
	void each(C&& callback) {
		for (usz i = 0; i < this->capacity; i++) {
			Entry* entry = this->buckets[i];
			while (entry != nullptr) {
				callback(entry->key_value.first, entry->key_value.second);
				entry = entry->next;
			}
		}
	}
	
	template <typename C>
	void each_ref(C&& callback) {
		for (usz i = 0; i < this->capacity; i++) {
			Entry* entry = this->buckets[i];
			while (entry != nullptr) {
				callback(entry->key_value.first, &entry->key_value.second);
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
		
		this->count = 0;
	}
	
	void resize(usz new_capacity) {
		if (!new_capacity) return;
		if (this->capacity >= new_capacity) return;
		
		new_capacity = MATH::next_power_of_two(new_capacity);
		this->capacity = new_capacity;
		this->threshold = (u64)((float)new_capacity * this->load_factor);
		
		if (this->buckets) {
			Entry** old_buckets = this->buckets;
			Entry** new_buckets = MEMORY::alloc<Entry*>(this->allocator, this->capacity);
			
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
	
	void free_entry(Entry* entry) {
		MEMORY::free(this->allocator, entry);
	}
	
	inline usz bucket_for(u64 hash) const {
		return hash & (this->capacity - 1);
	}
	
	struct Iterator {
		const HashMap* hashmap;
		usz bucket_index;
		Entry* current;
		
		Iterator(const HashMap* hashmap, const usz bucket_index, Entry* current) :
			hashmap(hashmap),
			bucket_index(bucket_index),
			current(current)
		{
			this->advance();
		}
		
		void advance() {
			while (current == nullptr && bucket_index < hashmap->capacity) {
				current = hashmap->buckets[bucket_index];
				if (current == nullptr) {
					++bucket_index;
				}
			}
		}
		
		std::pair<const K, T>& operator*() const { return current->key_value; }
		std::pair<const K, T>* operator->() const { return &current->key_value; }

		Iterator& operator++() {
			current = current->next;
			if (current == nullptr) {
				++bucket_index;
				advance();
			}
			return *this;
		}
		
		bool operator!=(const Iterator& other) const { return current != other.current; }
		bool operator==(const Iterator& other) const { return current == other.current; }
	};
	
	Iterator begin() const {
		if (this->buckets == nullptr || this->count == 0) return end();
		return Iterator(this, 0, nullptr);
	}
	
	Iterator end() const {
		return Iterator(this, this->capacity, nullptr);
	}
};