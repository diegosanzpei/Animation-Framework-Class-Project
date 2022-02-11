/**
* @file renderer.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include <glad/glad.h>
#include "window.h"
#include <glm/glm.hpp>

namespace cs460 {
class shader;
struct primitive;
struct mesh_comp;
class renderer {
public:
	static renderer& get_instance();

	// Initializes glfw, glad
	void create(unsigned w, unsigned h, const char* title, bool hidden);
	
	// Frees shaders, window and terminate glfw
	void destroy();
	
	// Error callback for GLFW
	static void glfw_error_callback(int error, const char* description);
	
	// Error callback for OpenGL
	static void APIENTRY opengl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
		GLsizei length, const GLchar* message, const void* userParam);
	
	// Update the window
	bool update_window();

	// Returns the main and only window
	const window& get_window() { return m_window; };

	void render_mesh(const glm::mat4& world_matrix, int model_idx, int model_inst, const mesh_comp* m);

	shader* get_shader() { return m_shader; }

	void imgui();

	enum class render_mode { render_all, render_selected, no_render };
	render_mode bv_rendering_mode() { return render_mode(m_render_bv_mode); }
	render_mode skin_rendering_mode() { return render_mode(m_render_skin_mode); }

private:
	renderer(const renderer& rhs) = delete;
	renderer& operator=(const renderer& rhs) = delete;
	renderer();
	~renderer();

	// Main window of the program
	window m_window;
	
	// Creates the main shader program
	void create_main_shader();
	
	// Frees the main shader program
	void destroy_main_shader();

	void init_glfw(unsigned w, unsigned h, const char* title, bool hidden);
	void init_opengl(unsigned w, unsigned h);
	void load_shaders();
	
	void set_world_matrix(const glm::mat4& world_matrix);
	void set_primitve_uniforms(const int model_idx, const primitive& prim);
	void skinning(int model_idx, int model_inst, const mesh_comp* m);
	
	// Main shader program
	shader* m_shader;

	bool m_use_normal_maps = true;
	bool m_use_diffuse_textures = true;
	int m_render_skin_mode = (int)render_mode::no_render;
	int m_render_bv_mode = (int)render_mode::render_selected;
};

#define g_renderer renderer::get_instance()
}
