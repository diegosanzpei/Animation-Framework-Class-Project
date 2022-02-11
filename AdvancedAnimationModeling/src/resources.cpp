/**
* @file resources.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include "resources.h"
#include <glad/glad.h>
#include <imgui.h>
#include "scene_graph.h"
#include <iostream>

namespace cs460 {
resources& resources::get_instance()
{
	static resources rsc;
	return rsc;
}

int resources::new_model(const char* model_name)
{
	// Check if model has already been created
	auto it = m_model_registry.find(model_name);
	if (it != m_model_registry.end())
		return -1;

	// Otherwise, create the resource
	m_models.emplace_back();
	int model_id = (int)m_models.size() - 1;

	// Register the model
	m_model_registry[model_name] = model_id;

	// Return the resource id for the new model
	return model_id;
}

void resources::destroy()
{
	// Unbind
	glBindVertexArray(0);
	
	size_t n_models = m_models.size();
	for (size_t i = 0; i < n_models; ++i)
		destroy_rsc(m_models[i]);

	m_models.clear();
}

void resources::destroy_rsc(model_rsc& rsc)
{
	// Delete buffers
	auto buff_end = rsc.m_buffers.end();
	for (auto it = rsc.m_buffers.begin(); it != buff_end; ++it)
		glDeleteBuffers(1, &it->second);

	// Delete textures
	auto text_end = rsc.m_textures.end();
	for (auto it = rsc.m_textures.begin(); it != text_end; ++it)
		glDeleteTextures(1, &it->second);

	// Destroy meshes
	size_t n_meshes = rsc.m_meshes.size();
	for (size_t i = 0; i < n_meshes; ++i)
		destroy_mesh(rsc.m_meshes[i]);
}

void resources::destroy_mesh(mesh& mesh)
{
	// Destroy all primitives
	size_t n_primitives = mesh.m_primitives.size();
	for (size_t i = 0; i < n_primitives; ++i)
		glDeleteVertexArrays(1, &mesh.m_primitives[i].m_vao);
}

void resources::imgui() const
{
	bool open = true;
	ImGui::Begin("Model Resources", &open, ImGuiWindowFlags_NoMove);

	auto end = m_model_registry.end();
	for (auto it = m_model_registry.begin(); it != end; ++it)
	{
		if (ImGui::Button(it->first.c_str()))
			g_scene.create_model_instance(it->second);
	}

	ImGui::End();
}

model_rsc::model_rsc()
{
}

mesh& model_rsc::new_mesh()
{
	m_meshes.emplace_back();
	return m_meshes.back();
}

skin& model_rsc::new_skin()
{
	m_skins.emplace_back();
	return m_skins.back();
}

animation& model_rsc::new_anim()
{
	m_anims.emplace_back();
	return m_anims.back();
}

bool model_rsc::get_buffer(int idx, unsigned int* buffer_handle)
{
	auto it = m_buffers.find(idx);

	// Buffer not created yet
	if (it == m_buffers.end())
	{
		glGenBuffers(1, buffer_handle);
		m_buffers[idx] = *buffer_handle;
		return false;
	}

	// Buffer already created
	else
	{
		*buffer_handle = m_buffers[idx];
		return true;
	}
}

bool model_rsc::get_texture(int idx, unsigned int* texture_handle)
{
	auto it = m_textures.find(idx);

	// Texture not created yet
	if (it == m_textures.end())
	{
		glGenTextures(1, texture_handle);
		m_textures[idx] = *texture_handle;
		return false;
	}

	// Texture already created
	else
	{
		*texture_handle = m_textures[idx];
		return true;
	}
}

material& model_rsc::get_material(int idx, bool* created)
{
	auto it = m_materials.find(idx);

	// Material not created yet
	if (it == m_materials.end())
	{
		if (created)
			*created = false;
		return m_materials[idx];
	}

	// Material already created
	else
	{
		if (created)
			*created = true;
		return it->second;
	}
}

primitive& mesh::new_primitive()
{
	m_primitives.emplace_back();
	return m_primitives.back();
}

void mesh::update_min_max_vertices(const glm::vec3& min, const glm::vec3& max)
{
	for (int i = 0; i < 3; ++i)
	{
		if (min[i] < m_min_vertex[i])
			m_min_vertex[i] = min[i];
		if (max[i] > m_max_vertex[i])
			m_max_vertex[i] = max[i];
	}
}

primitive::primitive()
{
	// Create the vao
	glGenVertexArrays(1, &m_vao);
}

void primitive::set_indices(unsigned int ebo, int type, int count, int offset)
{
	// Bind
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	
	// Unbind
	glBindVertexArray(0);

	// Set element buffer data
	m_element_type = type;
	m_element_count = count;
	m_ebo_offset = offset;
}

void primitive::set_attribute_pointer(unsigned int vbo, int attrib_idx, int size, int type, int stride, int offset)
{
	// Bind
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glVertexAttribPointer(attrib_idx, size, type, GL_FALSE, stride, (void*)offset);
	glEnableVertexAttribArray(attrib_idx);

	// Unbind
	glBindVertexArray(0);
}

glm::vec3 animation::channel::lerp_pos(float t, const sampler& sp) const
{
	// Normalized t
	int start, end;
	float tn = sp.lerp(t, &start, &end);

	if (tn < 0)
		return glm::vec3(0.0f);

	// Get the segment
	const unsigned char* data = reinterpret_cast<const unsigned char*>(
		sp.m_output.data());
	const glm::vec3* p0 = reinterpret_cast<const glm::vec3*>(
		data + start * sizeof(glm::vec3));
	const glm::vec3* p1 = reinterpret_cast<const glm::vec3*>(
		data + end * sizeof(glm::vec3));

	// Linear interpolation
	if (sp.m_lerp_mode == sampler::lerp_mode::linear)
		return *p0 + (*p1 - *p0) * tn;

	// Step
	else if (sp.m_lerp_mode == sampler::lerp_mode::step)
		return *p0;

	else
	{
		std::cout << "cubic not supported" << std::endl;
		return glm::vec3(0.0f);
	}
}

glm::quat animation::channel::lerp_rot(float t, const sampler& sp, bool normalize) const
{
	// Normalized t
	int start, end;
	float tn = sp.lerp(t, &start, &end);

	if (tn < 0)
		return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	// Get the segment
	const unsigned char* data = reinterpret_cast<const unsigned char*>(
		sp.m_output.data());
	const glm::quat* p0 = reinterpret_cast<const glm::quat*>(
		data + start * sizeof(glm::quat));
	const glm::quat* p1 = reinterpret_cast<const glm::quat*>(
		data + end * sizeof(glm::quat));

	// Linear interpolation
	if (sp.m_lerp_mode == sampler::lerp_mode::linear)
	{
		glm::quat res = glm::slerp(*p0, *p1, tn);

		// Nlerp
		if (normalize)
			return glm::normalize(res);

		// Slerp
		else
			return res;
	}

	// Step
	else if (sp.m_lerp_mode == sampler::lerp_mode::step)
		return *p0;

	else
	{
		std::cout << "cubic not supported" << std::endl;
		return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	}
}

float animation::sampler::lerp(float t, int* start, int* end) const
{
	// Get the number of points
	size_t n_points = m_input.size();
	if (n_points == 0)
		return -1;

	// Cap lerp output
	if (t < m_input[0])
	{
		*start = 0;
		*end = 0;
		return 0.0f;
	}
	else if (t > m_input[n_points - 1])
	{
		*start = (int)n_points - 1;
		*end = *start;
		return 0.0f;
	}

	int frame_idx = 1;
	while (t > m_input[frame_idx])
		++frame_idx;

	// Get the segment
	*start = frame_idx - 1;
	*end = frame_idx;

	// Normalize the t parameter
	float interval = m_input[frame_idx] - m_input[frame_idx - 1];
	float local_time = t - m_input[frame_idx - 1];
	float tn = local_time / interval;

	// Linear interpolation
	return tn;
}
}
