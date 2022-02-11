#include "player_controller.h"
#include "scene_graph.h"
#include "camera.h"
#include <imgui.h>
#include "input.h"
#include <glm/glm.hpp>
#include "node.h"
#include "clock.h"
#include "anim_comp.h"
#include "debug.h"

namespace cs460 {
void player_controller::initialize()
{
	m_cam = &g_scene.get_camera();
	m_cam->camera_has_control(false);
	m_anim = get_owner()->get_component<anim_comp>();

	if (m_directional)
	{
		glm::vec3& pos = m_cam->m_pos;

		// Spherical camera
		pos.y = glm::sin(glm::radians(m_beta)) * m_radius;
		float new_radius = glm::cos(glm::radians(m_beta)) * m_radius;
		pos.x = glm::sin(glm::radians(m_alpha)) * new_radius;
		pos.z = glm::cos(glm::radians(m_alpha)) * new_radius;

		// Position of the player
		const glm::vec3& player_pos = get_owner()->m_local.get_position() + glm::vec3(0.0f, 1.0f, 0.0f);
		pos += player_pos;
		m_cam->m_view = glm::normalize(player_pos - pos);
	}
	else
	{
		// Player pos
		glm::vec3& player_pos = get_owner()->m_local.get_position();

		// Orientation
		glm::vec3 dir = glm::normalize(m_target - player_pos);
		orientation(dir);

		// Move camera
		glm::vec3 cam_pos = player_pos - dir * m_radius;
		cam_pos.y += 2.0f;
		m_cam->m_pos = cam_pos;
		m_cam->m_view = glm::normalize(m_target - m_cam->m_pos);
	}
}

void player_controller::update()
{
	if (m_directional)
	{
		move_camera();
		move_player();
	}
	else
		targeted_movement();
}

void player_controller::destroy()
{
}

void player_controller::move_camera()
{
	const glm::vec2& right_stick = g_input.getGamePadStickVec(gamepad::right_stick);

	// Move camera
	float speed = 60.0f * g_clock.dt();
	m_alpha += -right_stick.x * 2.0f * speed;
	m_beta += -right_stick.y * speed;

	// Clamp
	m_beta = glm::clamp(m_beta, -10.0f, 60.0f);

	glm::vec3& pos = m_cam->m_pos;
	glm::vec3 dest;

	// Spherical camera
	dest.y = glm::sin(glm::radians(m_beta)) * m_radius;
	float new_radius = glm::cos(glm::radians(m_beta)) * m_radius;
	dest.x = glm::sin(glm::radians(m_alpha)) * new_radius;
	dest.z = glm::cos(glm::radians(m_alpha)) * new_radius;

	// Position of the player
	const glm::vec3& player_pos = get_owner()->m_local.get_position() + glm::vec3(0.0f, 1.0f, 0.0f);
	dest += player_pos;
	pos = pos + (dest - pos) * 10.0f * g_clock.dt();
	m_cam->m_view = glm::normalize(player_pos - pos);
}

void player_controller::move_player()
{
	// Get direction
	const glm::vec2& l_stick = g_input.getGamePadStickVec(gamepad::left_stick);
	glm::vec3 dir(l_stick.x, 0.0f, -l_stick.y);
	float length = glm::length(dir);
	
	// Set animation blend parameter
	float right_trigger = g_input.getGamePadTrigger(gamepad::right_trigger);
	float left_trigger = g_input.getGamePadTrigger(gamepad::left_trigger);
	
	if (length == 0.0f)
		length -= left_trigger;

	else if (length >= 0.98f)
		length += right_trigger;

	m_speed += (glm::clamp(length, 0.0f, 2.0f) - m_speed) * 4.0f * g_clock.dt();
	m_blend += (length - m_blend) * 4.0f * g_clock.dt();
	m_anim->set_blend_tree_param(glm::vec2(m_blend, 0.0f));

	// Transform the direction
	if (length > 0.0f)
	{
		dir = glm::inverse(m_cam->get_view_matrix()) * glm::vec4(dir, 0.0f);
		dir.y = 0.0f;
		dir = glm::normalize(dir);

		// Rotate player and compute forward
		orientation(dir);
	}
	
	if (glm::length(m_forward) != 0.0f)
		m_forward = glm::normalize(m_forward) * m_speed;
	else
		m_forward = glm::vec3(0.0f);

	// Move player
	glm::vec3& player_pos = get_owner()->m_local.get_position();

	if (m_speed < 1.2f)
		player_pos += m_forward * 3.0f * g_clock.dt();
	else if (m_speed > 1.0f)
		player_pos += m_forward * 4.0f * g_clock.dt();
}

void player_controller::orientation(const glm::vec3& dir)
{
	// Orientation
	glm::vec3 z_axis(0.0f, 0.0f, 1.0f);

	// Get the angle between the z-axis
	float ang = glm::degrees(glm::acos(glm::dot(dir, z_axis)));
	if (-dir.x > 0.0f)
		ang = -ang;

	// Angle in range [0,360]
	if (ang < 0.0f)
		ang = 360.0f + ang;

	// Bug fix I don't know how to describe
	if (glm::abs(ang - m_angle) > 180.0f)
	{
		if (ang > m_angle)
			ang = -(360.0f - ang);
		else
			ang = 360.0f + ang;
	}

	// Lerp angle
	m_angle = m_angle + (ang - m_angle) * 2.0f * g_clock.dt();

	// Make sure angle is in range [0,360] after lerp
	if (m_angle < 0.0f)
		m_angle = 360.0f;
	else if (m_angle > 360.0f)
		m_angle = 0.0f;

	// Compute a rotation matrix
	glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(m_angle), glm::vec3(0.0f, 1.0f, 0.0f));
	
	// Compute forward vector
	if (m_directional)
		m_forward = rot * glm::vec4(z_axis, 0.0f);

	// Set the orientation
	get_owner()->m_local.set_rotation(glm::quat_cast(rot));
}

void player_controller::targeted_movement()
{
	// Get input
	const glm::vec2& l_stick = g_input.getGamePadStickVec(gamepad::left_stick);
	glm::vec3 stick(l_stick.x, 0.0f, -l_stick.y);
	float length = glm::length(stick);

	update_target_blend();

	// Transform the direction
	if (length > 0.0f)
	{
		stick = glm::inverse(m_cam->get_view_matrix()) * glm::vec4(stick, 0.0f);
		stick.y = 0.0f;
		m_forward = glm::normalize(stick);
		stick = m_forward * length;
	}

	// Player pos
	glm::vec3& player_pos = get_owner()->m_local.get_position();
	m_speed += (glm::abs(length) - m_speed) * 5.0f * g_clock.dt();
	player_pos += m_speed * m_forward * 2.0f * g_clock.dt();

	if (glm::length(player_pos) < 3.0f)
	{
		player_pos = glm::normalize(player_pos) * 3.0f;
	}

	// Orientation
	glm::vec3 dir = glm::normalize(m_target - player_pos);
	orientation(dir);

	// Move camera
	glm::vec3 cam_pos = player_pos - dir * m_radius;
	cam_pos.y += 2.0f;
	m_cam->m_pos += (cam_pos - m_cam->m_pos) * 10.0f * g_clock.dt();
	m_cam->m_view = glm::normalize(m_target - m_cam->m_pos);
}

void player_controller::update_target_blend()
{
	const glm::vec2& l_stick = g_input.getGamePadStickVec(gamepad::left_stick);
	float slope = l_stick.y / l_stick.x;
	glm::vec2 blend_param;

	glm::vec2 max;

	// First quadrant
	if (l_stick.x >= 0.0f && l_stick.y >= 0.0f)
	{
		max.x = 1 / (slope + 1);
		max.y = 1 - max.x;

		blend_param.x = glm::clamp(l_stick.x, 0.0f, max.x);
		blend_param.y = glm::clamp(l_stick.y, 0.0f, max.y);
	}

	// Second quadrant
	else if (l_stick.x < 0.0f && l_stick.y >= 0.0f)
	{
		max.x = 1 / (slope - 1);
		max.y = 1 + max.x;

		blend_param.x = glm::clamp(l_stick.x, max.x, 0.0f);
		blend_param.y = glm::clamp(l_stick.y, 0.0f, max.y);
	}

	// Third quadrant
	else if (l_stick.x < 0.0f && l_stick.y < 0.0f)
	{
		max.x = -1 / (slope + 1);
		max.y = -max.x - 1;

		blend_param.x = glm::clamp(l_stick.x, max.x, 0.0f);
		blend_param.y = glm::clamp(l_stick.y, max.y, 0.0f);
	}

	// Fourth quadrant
	else
	{
		max.x = -1 / (slope - 1);
		max.y = max.x - 1;

		blend_param.x = glm::clamp(l_stick.x, 0.0f, max.x);
		blend_param.y = glm::clamp(l_stick.y, max.y, 0.0f);
	}

	blend_param -= 0.02f * l_stick;
	m_targ_blend += (blend_param - m_targ_blend) * 5.0f * g_clock.dt();
	m_anim->set_blend_tree_param(m_targ_blend);
}
}