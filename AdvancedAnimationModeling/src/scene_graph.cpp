/**
* @file scene_graph.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include "scene_graph.h"
#include "renderer.h"
#include "camera.h"
#include "component.h"
#include "input.h"
#include "resources.h"
#include "node.h"
#include "mesh_comp.h"
#include <imgui.h>
#include "shader.h"
#include <glm/gtx/rotate_vector.hpp>
#include "editor.h"
#include "curves.h"
#include "loader.h"
#include "anim_comp.h"
#include "player_controller.h"
#include "2_bone_ik.h"
#include "inverse_kinematics.h"
#include "curve_node_comp.h"

namespace cs460 {
scene_graph& scene_graph::get_instance()
{
	static scene_graph sg;
	return sg;
}

// Returns a reference to the main camera
camera& scene_graph::get_camera()
{
	return *m_camera_node->get_component<camera>();
}

// Update node transforms
void scene_graph::update()
{
	if (m_render_grid)
		g_editor.render_grid();

	// Check if user wants to reset the scene
	if (g_input.keyIsDown(keyboard::key_left_control)) {
		
		// Change scene
		if (g_input.keyIsPressed(keyboard::key_1))
			change_scene(scene_type::curves);
		
		else if (g_input.keyIsPressed(keyboard::key_2))
			change_scene(scene_type::animation);

		// Clear the scene graph
		if (g_input.keyIsDown(keyboard::key_left_shift)) {
			if (g_input.keyIsPressed(keyboard::key_r)) {
				clear_scene();
			}
		}

		else if (g_input.keyIsPressed(keyboard::key_3))
			change_scene(scene_type::anim_blending_1d);
		
		else if (g_input.keyIsPressed(keyboard::key_4))
			change_scene(scene_type::directional_movement);
		
		else if (g_input.keyIsPressed(keyboard::key_5))
			change_scene(scene_type::anim_blending_2d);
		
		else if (g_input.keyIsPressed(keyboard::key_6))
			change_scene(scene_type::targeted_movement);

		else if (g_input.keyIsPressed(keyboard::key_7))
			change_scene(scene_type::analytic_2_bone_ik);

		else if (g_input.keyIsPressed(keyboard::key_8))
			change_scene(scene_type::cyclic_coord_descent);

		else if (g_input.keyIsPressed(keyboard::key_9))
			change_scene(scene_type::fabrik);

		else if (g_input.keyIsPressed(keyboard::key_0))
			change_scene(scene_type::ik_demo);

		// Reset transforms of all nodes
		else if (g_input.keyIsPressed(keyboard::key_r))
		{
			//reset_scene();
			change_scene(m_scene);
		}
	}

	// Update nodes
	update_nodes();

	// Update world transforms of all nodes
	update_node_transforms();
}

// Render all nodes that reference a mesh
void scene_graph::render()
{
	if (m_root)
		render_rec(m_root);
}

void scene_graph::imgui()
{
	bool open = true;
	ImGui::Begin("World Settings", &open, ImGuiWindowFlags_NoMove);

	ImGui::Text("Light Color");
	if (ImGui::ColorEdit4("color", &m_light_color[0]))
		g_renderer.get_shader()->SetUniform("u_light_color", m_light_color);
	
	ImGui::Separator();
	ImGui::Text("Ambient Light");
	if (ImGui::SliderFloat("ambient", &m_ambient, 0.0f, 5.0f))
		g_renderer.get_shader()->SetUniform("u_ambient", m_ambient);
	
	ImGui::Text("Specular Light");
	if (ImGui::SliderFloat("specular", &m_specular, 0.0f, 5.0f))
		g_renderer.get_shader()->SetUniform("u_specular", m_specular);

	ImGui::Separator();
	ImGui::Text("Light Direction");
	if (ImGui::SliderFloat("yaw", &m_yaw_angle, 0.0f, 360.0f))
		g_renderer.get_shader()->SetUniform("u_light_dir", rotate_light());

	if (ImGui::SliderFloat("pitch", &m_pitch_angle, -89.0f, 89.0f))
		g_renderer.get_shader()->SetUniform("u_light_dir", rotate_light());

	ImGui::End();
}

void scene_graph::render_bvs()
{
	render_bvs_rec(m_root);
}

void scene_graph::render_skins()
{
	render_skins_rec(m_root);
}

// Delete all nodes that are below the given node in the hierarchy
void scene_graph::destroy_rec(node* node)
{
	// Delete node children first
	size_t n_childs = node->m_children.size();
	for (size_t i = 0; i < n_childs; ++i)
		destroy_rec(node->m_children[i]);

	// Delete node and it's components
	delete node;
}

void scene_graph::clear_scene()
{
	size_t n_childs = m_root->m_children.size();
	for (size_t i = 0; i < n_childs; ++i) {
		if (m_root->m_children[i] != m_camera_node)
			destroy_rec(m_root->m_children[i]);
	}

	m_root->m_children.clear();
	m_root->add_child(m_camera_node);
	m_node_registry.clear();
	g_editor.remove_selection();
	m_curve_id = 0;
}

scene_graph::~scene_graph()
{
}

void scene_graph::create_curve(const curve_type type)
{
	// Create the curve
	node* c = new node;

	// Curve id
	std::string id = std::to_string(m_curve_id);

	glm::vec3 curve_points[7] = { 
		{-0.25f, 0.0f, 0.0f}, {-0.33f, 0.0f, -0.82f}, {2.0f, 0.0f, -3.66f},
	    {2.5f, 0.0f, -0.86f}, {2.88f, 0.0f, 1.62f  }, {0.52f, 0.0f, 2.20f }, 
		{9.33f, 0.0f, 2.21f} 
	};

	// Create follower
	std::string model_name = "data/assets/rigged figure/CesiumMan.gltf";
	import_gltf_file(model_name.c_str());
	node* follower = create_model_instance(g_resources.get_model_id(model_name));

	if (type == curve_type::linear)
	{
		c->m_local.set_position(glm::vec3(-4.0f, 0.0f, 0.0f));
		c->m_name = "linear curve " + id;
		linear_curve* comp = c->add_component<linear_curve>();
		comp->set_curve_id(m_curve_id);
		comp->set_follower(follower);
		
		// Add default points
		for (int i = 0; i < 7; ++i)
			comp->add_point(curve_points[i]);
	}

	else if (type == curve_type::hermite)
	{
		c->m_local.set_position(glm::vec3(-4.0f, 0.0f, 0.0f));
		c->m_name = "hermite curve " + id;
		hermite_curve* comp = c->add_component<hermite_curve>();
		comp->set_curve_id(m_curve_id);
		comp->set_follower(follower);

		// Add default points
		comp->add_point(curve_points[0], curve_points[1] - curve_points[0], glm::vec3(0.0f));
		comp->add_point(curve_points[3], curve_points[1] - curve_points[0], glm::vec3(0.5f, 0.0f, -2.0f));
		comp->add_point(curve_points[6], 5.0f * glm::vec3(0.5f, 0.0f, -2.0f), glm::vec3(1.0f));
	}

	else if (type == curve_type::catmull_rom)
	{
		c->m_local.set_position(glm::vec3(-4.0f, 0.0f, 0.0f));
		c->m_name = "catmull-rom curve " + id;
		catmull_rom_curve* comp = c->add_component<catmull_rom_curve>();
		comp->set_curve_id(m_curve_id);
		comp->set_follower(follower);

		// Add default points
		comp->add_point(curve_points[0]);
		comp->add_point(curve_points[2]);
		comp->add_point(curve_points[4]);
		comp->add_point(curve_points[6]);
	}

	else if (type == curve_type::bezier)
	{
		c->m_local.set_position(glm::vec3(-4.0f, 0.0f, 0.0f));
		c->m_name = "bezier curve " + id;
		bezier_curve* comp = c->add_component<bezier_curve>();
		comp->set_curve_id(m_curve_id);
		comp->set_follower(follower);

		// Add default points
		comp->add_point(curve_points[0], curve_points[1] - curve_points[0], glm::vec3(0.0f));
		comp->add_point(curve_points[3], curve_points[1] - curve_points[0], curve_points[2] - curve_points[3]);
		comp->add_point(curve_points[6], curve_points[4] - curve_points[3], curve_points[5] - curve_points[6]);
		comp->add_point(curve_points[6] + glm::vec3(2.0f, 0.0f, 5.0f), glm::vec3(5.0f, 0.0f, 0.0f), curve_points[5] - curve_points[6]);
	}

	m_root->add_child(c);
	++m_curve_id;
}

// Creates a root node and a camera in the scene by default
scene_graph::scene_graph()
	: m_root(new node)
{
	m_root->m_name = "Root Node";

	// Create a node with a camera component
	node* cam = new node;
	cam->m_name = "camera";
	cam->add_component<camera>();
	m_camera_node = cam;
	
	// Make the camera node a child of the root node
	m_root->add_child(cam);

	change_scene(scene_type::curves);
}

// Resets the transform of all nodes in the scene
void scene_graph::reset_scene()
{
	// Reset the root node transform
	m_root->m_local = transform();

	// Reset children
	size_t n_children = m_root->m_children.size();
	for (size_t i = 0; i < n_children; ++i)
		reset_scene_rec(m_root->m_children[i]);
}

void scene_graph::reset_scene_rec(node* node)
{
	// Check if the node is from a model
	if (node->m_node_idx >= 0)
		node->m_local = g_resources.get_original_transform(node->m_model, node->m_node_idx);
	else if (node->m_comps.empty())
		node->m_local = transform();

	size_t n_children = node->m_children.size();
	for (size_t i = 0; i < n_children; ++i)
		reset_scene_rec(node->m_children[i]);
}

void scene_graph::render_bvs_rec(node* node)
{
	// Render children
	size_t n_children = node->m_children.size();
	for (size_t i = 0; i < n_children; ++i)
		render_bvs_rec(node->m_children[i]);

	// Render the node's bv
	if (mesh_comp* m = node->get_component<mesh_comp>())
		m->render_vb();
}

void scene_graph::render_skins_rec(node* node)
{
	// Render skin of the node
	if (mesh_comp* m = node->get_component<mesh_comp>())
	{
		int skin_idx = m->get_skin();
		if (skin_idx >= 0)
			m->render_skin();
	}

	// Render children
	size_t n_children = node->m_children.size();
	for (size_t i = 0; i < n_children; ++i)
		render_skins_rec(node->m_children[i]);
}

void scene_graph::update_nodes()
{
	update_nodes_rec(m_root);
}

void scene_graph::update_nodes_rec(node* node)
{
	// Update components
	node->update();

	// Update each child
	size_t n_childs = node->m_children.size();
	for (size_t i = 0; i < n_childs; ++i)
		update_nodes_rec(node->m_children[i]);
}

// Recursive function. Concatenates the transforms of the nodes in the scene
void scene_graph::update_node_transforms_rec(node* node, const transform& parent_world)
{
	// Concatenate node's world transform
	node->m_world = parent_world.concatenate(node->m_local);

	// Update each child
	size_t n_childs = node->m_children.size();
	for (size_t i = 0; i < n_childs; ++i)
		update_node_transforms_rec(node->m_children[i], node->m_world);
}

void scene_graph::update_node_transforms()
{
	update_node_transforms_rec(m_root, transform());
}

// Recursive function. Sends the nodes that reference a mesh to the renderer
void scene_graph::render_rec(node* node)
{
	// Render the node
	if (mesh_comp* m = node->get_component<mesh_comp>())
	{
		// Draw the mesh
		g_renderer.render_mesh(node->m_world.compute_matrix(), node->m_model, node->m_model_inst, m);
	}

	// Render the childs
	size_t n_childs = node->m_children.size();
	for (size_t i = 0; i < n_childs; ++i)
		render_rec(node->m_children[i]);
}

void scene_graph::create_model_instance_rec(std::unordered_map<node_id, node*>& node_reg, const int inst_id, const int model_id, node* parent, const std::vector<int>& child_idxs)
{
	// Get the child nodes
	const auto& child_nodes = g_resources.get_model_rsc(model_id).m_nodes;

	size_t n_childs = child_idxs.size();
	for (int i = 0; i < n_childs; ++i)
	{
		// Get the current child node data
		const node_rsc& data = child_nodes.at(child_idxs[i]);

		// Create the child
		node* child = new node;

		// Register the child
		int child_idx = child_idxs[i];
		node_reg[child_idx] = child;

		// Set the data
		child->m_name = std::to_string(model_id) + std::to_string(inst_id) + std::to_string(child_idx);
		if (!data.m_name.empty())
			child->m_name += "_" + data.m_name;

		child->m_model = model_id;
		child->m_node_idx = child_idx;
		child->m_model_inst = inst_id;
		child->m_local = data.m_local;

		if (data.m_mesh >= 0)
			child->add_component<mesh_comp>()->set_mesh(data.m_mesh, data.m_skin, data.m_skin_root);

		// Add the child to the parent
		parent->add_child(child);

		create_model_instance_rec(node_reg, inst_id, model_id, child, data.m_childs);
	}
}

node* scene_graph::create_model_instance(const int model_id)
{
	// Get the instances of the model
	auto& model_instances = m_node_registry[model_id];

	// Get the new instance id
	int inst_id = (int)model_instances.size();

	// Create the instance of the model in the registry
	model_instances.emplace_back();
	
	// Create the root node
	node* instance_root = new node;
	instance_root->m_model = model_id;
	instance_root->m_model_inst = inst_id;
	instance_root->m_name = "Model " + std::to_string(model_id) + ", Inst " + std::to_string(inst_id) + " Root Node";
	
	// Get the model
	const model_rsc& model = g_resources.get_model_rsc(model_id);

	// Add an animation comp to the root node
	if (model.m_anims.empty() == false)
		instance_root->add_component<anim_comp>()->set_animation(0);

	m_root->add_child(instance_root);

	create_model_instance_rec(model_instances[inst_id], inst_id, model_id, instance_root, model.m_root_nodes);

	return instance_root;
}

node* scene_graph::get_model_node(const int model_idx, const int instance_idx, const int node_idx)
{
	return m_node_registry.at(model_idx).at(instance_idx).at(node_idx);
}

glm::vec3 scene_graph::rotate_light()
{
	glm::vec3 temp = glm::rotate(m_light_dir, glm::radians(m_pitch_angle), glm::vec3(1.0f, 0.0f, 0.0f));
	return glm::rotate(temp, glm::radians(m_yaw_angle), glm::vec3(0.0f, 1.0f, 0.0f));
}

void scene_graph::change_scene(const scene_type st)
{
	clear_scene();

	render_grid(true);
	get_camera().camera_has_control(true);
	get_camera().reset_camera();
	g_editor.set_picking(true);
	g_editor.set_guizmo(true);

	m_scene = st;

	if (st == scene_type::curves)
	{
		create_curve(curve_type::bezier);
	}

	else if (st == scene_type::skinned_models)
	{
		std::string model_name = "data/assets/rigged figure/CesiumMan.gltf";
		import_gltf_file(model_name.c_str());
		create_model_instance(g_resources.get_model_id(model_name));
	}

	else if (st == scene_type::animation)
	{
		std::string cessium = "data/assets/rigged figure/CesiumMan.gltf";
		std::string brain = "data/assets/BrainStem/BrainStem.gltf";
		std::string fox = "data/assets/Fox/Fox.gltf";
		
		import_gltf_file(cessium.c_str());
		import_gltf_file(brain.c_str());
		import_gltf_file(fox.c_str());
		
		create_model_instance(g_resources.get_model_id(cessium));
		node* f = create_model_instance(g_resources.get_model_id(fox));
		node* b = create_model_instance(g_resources.get_model_id(brain));

		f->m_local.set_scale(glm::vec3(0.01f));
		f->m_local.set_position(glm::vec3(1.0f, 0.0f, 0.0f));
		b->m_local.set_position(glm::vec3(2.0f, 0.0f, 0.0f));
	}

	else if (st == scene_type::anim_blending_1d)
	{
		std::string bot = "data/assets/MIXAMO/xbot.gltf";
		import_gltf_file(bot.c_str());
		node* b = create_model_instance(g_resources.get_model_id(bot));
		
		g_editor.set_selected_node(b);

		// Create the 1d blend tree
		anim_comp* ac = b->get_component<anim_comp>();
		ac->set_1d_blend_tree();

		// Insert blend nodes
		ac->insert_blend_node(5, glm::vec2(0.0f));
		ac->insert_blend_node(22, glm::vec2(1.0f, 0.0f));
		ac->insert_blend_node(6, glm::vec2(2.0f, 0.0f));
		ac->insert_blend_node(15, glm::vec2(3.0f, 0.0f));
		ac->insert_blend_node(3, glm::vec2(4.0f, 0.0f));
	}

	else if (st == scene_type::directional_movement)
	{
		std::string bot = "data/assets/MIXAMO/xbot.gltf";
		import_gltf_file(bot.c_str());
		node* b = create_model_instance(g_resources.get_model_id(bot));

		// Create the 1d blend tree
		anim_comp* ac = b->get_component<anim_comp>();
		ac->set_1d_blend_tree();

		// Add player controller component
		b->add_component<player_controller>()->set_directional_movement(true);

		g_editor.set_selected_node(b);

		// Insert blend nodes
		ac->insert_blend_node(14, glm::vec2(-1.0f, 0.0f));
		ac->insert_blend_node(5, glm::vec2(0.0f));
		ac->insert_blend_node(22, glm::vec2(0.4f, 0.0f));
		ac->insert_blend_node(6, glm::vec2(0.8f, 0.0f));
		ac->insert_blend_node(15, glm::vec2(1.0f, 0.0f));
		ac->insert_blend_node(3, glm::vec2(2.0f, 0.0f));
	}

	else if (st == scene_type::anim_blending_2d)
	{
		std::string bot = "data/assets/MIXAMO/xbot.gltf";
		import_gltf_file(bot.c_str());
		node* b = create_model_instance(g_resources.get_model_id(bot));

		g_editor.set_selected_node(b);

		// Create the 1d blend tree
		anim_comp* ac = b->get_component<anim_comp>();
		ac->set_2d_blend_tree();

		// Insert blend nodes
		ac->insert_blend_node(5, glm::vec2(0.0f));
		ac->insert_blend_node(22, glm::vec2(0.0f, 1.0f));
		ac->insert_blend_node(23, glm::vec2(-1.0f, 0.0f));
		ac->insert_blend_node(24, glm::vec2(1.0f, 0.0f));
		ac->insert_blend_node(25, glm::vec2(0.0f, -1.0f));
		ac->insert_blend_node(17, glm::vec2(2.0f, 0.0f));
		ac->insert_blend_node(16, glm::vec2(-2.0f, 0.0f));
		ac->insert_blend_node(15, glm::vec2(0.0f, 2.0f));
	}

	else if (st == scene_type::targeted_movement)
	{
		std::string bot = "data/assets/MIXAMO/xbot.gltf";
		import_gltf_file(bot.c_str());
		node* b = create_model_instance(g_resources.get_model_id(bot));
		b->m_local.set_position(glm::vec3(0.0f, 0.0f, 10.0f));

		// Add player controller component
		player_controller* pc = b->add_component<player_controller>();
		pc->set_directional_movement(false);
		pc->initialize();

		g_editor.set_selected_node(b);

		// Create the 1d blend tree
		anim_comp* ac = b->get_component<anim_comp>();
		ac->set_2d_blend_tree();

		// Insert blend nodes
		ac->insert_blend_node(5, glm::vec2(0.0f));
		ac->insert_blend_node(22, glm::vec2(0.0f, 1.0f));
		ac->insert_blend_node(23, glm::vec2(-1.0f, 0.0f));
		ac->insert_blend_node(24, glm::vec2(1.0f, 0.0f));
		ac->insert_blend_node(25, glm::vec2(0.0f, -1.0f));

		std::string cessium = "data/assets/skull/skull.gltf";

		import_gltf_file(cessium.c_str());
		create_model_instance(g_resources.get_model_id(cessium))->m_local.set_position(glm::vec3(0,2,0));
	}

	else if (st == scene_type::analytic_2_bone_ik)
	{
		render_grid(false);
		node* ik = new node;
		ik->add_component<ik_2d>();
		ik->m_name = "2 bone IK: Joint 0";
		m_root->add_child(ik);
		g_editor.set_selected_node(ik);
		g_editor.set_picking(false);
		g_editor.set_guizmo(false);
	}

	else if (st == scene_type::cyclic_coord_descent)
	{
		get_camera().m_pos = glm::vec3(1.5f, 3.0f, 12.0f);
		get_camera().m_view = glm::normalize(glm::vec3(0.0f, -0.2f, -1.0f));
		node* ik = new node;
		m_root->add_child(ik);
		ik->add_component<ccd>()->default_init();
		ik->m_name = "CCD IK: Target";
		g_editor.set_selected_node(ik);
		g_editor.set_picking(false);
	}

	else if (st == scene_type::fabrik)
	{
		get_camera().m_pos = glm::vec3(1.5f, 3.0f, 12.0f);
		get_camera().m_view = glm::normalize(glm::vec3(0.0f, -0.2f, -1.0f));
		node* ik = new node;
		m_root->add_child(ik);
		ik->add_component<fabrik>()->default_init();
		ik->m_name = "FABRIK: Target";
		g_editor.set_selected_node(ik);
		g_editor.set_picking(false);
	}

	else if (st == scene_type::ik_demo)
	{
		std::string model_name = "data/assets/rigged figure/CesiumMan.gltf";
		import_gltf_file(model_name.c_str());
		node* c = create_model_instance(g_resources.get_model_id(model_name));
		c->get_component<anim_comp>()->set_animation(-1);
		c->m_local.set_scale(glm::vec3(4.0f));

		g_scene.update_node_transforms();

		get_camera().m_pos = glm::vec3(0.0f, 5.0f, 12.0f);
		get_camera().m_view = glm::normalize(glm::vec3(0.0f, -0.2f, -1.0f));
		
		node* ik = new node;
		m_root->add_child(ik);
		fabrik* f1 = ik->add_component<fabrik>();
		ik->m_name = "FABRIK: Target";
		
		ik->m_local.set_position(glm::vec3(2.0f, 2.0f, 0.0f));

		f1->clear_joints();
		f1->render_bones(false);
		f1->show_gui(false);
		f1->add_joint(g_scene.get_model_node(c->m_model, c->m_model_inst, 17));
		f1->add_joint(g_scene.get_model_node(c->m_model, c->m_model_inst, 18));
		f1->add_joint(g_scene.get_model_node(c->m_model, c->m_model_inst, 19));

		g_editor.set_selected_node(ik);
		g_editor.set_picking(false);
	}
}
}
