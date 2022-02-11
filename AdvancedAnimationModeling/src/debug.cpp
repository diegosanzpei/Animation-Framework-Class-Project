/**
* @file debug.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include "debug.h"
#include <glad/glad.h>
#include "renderer.h"
#include "shader.h"
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "scene_graph.h"
#include "camera.h"

namespace cs460 {
namespace geometry {
float cube[] = {
	// positions       ords
	// Front face
   -0.5f,  0.5f, 0.5f,  // top left	
   -0.5f, -0.5f, 0.5f,  // bottom left	
	0.5f,  0.5f, 0.5f,  // top right	
	0.5f,  0.5f, 0.5f,  // top right	
   -0.5f, -0.5f, 0.5f,  // bottom left
	0.5f, -0.5f, 0.5f,  // bottom right

	// Back face
	0.5f, -0.5f, -0.5f, // bottom left	
   -0.5f, -0.5f, -0.5f, // bottom right
   -0.5f,  0.5f, -0.5f, // top right	
   -0.5f,  0.5f, -0.5f, // top right	
	0.5f,  0.5f, -0.5f, // top left	
	0.5f, -0.5f, -0.5f, // bottom left	

	// Top face
   -0.5f,  0.5f,  0.5f, // bottom left	
	0.5f,  0.5f,  0.5f, // bottom right
   -0.5f,  0.5f, -0.5f, // top left	
	0.5f,  0.5f, -0.5f, // top right	
   -0.5f,  0.5f, -0.5f, // top left	
	0.5f,  0.5f,  0.5f, // bottom right

	// Bottom face
   -0.5f, -0.5f, -0.5f, // bottom left
	0.5f, -0.5f, -0.5f, // bottom right
   -0.5f, -0.5f,  0.5f, // top left	
	0.5f, -0.5f,  0.5f, // top right	
   -0.5f, -0.5f,  0.5f, // top left	
	0.5f, -0.5f, -0.5f, // bottom right

	// Right face	
	0.5f, -0.5f,  0.5f, // bottom left
	0.5f, -0.5f, -0.5f, // bottom right
	0.5f,  0.5f,  0.5f, // top left	
	0.5f,  0.5f, -0.5f, // top right	
	0.5f,  0.5f,  0.5f, // top left	
	0.5f, -0.5f, -0.5f, // bottom right

	// Left face
   -0.5f, -0.5f, -0.5f, // bottom left  
   -0.5f, -0.5f,  0.5f, // bottom right 
   -0.5f,  0.5f, -0.5f, // top left	 
   -0.5f,  0.5f,  0.5f, // top right	 
   -0.5f,  0.5f, -0.5f, // top left	 
   -0.5f, -0.5f,  0.5f  // bottom right 
};
}
debug::debug()
{
	glGenVertexArrays(1, &m_cube_vao);
	glGenBuffers(1, &m_cube_vbo);
	
	// Send data to GPU
	glBindVertexArray(m_cube_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_cube_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(geometry::cube), geometry::cube, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	// Line vao and vbo
	glGenVertexArrays(1, &m_line_vao);
	glGenBuffers(1, &m_line_vbo);
}

debug::~debug()
{
	glDeleteVertexArrays(1, &m_cube_vao);
	glDeleteBuffers(1, &m_cube_vbo);
	glDeleteVertexArrays(1, &m_line_vao);
	glDeleteBuffers(1, &m_line_vbo);
}

void debug::debug_draw_aabb(const glm::vec3& min, const glm::vec3& max, glm::vec4 color, bool wireframe, bool depth_test)
{
	glBindVertexArray(m_cube_vao);
	shader* s = g_renderer.get_shader();
	s->SetUniform("u_no_normals", true);
	s->SetUniform("u_use_texture", false);
	s->SetUniform("u_color", color);
	s->SetUniform("u_skinned", false);

	// Get position and scale of aabb
	glm::vec3 scale = max - min;
	glm::vec3 pos = min + scale / 2.0f;

	// Compute m2w
	glm::mat4 m2w = glm::translate(glm::mat4(1.0f), pos);
	m2w = glm::scale(m2w, scale);

	// Get the world to projection matrix
	const glm::mat4& w2p = g_scene.get_camera().get_world_to_projection();
	s->SetUniform("u_mvp", w2p * m2w);

	if (depth_test == false)
		glDisable(GL_DEPTH_TEST);

	// Draw
	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_CULL_FACE);
	}

	glDrawArrays(GL_TRIANGLES, 0, sizeof(geometry::cube) / (3 * sizeof(geometry::cube[0])));
	
	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_CULL_FACE);
	}

	if (depth_test == false)
		glEnable(GL_DEPTH_TEST);
}

void debug::debug_draw_line(const glm::vec3& start, const glm::vec3& end, glm::vec4 color, bool depth_test)
{
	// Send data to GPU
	glm::vec3 data[2] = { start, end };
	glBindVertexArray(m_line_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_line_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	// Set shader
	shader* s = g_renderer.get_shader();
	s->SetUniform("u_no_normals", true);
	s->SetUniform("u_use_texture", false);
	s->SetUniform("u_color", color);
	s->SetUniform("u_skinned", false);
	s->SetUniform("u_mvp", g_scene.get_camera().get_world_to_projection());
	
	// Draw
	if (depth_test)
		glDisable(GL_DEPTH_TEST);
	
	glDrawArrays(GL_LINES, 0, 2);
	
	if (depth_test)
		glEnable(GL_DEPTH_TEST);
}

void debug::debug_draw_triangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, glm::vec4 color)
{
	glm::vec3 data[3] = { p0, p1, p2 };

	// Send data to GPU
	glBindVertexArray(m_line_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_line_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	// Set shader
	shader* s = g_renderer.get_shader();
	s->SetUniform("u_no_normals", true);
	s->SetUniform("u_use_texture", false);
	s->SetUniform("u_color", color);
	s->SetUniform("u_skinned", false);
	s->SetUniform("u_mvp", g_scene.get_camera().get_world_to_projection());

	// Draw
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void debug::debug_draw_bone(const glm::vec3& start, const glm::vec3& end, glm::vec4 color, bool wireframe)
{
	glm::vec3 y_axis(0.0f, 1.0f, 0.0f);
	glm::vec3 fwd = glm::normalize(end - start);
	glm::vec3 right = glm::normalize(glm::cross(fwd, y_axis));
	glm::vec3 up = glm::normalize(glm::cross(right, fwd));

	float base_factor = 0.2f * glm::length(end - start);
	glm::vec3 base = start + fwd * base_factor;
	float base_size = base_factor / 2.0f;

	glm::vec3 a = base + up * base_size;
	glm::vec3 b = base - right * base_size - up * base_size;
	glm::vec3 c = base + right * base_size - up * base_size;
	const glm::vec3& d = end;
	const glm::vec3& e = start;

	glm::vec3 data[18] = {
		a, c, d,
		c, b, d,
		a, d, b,
		e, c, a,
		e, b, c,
		e, a, b
	};

	// Send data to GPU
	glBindVertexArray(m_line_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_line_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	// Set shader
	shader* s = g_renderer.get_shader();
	s->SetUniform("u_no_normals", true);
	s->SetUniform("u_use_texture", false);
	s->SetUniform("u_color", color);
	s->SetUniform("u_skinned", false);
	s->SetUniform("u_mvp", g_scene.get_camera().get_world_to_projection());

	// Draw
	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_CULL_FACE);
	}

	glDrawArrays(GL_TRIANGLES, 0, 18);

	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_CULL_FACE);
	}
}
}