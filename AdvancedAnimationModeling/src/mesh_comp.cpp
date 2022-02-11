/**
* @file mesh_comp.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include "mesh_comp.h"
#include "imgui.h"
#include <string>
#include "resources.h"
#include "node.h"
#include "transform.h"
#include "debug.h"
#include "scene_graph.h"
#include "clock.h"

namespace cs460 {
void mesh_comp::initialize()
{
}

void mesh_comp::update()
{
}

void mesh_comp::imgui()
{
	ImGui::Text("Mesh Component");
	ImGui::Text(("Mesh Index: " + std::to_string(m_mesh)).c_str());
	const std::string& name = g_resources.get_model_mesh(get_owner()->m_model, m_mesh).m_name;
	ImGui::Text(("Mesh Name: " + name).c_str());
	ImGui::ColorEdit4("Bounding Volume Color", &m_bv_color[0]);
}

void mesh_comp::set_mesh(int mesh_idx, int skin_idx, int skin_root_idx)
{
	m_mesh = mesh_idx;
	m_skin = skin_idx;
	m_skin_root = skin_root_idx;

	// Set the skin
	if (skin_idx >= 0)
		set_skin();

	// Get the mesh from the resource manager
	const model_rsc& model = g_resources.get_model_rsc(get_owner()->m_model);
	const mesh& m = model.m_meshes[mesh_idx];

	// Get the bounding volume of the mesh
	const glm::vec3& min = m.m_min_vertex;
	const glm::vec3& max = m.m_max_vertex;

	// Compute bv scale
	glm::vec3 scale = max - min;

	// Compute the entire bounding volume
	m_bv[0] = min;
	m_bv[1] = min + glm::vec3(0.0f, 0.0f, scale.z);
	m_bv[2] = min + glm::vec3(0.0f, scale.y, 0.0f);
	m_bv[3] = min + glm::vec3(0.0f, scale.y, scale.z);
	m_bv[4] = min + glm::vec3(scale.x, 0.0f, 0.0f);
	m_bv[5] = min + glm::vec3(scale.x, 0.0f, scale.z);
	m_bv[6] = min + glm::vec3(scale.x, scale.y, 0.0f);
	m_bv[7] = max;
}

void mesh_comp::render_vb()
{
	if (m_mesh < 0)
		return;

	glm::vec3 min, max;
	get_vb_min_max(min, max);

	// Draw the bounding volume
	g_debug.debug_draw_aabb(min, max, m_bv_color);
}

void mesh_comp::render_skin()
{
	// Model idx and instance
	int model_inst = get_owner()->m_model_inst;
	int model_idx = get_owner()->m_model;

	// Render bones
	glm::vec4 bone_color(0.0f, 1.0f, 0.0f, 1.0f);
	size_t n_segments = m_skin_segments.size();
	for (size_t i = 0; i < n_segments; i += 2)
	{
		const node* start = g_scene.get_model_node(model_idx, model_inst, m_skin_segments[i]);
		const node* end   = g_scene.get_model_node(model_idx, model_inst, m_skin_segments[i + 1]);
		g_debug.debug_draw_line(start->m_world.get_position(), end->m_world.get_position(), bone_color);
	}

	// Render joints
	const std::vector<int>& joints = g_resources.get_model_skin(model_idx, m_skin).m_joints;
	glm::vec3 offset(0.01f);
	glm::vec4 joint_color(1.0f, 0.0f, 0.0f, 1.0f);
	size_t n_joints = joints.size();
	for (size_t i = 0; i < n_joints; ++i)
	{
		const node* joint = g_scene.get_model_node(model_idx, model_inst, joints[i]);
		g_debug.debug_draw_aabb(joint->m_world.get_position() - offset, joint->m_world.get_position() + offset, joint_color, false, false);
	}
}

void mesh_comp::get_vb_min_max(glm::vec3& min, glm::vec3& max)
{
	// Get the node transform
	glm::mat4 m2w = get_owner()->m_world.compute_matrix();

	// Transform all the points to world space
	glm::vec3 bv_temp[8];
	for (int i = 0; i < 8; ++i)
		bv_temp[i] = m2w * glm::vec4(m_bv[i], 1.0f);

	// Get the new min and max points
	min = bv_temp[0];
	max = bv_temp[0];
	for (int i = 1; i < 8; ++i)
	{
		for (int axis = 0; axis < 3; ++axis)
		{
			if (bv_temp[i][axis] < min[axis])
				min[axis] = bv_temp[i][axis];
			else if (bv_temp[i][axis] > max[axis])
				max[axis] = bv_temp[i][axis];
		}
	}
}

void mesh_comp::set_skin()
{
	// Get the resources of the model
	const model_rsc& resources = g_resources.get_model_rsc(get_owner()->m_model);
	
	// Get the root node of the skin
	const node_rsc* root = &resources.m_nodes.at(m_skin_root);
	
	// Check if the root is a joint
	bool use_root_as_joint = false;
	const std::vector<int>& joints = resources.m_skins[m_skin].m_joints;
	size_t n_joints = joints.size();
	for (size_t i = 0; i < n_joints; ++i) 
	{
		if (joints[i] == m_skin_root)
		{
			use_root_as_joint = true;
			break;
		}
	}

	// Save the skin segments
	if (use_root_as_joint)
		set_skin_rec(resources, m_skin_root);
	else
	{
		const std::vector<int>& childs = resources.m_nodes.at(m_skin_root).m_childs;
		size_t n_childs = childs.size();
		for (size_t i = 0; i < n_childs; ++i)
			set_skin_rec(resources, childs[i]);
	}
}

void mesh_comp::set_skin_rec(const model_rsc& resources, const int node)
{
	const std::vector<int>& childs = resources.m_nodes.at(node).m_childs;
	size_t n_childs = childs.size();
	for (size_t i = 0; i < n_childs; ++i)
	{
		m_skin_segments.push_back(node);
		m_skin_segments.push_back(childs[i]);
		set_skin_rec(resources, childs[i]);
	}
}
}