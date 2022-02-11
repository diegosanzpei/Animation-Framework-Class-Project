/**
* @file inverse_kinematics.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include "component.h"
#include <glm/glm.hpp>
#include <vector>

namespace cs460 {
struct node;
struct ik_base : public component
{
public:
	virtual void initialize();
	virtual void update();
	virtual void destroy();
	virtual void imgui();

	virtual void clear_joints();
	virtual void add_joint(node* joint);

	void show_gui(bool gui) { m_use_gui = gui; }
	void render_bones(bool render) { m_render_bones = render; }

	void default_init();

protected:
	void debug_draw_bones();
	void debug_draw_target();

	virtual void move_joints() {}
	virtual void add_joint();
	virtual void remove_joint();
	
	bool solution_found();

	glm::quat axis_angle_to_quat(const glm::vec3& v1, const glm::vec3& v2);
	void update_world_transforms(int start_idx);

	bool m_use_gui = true;
	bool m_render_bones = true;

	int m_iterations = 30;
	int m_counter = 0;
	float m_threshold = 0.01f;
	bool m_target_moving = false;
	glm::vec3 m_prev_pos = glm::vec3(0.0f, 0.0f, 0.0f);
	enum solver_status { processing, failure, success};
	solver_status m_status = success;

	std::vector<node*> m_joints;
	std::vector<float> m_dists;

	glm::vec4 m_color = glm::vec4(0.0f, 0.5f, 0.7f, 0.8f);
	glm::vec4 m_color_process = glm::vec4(0.8f, 0.8f, 0.0f, 0.8f);
	glm::vec4 m_color2 = glm::vec4(0.8f, 0.4f, 0.0f, 0.8f);
	glm::vec4 m_color3 = glm::vec4(0.8f, 0.1f, 0.0f, 0.8f);
};

struct ccd : public ik_base
{
public:

protected:
	virtual void imgui();
	virtual void move_joints();
};

struct fabrik : public ik_base
{
public:
	virtual void clear_joints();
	virtual void add_joint(node* joint);

protected:
	virtual void imgui();
	virtual void move_joints();
	virtual void add_joint();
	virtual void remove_joint();

	// Temporal position buffer (used in fabrik algorithm)
	std::vector<glm::vec3> m_positions;
};
}