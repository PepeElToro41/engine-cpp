#include "scheduler.hpp"

#include "core/logging.hpp"


// ReSharper disable CppInconsistentNaming
namespace PIPELINES {
	Pipeline* PRE_START() {
		static Pipeline pipeline = Pipeline("PreStart");
		return &pipeline;
	}
	Pipeline* START() {
		static Pipeline pipeline = Pipeline("Start");
		return &pipeline;
	}
	Pipeline* POST_START() {
		static Pipeline pipeline = Pipeline("PostStart");
		return &pipeline;
	}

	Pipeline* PRE_RENDER() {
		static Pipeline pipeline = Pipeline("PreRender");
		return &pipeline;
	}
	Pipeline* RENDER() {
		static Pipeline pipeline = Pipeline("Render");
		return &pipeline;
	}
	Pipeline* POST_RENDER() {
		static Pipeline pipeline = Pipeline("PostRender");
		return &pipeline;
	}

	Pipeline* PRE_PHYSICS() {
		static Pipeline pipeline = Pipeline("PrePhysics");
		return &pipeline;
	}
	Pipeline* PHYSICS() {
		static Pipeline pipeline = Pipeline("Physics");
		return &pipeline;
	}
	Pipeline* POST_PHYSICS() {
		static Pipeline pipeline = Pipeline("PostPhysics");
		return &pipeline;
	}

	Pipeline* CLEANUP() {
		static Pipeline pipeline = Pipeline("Cleanup");
		return &pipeline;
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Scheduler::add_system(System* system) {
	if (system->pipeline) {
		auto systems_list = this->pipeline_systems.get_ref(system->pipeline);
		if (!systems_list) {
			LOG_ERROR(
				"[SCHEDULER]: Cannot attach System: (%s) Because the Pipeline: (%s) it depends on has not been added", 
				system->name.string, 
				system->pipeline->name.string
			)
			return;
		}
		systems_list->push(system);
	} else if (system->phase) {
		auto systems_list = this->phase_systems.get_ref(system->phase);
		if (!systems_list) {
			LOG_ERROR(
				"[SCHEDULER]: Cannot attach System: (%s) Because the Phase: (%s) it depends on has not been added", 
				system->name.string, 
				system->phase->name.string
			)
			return;
		}
		systems_list->push(system);
	} else {
		LOG_ERROR("[SCHEDULER]: Cannot attach System: (%s) because it doesn't have a dependency", system->name.string)
	}
}

DependencyGraph* Scheduler::get_graph_for_phase(const Phase* phase) const {
	for (const auto [_, graph] : this->pipeline_graphs) {
		if (graph->contains(phase)) {
			return graph;
		}
	}
	return nullptr;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Scheduler::add_phase(const Phase* phase, const Pipeline* pipeline) {
	const auto can_graph = this->pipeline_graphs.get(pipeline);
	if (!can_graph.has_value()) {
		LOG_ERROR("Pipeline: (%s) needs to be registered, before using it as a dependency for Phase: (%s)", pipeline->name.string, phase->name.string)
	}
	DependencyGraph* graph = can_graph.value();
	graph->insert(phase);
	
	this->phase_systems.set(phase, DynamicArray<System*> { this->allocator });
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Scheduler::add_before(const Phase* phase, const Phase* before) {
	DependencyGraph* graph = this->get_graph_for_phase(phase);
	if (!graph) {
		LOG_ERROR("[SCHEDULER]: Cannot depend on Phase: (%s) for Phase: (%s) because the dependency has not been registered")
		return;
	}
	graph->insert_before(phase, before);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Scheduler::add_after(const Phase* phase, const Phase* after) {
	DependencyGraph* graph = this->get_graph_for_phase(phase);
	if (!graph) {
		LOG_ERROR("[SCHEDULER]: Cannot depend on Phase: (%s) for Phase: (%s) because the dependency has not been registered")
		return;
	}
	graph->insert_after(phase, after);
}


void Scheduler::add_pipeline(const Pipeline* pipeline) {
	DependencyGraph* graph = MEMORY::alloc<DependencyGraph>(this->allocator);
	new (graph) DependencyGraph(allocator);
	
	this->pipeline_graphs.set(pipeline, graph);
	this->pipeline_systems.set(pipeline, DynamicArray<System*> { this->allocator });
}


void Scheduler::execute_pipeline(const Pipeline* pipeline, EngineApp* app, EngineContext* context) const {
	const auto ordered_phases = this->pipeline_graphs.get(pipeline);
	if (!ordered_phases.has_value()) {
		LOG_ERROR("[SCHEDULER]: Pipeline %s cannot be executed because it has not been added yet", pipeline->name.string)
		return;
	}
	
	auto pipeline_systems = this->pipeline_systems.get(pipeline);
	if (pipeline_systems.has_value()) {
		for (const System* next_system : pipeline_systems.value()) {
			next_system->callback(app, context);
		}
	}
	
	for (const Phase* next_phase : ordered_phases.value()->get_sorted_nodes()) {
		auto phase_systems = this->phase_systems.get(next_phase);
		if (!phase_systems.has_value()) continue;
		for (const System* next_system : phase_systems.value()) {
			next_system->callback(app, context);
		}
	}
}


void Scheduler::add_builtin_pipelines() {
	this->add_pipeline(PIPELINES::PRE_START());
	this->add_pipeline(PIPELINES::START());
	this->add_pipeline(PIPELINES::POST_START());
	
	this->add_pipeline(PIPELINES::PRE_RENDER());
	this->add_pipeline(PIPELINES::RENDER());
	this->add_pipeline(PIPELINES::POST_RENDER());
	
	this->add_pipeline(PIPELINES::PRE_PHYSICS());
	this->add_pipeline(PIPELINES::PHYSICS());
	this->add_pipeline(PIPELINES::POST_PHYSICS());
	
	this->add_pipeline(PIPELINES::CLEANUP());
}

void Scheduler::destroy() {
	this->pipeline_graphs.each([this](const Pipeline* _, DependencyGraph* graph) {
		graph->free();
	});
	this->pipeline_systems.each([this](Pipeline* _, DynamicArray<System*> systems_arr) {
		systems_arr.free();
	});
	this->phase_systems.each([](Phase* _, DynamicArray<System*> systems_arr) {
		systems_arr.free();
	});
	this->pipeline_graphs.free();
	this->pipeline_systems.free();
	this->phase_systems.free();
}