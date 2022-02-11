/**
* @file editor.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include <glm/glm.hpp>

namespace cs460 {
struct node;
class editor
{
public:
	void render();
	static editor& get_instance();
	node* get_selected_node() { return m_selected_node; }
	void remove_selection() { m_selected_node = nullptr; }
	void set_selected_node(node* n) { m_selected_node = n; }
	void set_picking(bool can_pick) { m_can_pick = can_pick; }
	void set_guizmo(bool use_guizmo) { m_use_guizmo = use_guizmo; }
	void render_grid();
	glm::vec3 get_mouse_ray();

private:
	editor();
	void render_scene_graph();
	void render_scene_graph_rec(node* node, bool opened = false);
	void raycast_rec(node* node, const glm::vec3& origin, const glm::vec3& dir, float* t_min);
	void imgui_render_frame();
	void imgui_new_frame();
	void render_inspector();
	bool render_guizmo();
	void load_gltf_files();
	void render_node_bv();
	void render_node_skin();
	void render_bvs();
	void render_skins();
	void curve_creator();
	void picking();
	float ray_vs_aabb(const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& min, const glm::vec3& max);
	void player_controls();

	int m_guizmode;
	int m_perspective; // World or local
	node* m_selected_node = nullptr;
	bool m_scene_graph_node_selected = false;
	bool m_can_pick = true;
	bool m_use_guizmo = true;
};

#define g_editor editor::get_instance()
}