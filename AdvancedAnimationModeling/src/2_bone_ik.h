/**
* @file 2_bone_ik.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include "component.h"
#include <glm/glm.hpp>

namespace cs460 {
struct node;
struct ik_2d : public component
{
public:
	virtual void initialize();
	virtual void update();
	virtual void destroy();
	virtual void imgui();

private:
	void update_target_pos();
	void debug_draw_bones();
	void debug_draw_target();
	void debug_draw_limits();
	void debug_draw_circle(float radius, const glm::vec4& color, const glm::vec4& cir_color, float off = 0.0f);
	void move_joints();

	bool m_reachable = true;
	bool m_draw_limits = true;

	float m_d1 = 1.0f, m_d2 = 1.0f;
	node* m_j1 = nullptr, *m_j2 = nullptr;

	glm::vec2 m_target = glm::vec2(0.0f);

	glm::vec4 m_color = glm::vec4(0.0f, 0.5f, 0.7f, 0.8f);
	glm::vec4 m_color2 = glm::vec4(0.8f, 0.4f, 0.0f, 0.8f);
	glm::vec4 m_color3 = glm::vec4(0.8f, 0.1f, 0.0f, 0.8f);
};
}