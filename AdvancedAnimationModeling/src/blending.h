#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <unordered_map>
#include "transform.h"
#include <string>
#include <array>

namespace cs460 {
struct animation;

typedef std::unordered_map<unsigned int, std::pair<transform, unsigned char>> anim_pose;

enum target_type { translation = 1, rotation = 2, scale = 4 };

struct blend_node
{
	std::vector<blend_node*> m_children;
	glm::vec2 m_blend_pos = glm::vec2(0.0f, 0.0f);

	blend_node* m_parent = nullptr;

	int m_model = 0;
	int m_anim = 0;

	anim_pose m_current_pose;

	void produce_pose(glm::vec2& blend_param, float time);

	// Implemented by objects that inherit from blend_node
	virtual void blend_children(glm::vec2& blend_param, float time) {}
	virtual void insert_node(int model_idx, int anim_idx, const glm::vec2& blend_pos) {}
	virtual void imgui(int tree_level, int child_id, blend_node*& selected) {}
	virtual void sort_childs() {}
	virtual void erase_child(blend_node* node) {}
	virtual void blend_graph(const glm::vec2& blend_param) {}
	virtual bool enough_blend_nodes() { return false; }
	virtual void get_min_max_blend_param(glm::vec2& min, glm::vec2& max) {}
	virtual void update_min_max_blend_param() {}
};

struct blend_node_1d : public blend_node
{
	void find_segment(float param, blend_node*& to, blend_node*& from);
	virtual void blend_children(glm::vec2& blend_param, float time);
	virtual void insert_node(int model_idx, int anim_idx, const glm::vec2& blend_pos);
	virtual void imgui(int tree_level, int child_id, blend_node*& selected);
	virtual void sort_childs();
	virtual void erase_child(blend_node* node);
	virtual void blend_graph(const glm::vec2& blend_param);
	virtual bool enough_blend_nodes();
	virtual void get_min_max_blend_param(glm::vec2& min, glm::vec2& max);

private:
	bool find_segment_rec(float param, int* left, int* right);
};

struct blend_node_2d : public blend_node
{
	virtual void blend_children(glm::vec2& blend_param, float time);
	virtual void insert_node(int model_idx, int anim_idx, const glm::vec2& blend_pos);
	virtual void imgui(int tree_level, int child_id, blend_node*& selected);
	virtual void erase_child(blend_node* node);
	virtual void blend_graph(const glm::vec2& blend_param);
	virtual bool enough_blend_nodes();
	virtual void get_min_max_blend_param(glm::vec2& min, glm::vec2& max);
	virtual void update_min_max_blend_param();

private:
	// Triangle container. Populated generate_triangles()
	std::vector<std::array<unsigned int, 3>> m_triangles;

	// Min and max blend parameters
	glm::vec2 m_min = glm::vec2(0.0f);
	glm::vec2 m_max = glm::vec2(1.0f);

	// Determines in which triangle the blend parameter is located and
	// computes the barycentric coordinates
	void find_nodes_barycentric(const glm::vec2& blend_param,
		blend_node*& n0, blend_node*& n1, blend_node*& n2, float& a0, float& a1, float& a2);

	float cross_product(const glm::vec2& v0, const glm::vec2& v1);
	bool compute_barycentric(const glm::vec2& point, const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, 
		float& a0, float& a1, float& a2);

	// Generates the triangles using delaunay triangulation
	void generate_triangles();

	int m_inside_triangle = -1;
};

class blend_tree
{
public:
	void destroy();
	void imgui();
	const anim_pose* produce_pose(float time);
	void create_1d_blend_tree();
	void create_2d_blend_tree();
	void insert_blend_node(int model_idx, int anim_idx, const glm::vec2& blend_pos);
	void set_blend_param(const glm::vec2& param) { m_blend_param = param; }
	void display_blend_graph() const { if (m_root) m_root->blend_graph(m_blend_param); }

private:
	void destroy_rec(blend_node* node);

	blend_node* m_selected = nullptr;
	blend_node* m_root = nullptr;
	glm::vec2 m_blend_param = glm::vec2(0.0f);
};
}