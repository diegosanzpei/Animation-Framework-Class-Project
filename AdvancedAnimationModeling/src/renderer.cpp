/**
* @file renderer.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "renderer.h"
#include "shader.h"
#include "scene_graph.h"
#include "resources.h"
#include "camera.h"
#include <imgui.h>
#include "mesh_comp.h"
#include "node.h"

namespace cs460{
renderer::renderer()
	: m_shader(nullptr)
	, m_window()
{}

renderer::~renderer()
{
}

/**
 * Initializes GLFW and GLAD
 * Creates a window with the given parameters
 */
void renderer::create(unsigned w, unsigned h, const char* title, bool hidden)
{
	// Initialize glfw and create the main window
	init_glfw(w, h, title, hidden);

	// Initialize glad and setup opengl
	init_opengl(w, h);
	
	// Load shaders and bind the main shader
	load_shaders();
}

/**
 * Terminates GLFW
 */
void renderer::destroy()
{
	destroy_main_shader();
	m_window.destroy();
	glfwTerminate();
}

/**
 * Called by GLFW to notify of an error
 */
void renderer::glfw_error_callback(int error, const char* description)
{
	std::cout << "GLFW Error: " << description << std::endl;
	exit(1);
}

void APIENTRY renderer::opengl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, 
									 GLsizei length, const GLchar* message, const void* userParam)
{
	// ignore non-significant error/warning codes
	if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return; 

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " <<  message << std::endl;

	switch (source)
	{
	    case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	    case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	    case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	    case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	    case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	    case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break; 
	    case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	    case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	    case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	    case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	    case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	    case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	    case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	    case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	    case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	    case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}

/**
 * Loads and creates the main shader of the program
 */
void renderer::create_main_shader()
{
	m_shader = shader::CreateShaderProgram("../resources/shaders/color.vert", "../resources/shaders/color.frag");
}

/**
 * Frees the main shader program
 */
void renderer::destroy_main_shader()
{
	glUseProgram(0);
	delete m_shader;
}

void renderer::init_glfw(unsigned w, unsigned h, const char* title, bool hidden)
{
	// Initialize GLFW
	if (!glfwInit())
	{
		// Initialization failed
		std::cout << "Failed to initialize GLFW" << std::endl;
		exit(1);
	}

	// Specify OpenGL version 4.4
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);

	// Core profile with forward compatibility
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// Set an error callback function for GLFW
	glfwSetErrorCallback(glfw_error_callback);

	// Create the window
	m_window.create(w, h, title, hidden);
}

void renderer::init_opengl(unsigned w, unsigned h)
{
	// Set the current OpenGL context
	glfwMakeContextCurrent(m_window.handle());

	// Enable vsync
	glfwSwapInterval(1);

	// Initialize GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		exit(1);
	}

	// Set an error callback function for OpenGL
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(opengl_debug_callback, 0);

	// Set the frame buffer size
	glViewport(0, 0, w, h);

	// Set up OpenGL
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	// Clear the back buffer for the first frame
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderer::load_shaders()
{
	// Create the shader program
	m_shader = shader::CreateShaderProgram("data/shaders/color.vert", "data/shaders/color.frag");

	// Bind the shader program
	glUseProgram(m_shader->GetHandle());
	m_shader->SetUniform("diffuse", 0);
	m_shader->SetUniform("normal_map", 1);
	m_shader->SetUniform("u_light_color", g_scene.get_light_color());
	m_shader->SetUniform("u_light_dir", g_scene.get_light_dir());
	m_shader->SetUniform("u_ambient", g_scene.get_ambient());
	m_shader->SetUniform("u_specular", g_scene.get_specular());
}

/**
 * Calls the update function of the window
 */
bool renderer::update_window()
{
	return m_window.update();
}

renderer& renderer::get_instance()
{
	static renderer r;
	return r;
}

void renderer::set_world_matrix(const glm::mat4& world_matrix)
{
	// Compute the model view projection matrix
	const glm::mat4& view_proj = g_scene.get_camera().get_world_to_projection();
	glm::mat4 mvp = view_proj * world_matrix;

	// Set the matrix in the shader
	m_shader->SetUniform("u_mvp", mvp);
	m_shader->SetUniform("u_model", world_matrix);
	m_shader->SetUniform("u_cam_pos", g_scene.get_camera().get_pos());
}

void renderer::set_primitve_uniforms(const int model_idx, const primitive& prim)
{
	// Get the material of the primitive
	const material& mat = g_resources.get_material(model_idx, prim.m_material);

	// Set the base color
	m_shader->SetUniform("u_color", mat.m_base_color);

	// Tell the shader if the primitive has normals
	m_shader->SetUniform("u_no_normals", prim.m_no_normals);

	// Set the base texture
	if (mat.m_diffuse >= 0 && m_use_diffuse_textures)
	{
		m_shader->SetUniform("u_use_texture", true);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mat.m_diffuse);
	}
	else
		m_shader->SetUniform("u_use_texture", false);

	// Set the normal map
	if (mat.m_normal >= 0 && prim.m_tangents && m_use_normal_maps)
	{
		m_shader->SetUniform("u_use_normal_map", true);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mat.m_normal);
	}
	else
		m_shader->SetUniform("u_use_normal_map", false);
}

void renderer::skinning(int model_idx, int model_inst, const mesh_comp* m)
{
	// Get the skin index
	int skin_idx = m->get_skin();
	
	// Check if mesh is not skinned
	if (skin_idx < 0)
	{
		m_shader->SetUniform("u_skinned", false);
		return;
	}

	// Get the skin
	const skin& skin = g_resources.get_model_skin(model_idx, skin_idx);

	// Get the world to model of the skin root node
	const node* root_node = g_scene.get_model_node(model_idx, model_inst, m->get_skin_root());
	glm::mat4 w2m_root = root_node->m_local.compute_matrix() * root_node->m_world.compute_inv_matrix();

	// Compute joint matrices
	size_t n_joints = skin.m_joints.size();
	for (size_t i = 0; i < n_joints; ++i)
	{
		// Get the inverse bind matrix of the current joint
		const glm::mat4& inv_bind_mtx = skin.m_inv_bind_mtxs[i];

		// Get the model to world mtx of the current joint
		int node_idx = skin.m_joints[i];
		const node* joint_node = g_scene.get_model_node(model_idx, model_inst, node_idx);
		glm::mat4 m2w_joint = joint_node->m_world.compute_matrix();

		// Joint matrix
		glm::mat4 joint_mtx = w2m_root * m2w_joint * inv_bind_mtx;

		// Send the final joint mtx to the shader
		m_shader->SetUniform("u_joint_matrices[" + std::to_string(i) + "]", joint_mtx);
	}

	// Tell the gpu that the mesh is skinned
	m_shader->SetUniform("u_skinned", true);
}

void renderer::render_mesh(const glm::mat4& world_matrix, int model_idx, int model_inst, const mesh_comp* m)
{
	// Set the m2w matrix
	set_world_matrix(world_matrix);

	// Get the mesh
	const mesh& mesh = g_resources.get_model_mesh(model_idx, m->get_mesh());

	// Compute joint matrices if necessary
	skinning(model_idx, model_inst, m);

	// Render each primitive
	size_t n_primitives = mesh.m_primitives.size();
	for (size_t i = 0; i < n_primitives; ++i)
	{
		const primitive& prim = mesh.m_primitives[i];
		set_primitve_uniforms(model_idx, prim);

		// Draw
		glBindVertexArray(prim.m_vao);

		if (prim.m_no_ebo)
			glDrawArrays(prim.m_render_mode, 0, prim.m_num_vertices);
		else
			glDrawElements(prim.m_render_mode, prim.m_element_count, prim.m_element_type, (void*)prim.m_ebo_offset);
	}

	// Unbind
	glBindVertexArray(0);
}

void renderer::imgui()
{
	bool open = true;
	ImGui::Begin("Render Options", &open, ImGuiWindowFlags_NoMove);
	ImGui::Checkbox("Use Diffuse Textures", &m_use_diffuse_textures);
	ImGui::Checkbox("Use Normal Maps", &m_use_normal_maps);
	ImGui::Text("Render Bounding Volumes");
	ImGui::RadioButton("all bv", &m_render_bv_mode, (int)render_mode::render_all); ImGui::SameLine();
	ImGui::RadioButton("selected bv", &m_render_bv_mode, (int)render_mode::render_selected); ImGui::SameLine();
	ImGui::RadioButton("no bv", &m_render_bv_mode, (int)render_mode::no_render);
	ImGui::Text("Render Skins");
	ImGui::RadioButton("all skins", &m_render_skin_mode, (int)render_mode::render_all); ImGui::SameLine();
	ImGui::RadioButton("selected skin", &m_render_skin_mode, (int)render_mode::render_selected); ImGui::SameLine();
	ImGui::RadioButton("no skins", &m_render_skin_mode, (int)render_mode::no_render);
	ImGui::End();
}
}
