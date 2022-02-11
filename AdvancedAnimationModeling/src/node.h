/**
* @file node.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include "transform.h"
#include <vector>
#include <string>
#include "component.h"

namespace cs460 {
struct node
{
	std::string m_name;
	node* m_parent = nullptr;
	std::vector<node*> m_children;

	// Model and node indices (gltf indices)
	int m_model = -1;
	int m_node_idx = -1;
	
	// Instance id
	int m_model_inst = 0;

	// Free all components
	~node();

	// Updates components
	void update();
	void imgui();
	void add_child(node* child);
	
	// Returns a pointer to the requested component
	// Returns null pointer if there is no T component
	template <typename T>
	T* get_component() {
		// Search for T component
		size_t n_comps = m_comps.size();
		for (size_t i = 0; i < n_comps; ++i) {
			if (T* comp = dynamic_cast<T*>(m_comps[i]))
				return comp;
		}

		// Not found
		return nullptr;
	}

	// Adds a component
	template <typename T>
	T* add_component() {
		// Create the component
		T* comp = new T;

		// Set the owner of the component
		comp->set_owner(this);

		// Initialize the component
		comp->initialize();
		
		// Store the component
		m_comps.push_back(comp);

		// Not found
		return comp;
	}

	// Transforms
	transform m_local;
	transform m_world;
	std::vector<component*> m_comps;
};
}