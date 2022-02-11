#pragma once
#include "component.h"
#include <vector>
#include <glm/glm.hpp>
#include <list>

namespace cs460 {
struct node;
struct anim_comp;
class distance_table
{
	struct table_element
	{
		table_element() {};
		table_element(float u, float s) : m_parameter(u), m_distance(s) {}
		float m_parameter = 0.0f;
		float m_distance = 0.0f;
	};

public:
	template<typename T>
	void compute_table(const T* curve);

	template<typename T>
	void imgui(const T* curve);

	template<typename T>
	void render_table(const T* curve);
	
	void set_step(float step);

	float get_parameter_from_dist(float dist) const;

	float get_total_dist() const;

	bool m_init = false;

private:
	template<typename T>
	void uniform_method(const T* curve);

	template<typename T>
	void adaptive_method(const T* curve);

	template<typename T>
	void uniform_imgui(const T* curve);

	template<typename T>
	void adaptive_imgui(const T* curve);

	typedef std::list<table_element> table_list;
	typedef table_list::iterator list_it;
	template<typename T>
	void adaptive_method_rec(const T* curve, table_list& list, list_it l_it, list_it r_it, 
		const glm::vec3& l_point, const glm::vec3& r_point, int force_division, int level);

	bool get_param_from_dist_rec(int left, int right, float dist, float* param) const;

	std::vector<table_element> m_table;

	float m_step = 0.05f;
	float m_epsilon = 1.0f;
	int m_force_div = 5;
	bool m_adaptive = false;
	bool m_render = false;
};

class curve_controller
{
public:
	void imgui();

	template<typename T>
	void update(const T* curve, const distance_table& table);

	float get_param() { return m_param; }
	void set_follower(node* follower);

private:
	void reset();
	void cte_speed_update(const distance_table& table);
	void distance_time_func_update(const distance_table& table);
	float distance_time_function(float tn);
	float ease_in(float tn);
	float linear(float tn);
	float ease_out(float tn);
	float normalizer();

	template<typename T>
	void move_follower(const T* curve);
	
	anim_comp* m_follower_anim = nullptr;
	node* m_follower = nullptr;

	// Common
	float m_param = 0;
	bool m_loop = true;
	bool m_play = true;
	float m_ref_rate = 0.7f;
	bool m_frenet = false;
	bool m_draw_frame = false;

	bool m_cte_speed = true;
	float m_total_time = 0.0f;
	float m_total_dist = 0.0f;

	// Constant speed
	float m_rate_of_travel = 1.0f;
	float m_travelled = 0.0f;

	// Distance Time
	float m_timer = 0.0f;
	float m_prev_dist = 0.0f;
	float m_t1 = 0.5f;
	float m_t2 = 0.5f;
};

class curve : public component
{
public:
	void set_curve_id(const int id) { m_id = id; }
	void add_point(const glm::vec3& pos);
	void set_follower(node* follower);

private:
	virtual void update();
	virtual void imgui();
	void render_curve_points();
	void add_point();
	float lerp(float t, int* start, int* end) const;

	std::vector<node*> m_points;
	distance_table m_table;
	curve_controller m_controller;

	int m_id = 0;
	bool m_once = false;

	glm::vec4 m_segment_color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	glm::vec4 m_point_color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	glm::vec4 m_tangent_color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
	glm::vec4 m_tangent_line_color = glm::vec4(0.0f, 0.5f, 0.5f, 1.0f);
	glm::vec3 m_offset = glm::vec3(0.02f);

	friend class linear_curve;
	friend class hermite_curve;
	friend class catmull_rom_curve;
	friend class bezier_curve;
};

class linear_curve : public curve
{
public:
	glm::vec3 lerp(float t) const;
	void add_point(const glm::vec3& pos);

private:
	virtual void update();
	virtual void imgui();
	void render_curve();
};

class hermite_curve : public curve
{
public:
	glm::vec3 lerp(float t) const;
	glm::vec3 first_derivative(float t) const;
	glm::vec3 second_derivative(float t) const;

	void add_point(const glm::vec3& pos, const glm::vec3& tan1, const glm::vec3& tan2);

private:
	float get_segment_points(float t, const glm::vec3** p0, glm::vec3* t0, const glm::vec3** p1, glm::vec3* t1) const;
	struct tangent { node* m_t1 = nullptr; node* m_t2 = nullptr; };
	std::vector<tangent> m_tangents;
	virtual void update();
	virtual void imgui();
	void render_curve();
};

class catmull_rom_curve : public curve
{
public:
	glm::vec3 lerp(float t) const;
	glm::vec3 first_derivative(float t) const;
	glm::vec3 second_derivative(float t) const;

	void add_point(const glm::vec3& pos);

private:
	float get_segment_points(float t, const glm::vec3** p0, const glm::vec3** t0, const glm::vec3** p1, const glm::vec3** t1) const;
	std::vector<glm::vec3> m_tangents;
	virtual void update();
	virtual void imgui();
	void render_curve();
	void compute_tangents();
};

class bezier_curve : public curve
{
public:
	glm::vec3 lerp(float t) const;
	glm::vec3 first_derivative(float t) const;
	glm::vec3 second_derivative(float t) const;

	void add_point(const glm::vec3& pos, const glm::vec3& c1, const glm::vec3& c2);

private:
	float get_segment_points(float t, const glm::vec3** p0, const glm::vec3** p1, const glm::vec3** p2, const glm::vec3** p3) const;
	struct control_point { node* m_c1 = nullptr; node* m_c2 = nullptr; };
	std::vector<control_point> m_control_points;
	virtual void update();
	virtual void imgui();
	void render_curve();
};
}