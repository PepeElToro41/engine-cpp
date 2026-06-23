#pragma once

#include "defines.hpp"
#include "templates/hashmap.hpp"

template<usz ARRAY_PORTION, typename K, typename T, T RESET> 
struct HybridHashMap {
	T array_portion[ARRAY_PORTION]{};
	
	HashMap<K, T> hash_portion{};
	
	inline std::optional<T> get(K key) const {
		if (key >= ARRAY_PORTION) {
			return this->hash_portion.get(key);
		} else {
			return this->array_portion[key];
		}
	}
	inline T* get_ref(K key) const {
		if (key >= ARRAY_PORTION) {
			return this->hash_portion.get_ref(key);
		} else {
			return &this->array_portion[key];
		}
	}
	inline void set(K key, T value) {
		if (key >= ARRAY_PORTION) {
			this->hash_portion.set(key, value);
		} else {
			this->array_portion[key] = value;
		}
	}
	inline void remove(K key, T value) {
		if (key >= ARRAY_PORTION) {
			this->hash_portion.remove(key, value);
		} else {
			this->array_portion[key] = RESET;
		}
	}
};
