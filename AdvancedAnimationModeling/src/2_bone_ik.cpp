#include "2_bone_ik.h"
#include "node.h"
#include <imgui.h>
#include "debug.h"
#include "editor.h"
#include "input.h"
#include "scene_graph.h"
#include "camera.h"
#include <iostream>
namespace cs460 {
void ik_2d::initialize()
{
	// Create the joints
	m_j1 = new node;
	m_j2 = new node;
	get_owner()->add_child(m_j1);
	m_j1->add_child(m_j2);

	// Set joind names
	m_j1->m_name = "joint 1";
	m_j2->m_name = "joint 2";

	// Set the default positions of the joints
	m_j1->m_local.set_position(glm::vec3(1.0f, 0.0f, 0.0f));
	m_j2->m_local.set_position(glm::vec3(1.0f, 0.0f, 0.0f));
}

void ik_2d::update()
{
	// Get input
	bool reach_target = g_input.mouseIsDown(mouse::button_left) && !ImGui::GetIO().WantCaptureMouse;
	if (reach_target)
	{
		update_target_pos();
		move_joints();
	}

	if (m_draw_limits)
		debug_draw_limits();

	debug_draw_target();
	debug_draw_bones();
}

void ik_2d::destroy()
{
}

void ik_2d::imgui()
{
	bool open = true;
	ImGui::Begin("2 bone IK", &open, ImGuiWindowFlags_NoMove);
	ImGui::Text("Link Lengths");
	
	if (ImGui::SliderFloat("Link 1", &m_d1, 0.1f, 5.0f))
	{
		m_j1->m_local.set_position(glm::vec3(m_d1, 0.0f, 0.0f));
		move_joints();
	}

	if (ImGui::SliderFloat("Link 2", &m_d2, 0.1f, 5.0f))
	{
		m_j2->m_local.set_position(glm::vec3(m_d2, 0.0f, 0.0f));
		move_joints();
	}

	ImGui::Separator();

	ImGui::Checkbox("Draw Reachable Workspace", &m_draw_limits);

	ImGui::End();
}

void ik_2d::update_target_pos()
{
	// Get the ray data
	const glm::vec3& origin = g_scene.get_camera().get_pos();
	glm::vec3 ray = g_editor.get_mouse_ray();

	// Raycast to xy plane
	float t = (get_owner()->m_world.get_position().z - origin.z) / ray.z;
	m_target.x = origin.x + t * ray.x;
	m_target.y = origin.y + t * ray.y;

	const glm::vec3& rp = get_owner()->m_world.get_position();
	glm::vec2 root_pos(rp.x, rp.y);
	m_target = m_target - root_pos;
}

void ik_2d::debug_draw_bones()
{
	// Draw the bones
	const glm::vec3& j0 = get_owner()->m_world.get_position();
	const glm::vec3& j1 = m_j1->m_world.get_position();
	const glm::vec3& j2 = m_j2->m_world.get_position();
	float offset = 0.01f;
	float size = 0.03f;

	glm::vec3 fwd = glm::normalize(j1 - j0);

	glm::vec4 color = m_reachable ? m_color : m_color3;

	g_debug.debug_draw_aabb(j0 - glm::vec3(size), j0 + glm::vec3(size), m_color2, false);
	g_debug.debug_draw_bone(j0 - fwd * offset, j1 + fwd * offset, glm::vec4(1.0f));
	g_debug.debug_draw_bone(j0, j1, color, false);

	fwd = glm::normalize(j2 - j1);

	g_debug.debug_draw_aabb(j1 - glm::vec3(size), j1 + glm::vec3(size), m_color2, false);
	g_debug.debug_draw_bone(j1 - fwd * offset, j2 + fwd * offset, glm::vec4(1.0f));
	g_debug.debug_draw_bone(j1, j2, color, false);

	g_debug.debug_draw_aabb(j2 - glm::vec3(size), j2 + glm::vec3(size), m_color2, false);
}

void ik_2d::debug_draw_target()
{
	float size = 0.04f;
	float size2 = 0.041f;
	const glm::vec3& rp = get_owner()->m_world.get_position();
	glm::vec3 target(rp.x + m_target.x, rp.y + m_target.y, rp.z);
	g_debug.debug_draw_aabb(target - glm::vec3(size), target + glm::vec3(size), m_color3, false);
	g_debug.debug_draw_aabb(target - glm::vec3(size2), target + glm::vec3(size2), glm::vec4(1.0f), true);
}

void ik_2d::debug_draw_limits()
{
	// Reachable workspace
	debug_draw_circle(m_d1 + m_d2, glm::vec4(1.0f, 1.0f, 1.0f, 0.2f), glm::vec4(1.0f, 1.0f, 1.0f, 0.8f), -0.02f);

	// Unreachable inner circle
	float inner_radius = m_d1 - m_d2;
	if (m_d1 - m_d2 > 0.0f)
		debug_draw_circle(inner_radius, glm::vec4(0.8f, 0.1f, 0.0f, 0.2f), glm::vec4(0.8f, 0.1f, 0.0f, 0.8f), -0.01f);

}

void ik_2d::debug_draw_circle(float radius, const glm::vec4& color, const glm::vec4& cir_color, float off)
{
	float delta = 5.0f;
	glm::vec3 center = get_owner()->m_world.get_position() + glm::vec3(0.0f, 0.0f, off);
	glm::vec3 prev_point = center + glm::vec3(radius, 0.0f, 0.0f);
	for (float i = delta; i <= 360.0f; i += delta)
	{
		// Compute point
		float angle = glm::radians(i);
		glm::vec3 point(radius * glm::cos(angle), radius * glm::sin(angle), 0.0f);
		point = center + point;

		// Draw triangle
		g_debug.debug_draw_triangle(center, prev_point, point, color);
		g_debug.debug_draw_line(prev_point, point, cir_color);
		prev_point = point;
	}
}

void ik_2d::move_joints()
{
	// Cosine of joint 2 angle
	float a = glm::dot(m_target, m_target);
	float b = m_d1 * m_d1 + m_d2 * m_d2;
	float c = 2.0f * m_d1 * m_d2;
	float j2_cos = (a - b) / c;

	// Target out of reach
	m_reachable = (j2_cos >= -1.0f && j2_cos <= 1.0f);
	if (!m_reachable)
		return;

	// Compute angle of joint 2
	float j2_angle = glm::acos(j2_cos);
	
	// Compute angle of joint 1
	a = m_d1 + m_d2 * j2_cos;
	b = m_d2 * glm::sin(j2_angle);
	float y = m_target.y * a - m_target.x * b;
	float x = m_target.x * a + m_target.y * b;
	float j1_angle = atan2(y, x);

	// Move joint 1
	get_owner()->m_local.set_rotation(glm::quat(glm::vec3(0.0f, 0.0f, j1_angle)));

	// Move joint 2
	m_j1->m_local.set_rotation(glm::quat(glm::vec3(0.0f, 0.0f, j2_angle)));
}
}