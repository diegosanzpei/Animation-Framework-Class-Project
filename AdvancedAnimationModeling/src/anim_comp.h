/**
* @file anim_comp.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include "component.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "blending.h"

namespace cs460 {
struct node_rsc;
struct model_rsc;
class blend_tree;
struct anim_comp : public component
{
	virtual void initialize();
	virtual void update();
	virtual void destroy();
	virtual void imgui();

	void set_animation(int anim_idx);
	void set_anim_factor(float factor);

	void set_1d_blend_tree();
	void set_2d_blend_tree();

	void insert_blend_node(int anim_idx, const glm::vec2& blend_pos);

	void set_blend_tree_param(const glm::vec2& param);

	const blend_tree& get_blend_tree() { return m_blend_tree; }

private:
	void default_update();
	void blend_tree_update();
	void set_animation();
	blend_tree m_blend_tree;
	bool m_use_blend_tree = false;

	bool m_nlerp = false;
	bool m_play = true;
	float m_anim_time = 0.0f;
	bool m_loop = true;
	int m_anim = -1;
	float m_start_time = 0.0f;
	float m_end_time = 0.0f;
	float m_anim_factor = 1.0f;
};
}