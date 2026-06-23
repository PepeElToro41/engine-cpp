#include "context.hpp"

#include "core/logging.hpp"
#include "platform/platform.hpp"


template <typename T>
T* EngineContext::get_resource() {
	const usz type_hash = typeid(T).hash_code();
	
	auto resource = this->allocated_resources.get(type_hash);
	if (!resource.has_value()) {
		LOG_ERROR("No engine resource found for type: %s", typeid(T).name())
		return nullptr;
	}
	return resource.value();
}

template <typename T>
T* EngineContext::set_resource(T& resource) {
	T* new_resource = MEMORY::alloc<T>(this->allocator);
	
	PLATFORM::memcpy(new_resource, &resource, sizeof(T));
	
	const usz type_hash = typeid(T).hash_code();
	this->allocated_resources.set(type_hash, new_resource);
	
	return new_resource;
}

template <typename T>
T* EngineContext::create_resource() {
	T* new_resource = MEMORY::alloc<T>(this->allocator);
	
	const usz type_hash = typeid(T).hash_code();
	this->allocated_resources.set(type_hash, new_resource);
	
	return new_resource;
}