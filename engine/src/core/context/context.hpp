#pragma once
#include "core/memory/memory.hpp"
#include "templates/hashmap.hpp"

struct EngineContext {
	BaseAllocator* allocator;
	EngineContext* parent;
	
	HashMap<usz, void*> allocated_resources;
	
	explicit EngineContext(BaseAllocator* allocator)  : 
		allocator(allocator),
		parent(nullptr), 
		allocated_resources(allocator) 
	{}
	
	void init();
	
	template<typename T>
	T* set_resource(T& resource);
	
	template<typename T>
	T* create_resource();

	template<typename T>
	T* get_resource();
};
