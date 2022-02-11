/**
* @file anim_comp.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include "anim_comp.h"
#include "imgui.h"
#include <string>
#include "resources.h"
#include "node.h"
#include "transform.h"
#include "debug.h"
#include "scene_graph.h"
#include "clock.h"
#include <iostream>

namespace cs460 {
void anim_comp::initialize()
{
}

void anim_comp::update()
{
	if (m_use_blend_tree)
		blend_tree_update();
	else
		default_update();
}

void anim_comp::destroy()
{
	m_blend_tree.destroy();
}

void anim_comp::imgui()
{
	// Get the animations of the model
	const std::vector<animation>& anims = g_resources.get_model_rsc(get_owner()->m_model).m_anims;

	ImGui::Text("Animation");
	if (m_play)
	{
		if (ImGui::Button("Pause"))
			m_play = false;
	}
	else
	{
		if (ImGui::Button("Play"))
			m_play = true;
	}

	ImGui::SameLine();
	if (ImGui::Button("Reset"))
	{
		m_anim_time = m_start_time;
	}

	ImGui::SameLine();
	ImGui::Checkbox("Loop", &m_loop);
	
	ImGui::SameLine();
	ImGui::Checkbox("Nlerp", &m_nlerp);

	ImGui::SliderFloat("Animation Time", &m_anim_time, m_start_time, m_end_time);

	if (m_use_blend_tree)
	{
		m_blend_tree.imgui();
		return;
	}

	if (ImGui::BeginCombo("Set Animation", m_anim < 0 ? "none" : anims[m_anim].m_name.c_str()))
	{
		if (ImGui::Selectable("none"))
			m_anim = -1;

		size_t n_anims = anims.size();
		for (size_t i = 0; i < n_anims; ++i)
		{
			if (ImGui::Selectable(anims[i].m_name.c_str()))
			{
				m_anim = (int)i;
				set_animation();
				break;
			}
		}
		ImGui::EndCombo();
	}
}

void anim_comp::set_animation(int anim_idx)
{
	m_anim = anim_idx;
	set_animation();
}

void anim_comp::set_anim_factor(float factor)
{
	m_anim_factor = factor;
}

void anim_comp::set_1d_blend_tree()
{
	m_use_blend_tree = true;
	m_blend_tree.create_1d_blend_tree();
}

void anim_comp::set_2d_blend_tree()
{
	m_use_blend_tree = true;
	m_blend_tree.create_2d_blend_tree();
}

void anim_comp::insert_blend_node(int anim_idx, const glm::vec2& blend_pos)
{
	m_blend_tree.insert_blend_node(get_owner()->m_model, anim_idx, blend_pos);
}

void anim_comp::set_blend_tree_param(const glm::vec2& param)
{
	m_blend_tree.set_blend_param(param);
}

void anim_comp::default_update()
{
	if (m_anim >= 0)
	{
		// Get the animation
		int model_idx = get_owner()->m_model;
		int inst_idx = get_owner()->m_model_inst;
		const animation& anim = g_resources.get_model_rsc(model_idx).m_anims[m_anim];

		// Update animation timer
		if (m_play)
		{
			if (m_anim_time <= m_end_time)
				m_anim_time += m_anim_factor * g_clock.dt();

			if (m_loop && m_anim_time > m_end_time)
				m_anim_time = m_start_time;
		}

		// Get channel and samplers of the animation
		const auto& channels = anim.m_chanels;
		const auto& samplers = anim.m_samplers;

		// Go through each channel
		size_t n_channels = channels.size();
		for (size_t i = 0; i < n_channels; ++i)
		{
			const auto& ch = channels[i];
			const auto& sp = samplers.at(ch.m_sampler);

			// Get the node to interpolate
			node* n = g_scene.get_model_node(model_idx, inst_idx, ch.m_node);

			// Interpolate node
			if (ch.m_path_type == animation::channel::path_type::translation)
			{
				n->m_local.set_position(ch.lerp_pos(m_anim_time, sp));
			}
			else if (ch.m_path_type == animation::channel::path_type::rotation)
			{
				n->m_local.set_rotation(ch.lerp_rot(m_anim_time, sp, m_nlerp));
			}
			else if (ch.m_path_type == animation::channel::path_type::scale)
			{
				n->m_local.set_scale(ch.lerp_pos(m_anim_time, sp));
			}
		}
	}
}

void anim_comp::blend_tree_update()
{
	if (m_play)
		m_anim_time += g_clock.dt();

	// Get the final pose after blending
	bool fail = false;
	const anim_pose* final_pose = m_blend_tree.produce_pose(m_anim_time);

	// Check for errors
	if (!final_pose)
		return;

	// Set the node transforms
	auto end = final_pose->end();
	int model_idx = get_owner()->m_model;
	int instance_idx = get_owner()->m_model_inst;
	for (auto it = final_pose->begin(); it != end; ++it)
	{
		// Find the node and get its transform
		node* n = g_scene.get_model_node(model_idx, instance_idx, it->first);
		transform& tr = n->m_local;

		// Get the blend data
		const transform& blend_tr = it->second.first;
		unsigned char flags = it->second.second;

		// Set the translation
		if ((flags & target_type::translation) != 0)
			tr.set_position(blend_tr.get_position());

		// Set the orientation
		if ((flags & target_type::rotation) != 0)
			tr.set_rotation(blend_tr.get_rotation());

		// Set the scale
		if ((flags & target_type::scale) != 0)
			tr.set_scale(blend_tr.get_scale());
	}
}

void anim_comp::set_animation()
{
	const model_rsc& model = g_resources.get_model_rsc(get_owner()->m_model);
	if (m_anim >= 0)
		m_end_time = model.m_anims.at(m_anim).m_max_time;
	m_start_time = 0.0f;
}
}