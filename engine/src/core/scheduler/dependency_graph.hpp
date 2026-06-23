#pragma once

#include "phase.hpp"
#include "core/logging.hpp"
#include "core/memory/allocators/temporal_allocator.hpp"
#include "templates/dynamic_array.hpp"

struct Phase;

struct DependencyGraph {
	BaseAllocator* allocator;
	DynamicArray<DynamicArray<usz>> nodes_edges;
	DynamicArray<const Phase*> nodes;
	bool dirty = false;
	
	explicit DependencyGraph(BaseAllocator* allocator) : 
		allocator(allocator),
		nodes_edges(allocator),
		nodes(allocator),
		sorted_nodes(allocator)
	{}
	
	usz push_node(const Phase* node) {
		const usz pushed = this->nodes.size;
		this->nodes.push(node);
		
		this->nodes_edges.fill(this->nodes.size);
		this->nodes_edges.set(pushed, DynamicArray<usz>{ this->allocator });
		return pushed;
	}
	
	bool contains(const Phase* node) {
		return this->nodes.contains(node);
	}
	 
	void insert(const Phase* node) {
		this->dirty = true;
		this->push_node(node);
	}
	void insert_before(const Phase* node, const Phase* before) {
		const auto dependency_id = this->nodes.find(before);
		if (!dependency_id.has_value()) {
			LOG_ERROR("[SCHEDULER]: Before dependency has not been added")
			return;
		}
		this->dirty = true;
		const usz push_id = this->push_node(node);
		
		DynamicArray<usz>* edges = this->nodes_edges.get_ref(push_id);
		edges->push(dependency_id.value());
	}
	void insert_after(const Phase* node, const Phase* after) {
		const auto dependency_id = this->nodes.find(after);
		if (!dependency_id.has_value()) {
			LOG_ERROR("[SCHEDULER]: After dependency: (%s) has not been added for Phase: (%s)", after->name.string, node->name.string)
			return;
		}
		this->dirty = true;
		const usz push_id = this->push_node(node);
		
		DynamicArray<usz>* edges = this->nodes_edges.get_ref(dependency_id.value());
		edges->push(push_id);
	}
	
	DynamicArray<const Phase*> get_sorted_nodes() {
		if (!dirty) {
			return this->sorted_nodes;
		}
		TemporalAllocator temp = TemporalAllocator::create();
		DynamicArray<usz> in_degrees { &temp };
		DynamicArray<usz> queue { &temp };
		
		this->sorted_nodes.clear();
		this->sorted_nodes.ensure_capacity(this->nodes.size);
		
		in_degrees.fill(this->nodes.size);
		queue.ensure_capacity(this->nodes.size);
		
		for (DynamicArray<usz> edges : this->nodes_edges) {
			for (const usz node_id : edges) {
				usz* adjacent_count = in_degrees.get_ref(node_id);
				*adjacent_count += 1;
			}
		}
		
		for (usz node_id = 0; node_id < this->nodes.size; node_id++) {
			const usz adjacent_count = in_degrees.get(node_id);
			if (adjacent_count == 0) {
				queue.push(node_id);
			}
		}
		
		while (!queue.empty()) {
			usz first_id = queue.first();
			queue.remove(0);
			this->sorted_nodes.push(this->nodes.get(first_id));
			
			for (usz next_id : this->nodes_edges.get(first_id)) {
				usz* adjacent_count = in_degrees.get_ref(next_id);
				*adjacent_count -= 1;
				if (*adjacent_count == 0) {
					queue.push(next_id);
				}
			}
		}
		
		this->dirty = false;
		return this->sorted_nodes;
	}
	
	void free() {
		this->nodes_edges.free();
		this->nodes.free();
		this->sorted_nodes.free();
	}
	
private:
	DynamicArray<const Phase*> sorted_nodes;
};