#pragma once
#include "dependency_graph.hpp"
#include "phase.hpp"
#include "pipeline.hpp"
#include "system.hpp"
#include "templates/dynamic_array.hpp"
#include "templates/hashmap.hpp"


struct Scheduler {
	BaseAllocator* allocator;
	HashMap<const Phase*, DynamicArray<System*>> phase_systems;
	HashMap<const Pipeline*, DynamicArray<System*>> pipeline_systems;
	
	HashMap<const Pipeline*, DependencyGraph*> pipeline_graphs;
 	
	
	explicit Scheduler(BaseAllocator* allocator) : 
		allocator(allocator),
		phase_systems(allocator),
		pipeline_systems(allocator),
		pipeline_graphs(allocator)
	{}
	
	void add_system(System* system);
	
	DependencyGraph* get_graph_for_phase(const Phase* phase) const;
	
	void add_phase(const Phase* phase, const Pipeline* pipeline);
	void add_before(const Phase* phase, const Phase* before);
	void add_after(const Phase* phase, const Phase* after);
	
	void add_builtin_pipelines();
	void add_pipeline(const Pipeline* pipeline);
	
	void execute_pipeline(const Pipeline* pipeline, EngineApp* app, EngineContext* context) const;
	
	void destroy();
};

// ReSharper disable CppInconsistentNaming
namespace PIPELINES {
	Pipeline* PRE_START();
	Pipeline* START();
	Pipeline* POST_START();
	
	Pipeline* PRE_RENDER();
	Pipeline* RENDER();
	Pipeline* POST_RENDER();
	
	Pipeline* PRE_PHYSICS();
	Pipeline* PHYSICS();
	Pipeline* POST_PHYSICS();
	
	Pipeline* CLEANUP();
}