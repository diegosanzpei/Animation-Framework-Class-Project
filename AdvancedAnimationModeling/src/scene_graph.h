/**
* @file scene_graph.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include <glm/glm.hpp>
#include "resources.h"
#include <unordered_map>
#include <vector>

namespace cs460 {
class component;
class camera;
struct node;
class transform;

typedef int model_id;
typedef int node_id;
typedef std::unordered_map<model_id, std::vector<std::unordered_map<node_id, node*>>> node_registry;

class scene_graph
{
public:
	static scene_graph& get_instance();
	
	// Returns the root node of the scene
	node* get_root() { return m_root; }
	
	// Returns a reference to the main camera of the scene
	camera& get_camera();
	
	// Updates all nodes
	void update();

	// Renders all nodes that reference a mesh
	void render();

	// Frees all nodes
	void destroy() { destroy_rec(m_root); }
	
	// World settings gui window
	void imgui();
	
	// Light parameter getters
	float get_ambient() { return m_ambient; }
	float get_specular() { return m_specular; }
	const glm::vec3& get_light_dir() { return m_light_dir; }
	const glm::vec3& get_light_color() { return m_light_color; }
	
	// Render the bounding volumes of all nodes that have a mesh component
	void render_bvs();
	void render_skins();

	// Creates an instance of a model
	node* create_model_instance(const int model_id);

	node* get_model_node(const int model_idx, const int instance_idx, const int node_idx);

	enum class scene_type { 
		curves, skinned_models, animation, 
		anim_blending_1d, directional_movement, anim_blending_2d, targeted_movement,
		analytic_2_bone_ik, cyclic_coord_descent, fabrik, ik_demo
	};
	scene_type get_scene() { return m_scene; };

	enum class curve_type { linear, hermite, catmull_rom, bezier };
	void create_curve(const curve_type type);
	
	// Frees all nodes except for the root node and the camera
	void clear_scene();

	void render_grid(bool render) { m_render_grid = render; }

	// Updates the world transform of all nodes
	void update_node_transforms();

private:
	scene_graph();
	~scene_graph();

	// Frees all nodes that are below the given node in the hierarchy
	void destroy_rec(node* node);

	// Reset the transforms of all nodes in the scene
	void reset_scene();
	void reset_scene_rec(node* node);

	// Renders the bounding volume of all nodes that reference a mesh
	void render_bvs_rec(node* node);
	void render_skins_rec(node* node);

	// Updates the components of all nodes
	void update_nodes();
	void update_nodes_rec(node* node);

	// Updates the world transform of all nodes
	void update_node_transforms_rec(node* node, const transform& parent_world);

	// Renders all nodes that reference a mesh
	void render_rec(node* node);

	void create_model_instance_rec(std::unordered_map<node_id, node*>& node_reg, const int inst_id, const int model, node* parent, const std::vector<int>& child_idxs);

	node* m_root = nullptr;
	node* m_camera_node = nullptr;
	
	// Lighting parameters of the scene
	glm::vec3 m_light_color = glm::vec3(1.0f);
	glm::vec3 m_light_dir = glm::vec3(0.0f, 0.0f, -1.0f);
	float m_yaw_angle = 0.0f;
	float m_pitch_angle = 0.0f;
	float m_ambient = 0.1f;
	float m_specular = 0.5f;

	bool m_render_grid = true;

	// Updates the direction of the light
	glm::vec3 rotate_light();
	
	node_registry m_node_registry;

	scene_type m_scene = scene_type::curves;

	// Change the scene
	void change_scene(const scene_type st);

	int m_curve_id = 0;
};

#define g_scene scene_graph::get_instance()
}