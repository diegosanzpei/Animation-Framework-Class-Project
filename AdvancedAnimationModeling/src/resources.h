/**
* @file resources.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include "transform.h"
#include <string>

namespace cs460 {
struct primitive
{
	primitive();
	unsigned int m_vao = 0;	// VAO handle
	int m_num_vertices = 0;	// Number of vertices that form the primitive
	int m_render_mode = 4;	// Render model (GL_POINTS, GL_TRIANGLES...)

	unsigned int m_element_type = 0;  // Type of indices (BYTE, SHORT...)
	unsigned int m_element_count = 0; // Number of indices
	unsigned int m_ebo_offset = 0;    // Indices offset in the ebo
	bool m_no_ebo = false;            // Primitive uses indices

	int m_material = -1;	   // Material index
	bool m_tangents = false;   // Only use normal maps if tangents are provided
	bool m_no_normals = false; // Only apply lighting if normals are provided

	void set_indices(unsigned int ebo, int type, int count, int offset);
	void set_attribute_pointer(unsigned int vbo, int attrib_idx, int size, int type, int stride, int offset);
};

struct mesh
{
	primitive& new_primitive();
	void update_min_max_vertices(const glm::vec3& min, const glm::vec3& max);
	std::vector<primitive> m_primitives;

	// Mesh bounding volume
	glm::vec3 m_min_vertex = glm::vec3(FLT_MAX);
	glm::vec3 m_max_vertex = glm::vec3(FLT_MIN);
	std::string m_name;
};

struct material
{
	glm::vec4 m_base_color = glm::vec4(1.0f);
	int m_diffuse = -1;
	int m_normal = -1;
};

struct node_rsc
{
	std::string m_name;

	int m_mesh = -1;
	int m_skin = -1;
	int m_skin_root = -1;

	transform m_local;
	std::vector<int> m_childs;
};

struct skin
{
	std::string m_name;
	std::vector<glm::mat4> m_inv_bind_mtxs;
	std::vector<int> m_joints;
};

struct animation
{
	struct sampler
	{
		enum class lerp_mode { linear, step, cubic };
		lerp_mode m_lerp_mode;
		std::vector<float> m_input;
		std::vector<float> m_output;
		float lerp(float t, int* start, int* end) const;
	};

	struct channel
	{
		int m_node;
		enum class path_type { translation, rotation, scale, weights };
		path_type m_path_type;
		int m_sampler;

		glm::vec3 lerp_pos(float t, const sampler& sp) const;
		glm::quat lerp_rot(float t, const sampler& sp, bool normalize) const;
	};

	std::vector<channel> m_chanels;
	std::vector<sampler> m_samplers;
	float m_max_time = 0.0f;
	std::string m_name;
};

struct model_rsc
{
	model_rsc();

	mesh& new_mesh();
	skin& new_skin();
	animation& new_anim();

	bool get_buffer(int idx, unsigned int* buffer_handle);
	bool get_texture(int idx, unsigned int* texture_handle);
	material& get_material(int idx, bool* created = nullptr);

	std::vector<int> m_root_nodes;
	std::unordered_map<int, node_rsc> m_nodes;
	
	std::vector<animation> m_anims; // Animations used by the model
	std::vector<mesh> m_meshes;		// Meshes used by the model
	std::vector<skin> m_skins;		// Skins used by the model

	std::unordered_map<int, unsigned int> m_textures; // Handles of the textures
	std::unordered_map<int, unsigned int> m_buffers;  // Handles of the vbos and ebos
	std::unordered_map<int, material> m_materials;	// Materials used by the model
};

class resources
{
public:
	resources() {}
	static resources& get_instance();
	int new_model(const char* model_name);

	// Returns a reference to the resources of the model
	model_rsc& get_model_rsc(unsigned int idx) { return m_models[idx]; }

	// Returns a reference to the specified material of the given model
	const material& get_material(int model_idx, int mat_idx) { 
		return m_models[model_idx].m_materials[mat_idx]; 
	}

	// Save the transform of the specified node of the given model
	void record_node(int model_idx, int node_idx, const node_rsc& n) {
		m_models[model_idx].m_nodes[node_idx] = n;
	}

	// Returns a reference to the original transform of the specified node
	const transform& get_original_transform(int model_idx, int node_idx) {
		return m_models[model_idx].m_nodes[node_idx].m_local;
	}

	// Returns a reference to the specified mesh of the given model
	const mesh& get_model_mesh(int model_idx, int mesh_idx) {
		return m_models[model_idx].m_meshes[mesh_idx];
	}

	// Returns a reference to the specified skin of the given model
	const skin& get_model_skin(int model_idx, int skin_idx) {
		return m_models[model_idx].m_skins[skin_idx];
	}
	
	// Frees all models
	void destroy();

	// Show loaded model resources
	void imgui() const;

	// Returns true if model is already loaded
	bool model_registered(const char* model_name) {
		return m_model_registry.find(model_name) != m_model_registry.end();
	}

	int get_model_id(const std::string& name) {
		return m_model_registry.at(name);
	}

private:
	std::vector<model_rsc> m_models;
	std::unordered_map<std::string, int> m_model_registry;
	void destroy_rsc(model_rsc& rsc);
	void destroy_mesh(mesh& mesh);
};
#define g_resources resources::get_instance()
}