/**
* @file mesh_comp.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include "component.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace cs460 {
struct node_rsc;
struct model_rsc;
struct mesh_comp : public component
{
	virtual void initialize();
	virtual void update();
	virtual void imgui();

	void set_mesh(int mesh_idx, int skin_idx, int skin_root_idx);
	void render_vb();
	void render_skin();

	void get_vb_min_max(glm::vec3& min, glm::vec3& max);

	int get_mesh() const { return m_mesh; }
	int get_skin() const { return m_skin; }
	int get_skin_root() const { return m_skin_root; }

private:
	void set_skin();
	void set_skin_rec(const model_rsc& resources, const int node);

	int m_mesh = -1;
	int m_skin = -1;
	int m_skin_root = -1;

	glm::vec3 m_bv[8] = { glm::vec3(0.0f) };
	glm::vec4 m_bv_color = glm::vec4(1.0f);
	std::vector<int> m_skin_segments;
};
}