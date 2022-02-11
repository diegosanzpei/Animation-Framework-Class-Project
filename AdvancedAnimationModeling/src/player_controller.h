#pragma once
#include "component.h"
#include <glm/glm.hpp>

namespace cs460 {
class camera;
struct anim_comp;
class player_controller : public component
{
public:
	virtual void initialize();
	virtual void update();
	virtual void destroy();

	void set_directional_movement(bool directional) { m_directional = directional; }

private:
	void move_camera();
	void move_player();
	void orientation(const glm::vec3& dir);
	void targeted_movement();
	void update_target_blend();

	camera* m_cam = nullptr;
	anim_comp* m_anim = nullptr;
	glm::vec3 m_target = glm::vec3(0.0f, 1.0f, 0.0f);
	float m_alpha = 180.0f;
	float m_beta = 25.0f;
	float m_radius = 4.0f;
	float m_angle = 0.0f;
	float m_blend = 0.0f;
	glm::vec2 m_targ_blend = glm::vec2(0.0f);
	float m_speed = 0.0f;
	bool m_directional = true;
	glm::vec3 m_forward = glm::vec3(0.0f, 0.0f, 1.0f);
};
}