/**
* @file debug.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include <glm/glm.hpp>

namespace cs460{
class debug
{
public:
	void debug_draw_aabb(const glm::vec3& min, const glm::vec3& max, glm::vec4 color, bool wireframe = true, bool depth_test = true);
	void debug_draw_line(const glm::vec3& start, const glm::vec3& end, glm::vec4 color, bool depth_test = true);
	void debug_draw_triangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, glm::vec4 color);
	void debug_draw_bone(const glm::vec3& start, const glm::vec3& end, glm::vec4 color, bool wireframe = true);

	static debug& get_instance() { static debug d; return d; }
private:
	debug();
	~debug();

	unsigned int m_cube_vao = 0;
	unsigned int m_cube_vbo = 0;
	unsigned int m_line_vao = 0;
	unsigned int m_line_vbo = 0;
};

#define g_debug debug::get_instance()
}