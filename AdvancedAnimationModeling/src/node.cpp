/**
* @file node.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include "node.h"
#include <imgui.h>

namespace cs460 {
// Free all components
node::~node()
{
	size_t n_comps = m_comps.size();
	for (size_t i = 0; i < n_comps; ++i)
	{
		m_comps[i]->destroy();
		delete m_comps[i];
	}
}

// Update all components
void node::update()
{
	size_t n_comps = m_comps.size();
	for (size_t i = 0; i < n_comps; ++i)
		m_comps[i]->update();
}

void node::imgui()
{
	ImGui::Separator();
	m_local.imgui();
	size_t n_comps = m_comps.size();
	for (size_t i = 0; i < n_comps; ++i)
	{
		ImGui::Separator();
		m_comps[i]->imgui();
	}
}

void node::add_child(node* child)
{
	child->m_parent = this;
	m_children.push_back(child);
}
}