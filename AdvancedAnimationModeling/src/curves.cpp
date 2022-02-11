#include "curves.h"
#include "node.h"
#include "debug.h"
#include "imgui.h"
#include "clock.h"
#include "curve_node_comp.h"
#include "anim_comp.h"

namespace cs460 {
void curve::update()
{
	// Draw the curve
	render_curve_points();
}

void curve::imgui()
{
	int n_points = (int)m_points.size();

	if (n_points <= 1)
		return;

	m_controller.imgui();
}

void curve::render_curve_points()
{
	// Draw curve points
	int n_points = (int)m_points.size();
	for (int i = 0; i < n_points; ++i)
	{
		const glm::vec3& pos = m_points[i]->m_world.get_position();
		g_debug.debug_draw_aabb(pos - m_offset, pos + m_offset, m_point_color, false);
	}
}

void curve::add_point()
{
	node* point = new node;
	point->add_component<curve_node_comp>();
	point->m_name = "curve " + std::to_string(m_id) + " point " + std::to_string(m_points.size());
	m_points.push_back(point);
	get_owner()->add_child(point);
}

void curve::add_point(const glm::vec3& pos)
{
	node* point = new node;
	point->add_component<curve_node_comp>();
	point->m_name = "curve " + std::to_string(m_id) + " point " + std::to_string(m_points.size());
	point->m_local.set_position(pos);
	m_points.push_back(point);
	get_owner()->add_child(point);
}

void curve::set_follower(node* follower)
{
	m_controller.set_follower(follower);
}

float curve::lerp(float t, int* start, int* end) const
{
	// Get the number of points
	size_t n_points = m_points.size();
	if (n_points < 2)
		return -1;

	// Cap lerp output
	if (t <= 0.0f)
	{
		*start = 0;
		*end = 1;
		return 0.0f;
	}
	else if (t >= 1.0f)
	{
		*start = (int)n_points - 2;
		*end = (int)n_points - 1;
		return 1.0f;
	}

	float t_step = 1.0f / (n_points - 1);
	int frame_idx = (int)glm::floor(t / t_step);
	frame_idx = glm::clamp(frame_idx, 0, (int)(n_points - 2));

	// Get the segment
	*start = frame_idx;
	*end = frame_idx + 1;

	// Normalize the t parameter
	float interval = t_step * (*end) - t_step * (*start);
	float local_time = t - t_step * (*start);
	float tn = local_time / interval;
	tn = glm::clamp(tn, 0.0f, 1.0f);

	return tn;
}

glm::vec3 linear_curve::lerp(float t) const
{
	// Normalized parameter
	int start, end;
	float tn = curve::lerp(t, &start, &end);

	if (tn < 0)
		return glm::vec3(0.0f);

	// Linear lerp
	const glm::vec3& p0 = m_points[start]->m_world.get_position();
	const glm::vec3& p1 = m_points[end]->m_world.get_position();
	return p0 + (p1 - p0) * tn;
}

void linear_curve::add_point(const glm::vec3& pos)
{
	curve::add_point(pos);
}

void linear_curve::update()
{
	curve::update();
	render_curve();
	m_table.render_table(this);

	if (m_once && m_table.m_init == false)
	{
		m_table.m_init = true;
		m_table.compute_table(this);
	}

	m_once = true;
}

void linear_curve::imgui()
{
	if (ImGui::CollapsingHeader("Linear Curve Component:"))
	{
		curve::imgui();
		if (ImGui::CollapsingHeader("Edit Curve"))
		{
			if (ImGui::Button("Add Point"))
			{
				curve::add_point();
			}
		}

		m_table.imgui(this);
	}
}

void linear_curve::render_curve()
{
	// Draw curve segments
	size_t n_points = m_points.size();
	for (size_t i = 0; i < n_points; ++i)
	{
		if (i + 1 < n_points)
		{
			const glm::vec3& start = m_points[i]->m_world.get_position();
			const glm::vec3& end = m_points[i + 1]->m_world.get_position();
			g_debug.debug_draw_line(start, end, m_segment_color);
		}
	}
}

glm::vec3 bezier_curve::lerp(float t) const
{
	const glm::vec3* p0;
	const glm::vec3* p1;
	const glm::vec3* p2;
	const glm::vec3* p3;
	float tn = get_segment_points(t, &p0, &p1, &p2, &p3);

	float cte = (1 - tn);

	return (cte* cte * cte) * (*p0)
		+ 3.0f * tn * (cte * cte) * (*p1)
		+ 3.0f * (tn * tn) * cte * (*p2)
		+ (tn * tn * tn) * (*p3);
}

glm::vec3 bezier_curve::first_derivative(float t) const
{
	const glm::vec3* p0;
	const glm::vec3* p1;
	const glm::vec3* p2;
	const glm::vec3* p3;
	float tn = get_segment_points(t, &p0, &p1, &p2, &p3);

	return (tn * tn) * (3.0f * (*p3 + 3.0f * (-(*p2) + *p1) - *p0))
		+ tn * (6.0f * (*p2 - 2.0f * (*p1) + *p0))
		+ 3.0f * (*p1 - *p0);
}

glm::vec3 bezier_curve::second_derivative(float t) const
{
	const glm::vec3* p0;
	const glm::vec3* p1;
	const glm::vec3* p2;
	const glm::vec3* p3;
	float tn = get_segment_points(t, &p0, &p1, &p2, &p3);

	return 6.0f * tn * (-(*p0) + 3.0f * (*p1) - 3.0f * (*p2) + *p3)
		+ 6.0f * (*p0 - 2.0f * (*p1) + *p2);
}

float bezier_curve::get_segment_points(float t, const glm::vec3** p0, const glm::vec3** p1, const glm::vec3** p2, const glm::vec3** p3) const
{
	// Normalized parameter
	int start, end;
	float tn = curve::lerp(t, &start, &end);

	if (tn < 0)
		return -1;

	// Bezier lerp
	*p0 = &m_points[start]->m_world.get_position();
	*p3 = &m_points[end]->m_world.get_position();
	*p1 = &m_control_points[start].m_c2->m_world.get_position();
	*p2 = &m_control_points[end].m_c1->m_world.get_position();

	return tn;
}

void bezier_curve::add_point(const glm::vec3& pos, const glm::vec3& c1, const glm::vec3& c2)
{
	curve::add_point(pos);

	int n_points = (int)m_points.size();
	int point = n_points - 1;
	int prev_point = point - 1;

	// Set the control point of the previous point
	if (prev_point > 0)
	{
		node* c2 = new node;
		c2->add_component<curve_node_comp>();
		c2->m_name = "point " + std::to_string(prev_point) + " control 2";
		c2->m_local.set_position(c1);
		m_control_points[prev_point].m_c2 = c2;
		m_points[prev_point]->add_child(c2);
	}

	// Set the tangent of the new point
	node* c = new node;
	c->add_component<curve_node_comp>();
	m_control_points.emplace_back();
	if (point != 0)
	{
		c->m_local.set_position(c2);
		c->m_name = "point " + std::to_string(point) + " control 1";
		m_control_points.back().m_c1 = c;
	}
	else
	{
		c->m_local.set_position(c1);
		c->m_name = "point " + std::to_string(point) + " control 2";
		m_control_points.back().m_c2 = c;
	}

	m_points[point]->add_child(c);
}

void bezier_curve::update()
{
	curve::update();
	render_curve();
	m_table.render_table(this);

	if (m_once && m_table.m_init == false)
	{
		m_table.m_init = true;
		m_table.compute_table(this);
	}

	m_once = true;

	if (m_table.m_init)
		m_controller.update(this, m_table);
}

void bezier_curve::imgui()
{
	if (ImGui::CollapsingHeader("Bezier Curve Component:"))
	{
		curve::imgui();
		if (ImGui::CollapsingHeader("Edit Curve"))
		{
			if (ImGui::Button("Add Point"))
			{
				add_point(glm::vec3(0.0f), glm::vec3(0.2f, 0.2f, 0.0f), glm::vec3(-0.2f, 0.2f, 0.0f));
			}
		}
		m_table.imgui(this);
	}
}

void bezier_curve::render_curve()
{
	size_t n_points = m_points.size();
	if (n_points < 2)
		return;

	// Render control points
	for (size_t i = 0; i < n_points; ++i)
	{
		const glm::vec3& p = m_points[i]->m_world.get_position();
		node* c1 = m_control_points[i].m_c1;
		node* c2 = m_control_points[i].m_c2;
		if (c1)
		{
			const glm::vec3& pos = c1->m_world.get_position();
			g_debug.debug_draw_aabb(pos - m_offset, pos + m_offset, m_tangent_color);
		}
		if (c2)
		{
			const glm::vec3& pos = c2->m_world.get_position();
			g_debug.debug_draw_aabb(pos - m_offset, pos + m_offset, m_tangent_color);
		}
	}

	// Go through all the keys
	float step = 0.01f;
	float t = step;
	glm::vec3 prev = lerp(0.0f);
	while (t < 1.0f)
	{
		glm::vec3 point = lerp(t);
		g_debug.debug_draw_line(prev, point, m_segment_color);
		t += step;
		prev = point;
	}
}

glm::vec3 catmull_rom_curve::lerp(float t) const
{
	const glm::vec3* p0;
	const glm::vec3* t0;
	const glm::vec3* p1;
	const glm::vec3* t1;
	float tn = get_segment_points(t, &p0, &t0, &p1, &t1);

	return (tn * tn * tn) * (2.0f * (*p0 - *p1) + *t0 + *t1)
		+ (tn * tn) * (3.0f * (*p1 - *p0) - 2.0f * (*t0) - *t1)
		+ tn * (*t0) + *p0;
}

glm::vec3 catmull_rom_curve::first_derivative(float t) const
{
	const glm::vec3* p0;
	const glm::vec3* t0;
	const glm::vec3* p1;
	const glm::vec3* t1;
	float tn = get_segment_points(t, &p0, &t0, &p1, &t1);

	glm::vec3 a = 2.0f * (*p0 - *p1) + *t0 + *t1;
	glm::vec3 b = 3.0f * (*p1 - *p0) - 2.0f * (*t0) - *t1;
	glm::vec3 c = *t0;

	return 3.0f * a * (tn * tn) + 2.0f * b * tn + c;
}

glm::vec3 catmull_rom_curve::second_derivative(float t) const
{
	const glm::vec3* p0;
	const glm::vec3* t0;
	const glm::vec3* p1;
	const glm::vec3* t1;
	float tn = get_segment_points(t, &p0, &t0, &p1, &t1);

	glm::vec3 a = 2.0f * (*p0 - *p1) + *t0 + *t1;
	glm::vec3 b = 3.0f * (*p1 - *p0) - 2.0f * (*t0) - *t1;

	return 6.0f * a * tn + 2.0f * b;
}

void catmull_rom_curve::add_point(const glm::vec3& pos)
{
	// Add the point
	curve::add_point(pos);
	m_tangents.emplace_back();
	compute_tangents();
}

float catmull_rom_curve::get_segment_points(float t, const glm::vec3** p0, const glm::vec3** t0, const glm::vec3** p1, const glm::vec3** t1) const
{
	// Normalized parameter
	int start, end;
	float tn = curve::lerp(t, &start, &end);

	if (tn < 0)
		return -1;

	*p0 = &m_points[start]->m_world.get_position();
	*p1 = &m_points[end]->m_world.get_position();
	*t0 = &m_tangents[start];
	*t1 = &m_tangents[end];

	return tn;
}

void catmull_rom_curve::update()
{
	curve::update();
	compute_tangents();
	render_curve();
	m_table.render_table(this);

	if (m_once && m_table.m_init == false)
	{
		m_table.m_init = true;
		m_table.compute_table(this);
	}

	m_once = true;

	if (m_table.m_init)
		m_controller.update(this, m_table);

}

void catmull_rom_curve::imgui()
{
	if (ImGui::CollapsingHeader("Catmull-Rom Curve Component:"))
	{
		curve::imgui();
		if (ImGui::CollapsingHeader("Edit Curve"))
		{
			if (ImGui::Button("Add Point"))
			{
				add_point(glm::vec3(0.0f));
			}
		}
		m_table.imgui(this);
	}
}

void catmull_rom_curve::render_curve()
{
	size_t n_points = m_points.size();
	if (n_points < 3)
		return;

	// Go through all the keys
	float step = 0.01f;
	float t = step;
	glm::vec3 prev = lerp(0.0f);
	while (t < 1.0f)
	{
		glm::vec3 point = lerp(t);
		g_debug.debug_draw_line(prev, point, m_segment_color);
		t += step;
		prev = point;
	}
}

void catmull_rom_curve::compute_tangents()
{
	// Number of points
	size_t n_points = m_points.size();
	if (n_points < 3)
		return;

	// Endpoints: First Point
	const glm::vec3* p0 = &m_points[0]->m_world.get_position();
	const glm::vec3* p1 = &m_points[1]->m_world.get_position();
	const glm::vec3* p2 = &m_points[2]->m_world.get_position();
	m_tangents[0] = 0.5f * ((*p1 - *p0) + (*p1 - *p2));

	// Endpoints: Last Point
	p0 = &m_points[n_points - 1]->m_world.get_position();
	p1 = &m_points[n_points - 2]->m_world.get_position();
	p2 = &m_points[n_points - 3]->m_world.get_position();
	m_tangents[n_points - 1] = -0.5f * ((*p1 - *p0) + (*p1 - *p2));

	size_t end_point = n_points - 1;
	for (size_t i = 1; i < end_point; ++i)
	{
		// Compute the tangent of the mid point
		const glm::vec3& start_pos = m_points[i - 1]->m_world.get_position();
		const glm::vec3& end_pos   = m_points[i + 1]->m_world.get_position();
		m_tangents[i] = 0.5f * (end_pos - start_pos);
	}
}

glm::vec3 hermite_curve::lerp(float t) const
{
	const glm::vec3* p0;
	glm::vec3 t0;
	const glm::vec3* p1;
	glm::vec3 t1;
	float tn = get_segment_points(t, &p0, &t0, &p1, &t1);

	return (tn * tn * tn) * (2.0f * (*p0 - *p1) + t0 + t1)
		+ (tn * tn) * (3.0f * (*p1 - *p0) - 2.0f * (t0) - t1)
		+ tn * (t0) + *p0;
}

glm::vec3 hermite_curve::first_derivative(float t) const
{
	const glm::vec3* p0;
	glm::vec3 t0;
	const glm::vec3* p1;
	glm::vec3 t1;
	float tn = get_segment_points(t, &p0, &t0, &p1, &t1);

	glm::vec3 a = 2.0f * (*p0 - *p1) + t0 + t1;
	glm::vec3 b = 3.0f * (*p1 - *p0) - 2.0f * (t0) - t1;
	glm::vec3 c = t0;

	return 3.0f * a * (tn * tn) + 2.0f * b * tn + c;
}

glm::vec3 hermite_curve::second_derivative(float t) const
{
	const glm::vec3* p0;
	glm::vec3 t0;
	const glm::vec3* p1;
	glm::vec3 t1;
	float tn = get_segment_points(t, &p0, &t0, &p1, &t1);

	glm::vec3 a = 2.0f * (*p0 - *p1) + t0 + t1;
	glm::vec3 b = 3.0f * (*p1 - *p0) - 2.0f * (t0) - t1;

	return 6.0f * a * tn + 2.0f * b;
}

void hermite_curve::add_point(const glm::vec3& pos, const glm::vec3& tan1, const glm::vec3& tan2)
{
	curve::add_point(pos);
	
	int n_points = (int)m_points.size();
	int point = n_points - 1;
	int prev_point = point - 1;

	// Set the tangent of the previous point
	if (prev_point > 0)
	{
		node* t2 = new node;
		t2->add_component<curve_node_comp>();
		t2->m_name = "point " + std::to_string(prev_point) + " tangent 2";
		t2->m_local.set_position(tan1);
		m_tangents[prev_point].m_t2 = t2;
		m_points[prev_point]->add_child(t2);
	}

	// Set the tangent of the new point
	node* t = new node;
	t->add_component<curve_node_comp>();
	m_tangents.emplace_back();
	if (point != 0)
	{
		t->m_local.set_position(tan2);
		t->m_name = "point " + std::to_string(point) + " tangent 1";
		m_tangents.back().m_t1 = t;
	}
	else
	{
		t->m_local.set_position(tan1);
		t->m_name = "point " + std::to_string(point) + " tangent 2";
		m_tangents.back().m_t2 = t;
	}

	m_points[point]->add_child(t);
}

float hermite_curve::get_segment_points(float t, const glm::vec3** p0, glm::vec3* t0, const glm::vec3** p1, glm::vec3* t1) const
{
	// Normalized parameter
	int start, end;
	float tn = curve::lerp(t, &start, &end);

	if (tn < 0)
		return -1;

	*p0 = &m_points[start]->m_world.get_position();
	*p1 = &m_points[end]->m_world.get_position();
	*t0 = m_tangents[start].m_t2->m_world.get_position() - **p0;
	*t1 = m_tangents[end].m_t1->m_world.get_position() - **p1;

	return tn;
}

void hermite_curve::update()
{
	curve::update();
	render_curve();
	m_table.render_table(this);

	if (m_once && m_table.m_init == false)
	{
		m_table.m_init = true;
		m_table.compute_table(this);
	}

	m_once = true;

	if (m_table.m_init)
		m_controller.update(this, m_table);
}

void hermite_curve::imgui()
{
	if (ImGui::CollapsingHeader("Hermite Curve Component:"))
	{
		curve::imgui();
		if (ImGui::CollapsingHeader("Edit Curve"))
		{
			if (ImGui::Button("Add Point"))
			{
				add_point(glm::vec3(0.0f), glm::vec3(0.2f, 0.0f, 0.0f), glm::vec3(0.2f, 0.0f, 0.0f));
			}
		}
		
		m_table.imgui(this);
	}
}

void hermite_curve::render_curve()
{
	size_t n_points = m_points.size();
	if (n_points < 2)
		return;

	// Render tangents
	for (size_t i = 0; i < n_points; ++i)
	{
		const glm::vec3& p = m_points[i]->m_world.get_position();
		node* t1 = m_tangents[i].m_t1;
		node* t2 = m_tangents[i].m_t2;
		if (t1)
		{
			const glm::vec3& pos = t1->m_world.get_position();
			g_debug.debug_draw_aabb(pos - m_offset, pos + m_offset, m_tangent_color);
			g_debug.debug_draw_line(p, pos, m_tangent_line_color);
		}
		if (t2)
		{
			const glm::vec3& pos = t2->m_world.get_position();
			g_debug.debug_draw_aabb(pos - m_offset, pos + m_offset, m_tangent_color);
			g_debug.debug_draw_line(p, pos, m_tangent_line_color);
		}
	}

	// Go through all the keys
	float step = 0.01f;
	float t = step;
	glm::vec3 prev = lerp(0.0f);
	while (t < 1.0f)
	{
		glm::vec3 point = lerp(t);
		g_debug.debug_draw_line(prev, point, m_segment_color);
		t += step;
		prev = point;
	}
}

void distance_table::set_step(float step)
{
	m_step = glm::clamp((float)step, 0.001f, 0.05f);
	int n_elements = (int)glm::ceil(1.0f / m_step) + 1;
	m_table.resize(n_elements);
}

float distance_table::get_parameter_from_dist(float dist) const
{
	float param = -1.0f;
	size_t size = m_table.size();
	
	// Sanity check
	assert(size > 1);

	// Clamp distance
	dist = glm::clamp(dist, 0.0f, m_table[size - 1].m_distance);

	// Find parameter
	get_param_from_dist_rec(0, (int)size - 1, dist, &param);
	return param;
}

float distance_table::get_total_dist() const
{
	if (m_table.empty() == false)
		return m_table.back().m_distance;
	else
		return 0.0f;
}

bool distance_table::get_param_from_dist_rec(int left, int right, float dist, float* param) const
{
	// End condition
	if (left == right - 1)
	{
		if (m_table[left].m_distance == dist)
			*param = m_table[left].m_parameter;
		else if (m_table[right].m_distance == dist)
			*param = m_table[right].m_parameter;
		else
		{
			// Lerp between left and right
			float local_diff = dist - m_table[left].m_distance;
			float diff = m_table[right].m_distance - m_table[left].m_distance;
			float n = local_diff / diff;

			// Lerp the parameter
			float l_u = m_table[left].m_parameter;
			float r_u = m_table[right].m_parameter;
			*param = l_u + (r_u - l_u) * n;
		}
		
		return true;
	}

	// Index of the element between left and right
	int mid = left + (int)glm::floor((right - left) / 2.0f);
	
	// Parameter found
	if (m_table[mid].m_distance == dist)
	{
		*param = m_table[mid].m_parameter;
		return true;
	}

	// Recurse left
	else if (m_table[mid].m_distance > dist)
	{
		if (get_param_from_dist_rec(left, mid, dist, param))
			return true;
	}

	// Recurse right
	else
	{
		if (get_param_from_dist_rec(mid, right, dist, param))
			return true;
	}

	return false;
}

template<typename T>
void distance_table::compute_table(const T* curve)
{
	if (m_adaptive)
		adaptive_method<T>(curve);
	else
		uniform_method<T>(curve);
}

template<typename T>
void distance_table::imgui(const T* curve)
{
	if (ImGui::CollapsingHeader("Distance Table"))
	{
		if (ImGui::BeginCombo("Table Type", m_adaptive ? "Adaptive Table" : "Uniform Table"))
		{
			if (ImGui::Selectable("Uniform Table"))
			{
				m_adaptive = false;
				compute_table(curve);
			}

			if (ImGui::Selectable("Adaptive Table"))
			{
				m_adaptive = true;
				compute_table(curve);
			}

			ImGui::EndCombo();
		}

		ImGui::Checkbox("Render Table", &m_render);

		if (m_adaptive)
			adaptive_imgui(curve);
		else
			uniform_imgui(curve);


		if (ImGui::Button("Rebuild Table"))
			compute_table<T>(curve);

		ImGui::Separator();
		ImGui::Text((std::to_string(m_table.size()) + " entries").c_str());
		ImGui::Separator();

		if (ImGui::TreeNode("Table Data"))
		{
			// Expose a few Borders related flags interactively
			enum ContentsType { CT_Text, CT_FillButton };
			ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
			if (ImGui::BeginTable("Distance Table", 3, flags))
			{
				ImGui::TableSetupColumn("Index");
				ImGui::TableSetupColumn("U");
				ImGui::TableSetupColumn("S");
				ImGui::TableHeadersRow();

				// Render Rows
				int n_elements = (int)m_table.size();
				for (int i = 0; i < n_elements; ++i)
				{
					// Index
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(std::to_string(i).c_str());

					// Parameter
					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(std::to_string(m_table[i].m_parameter).c_str());

					// Distance
					ImGui::TableSetColumnIndex(2);
					ImGui::TextUnformatted(std::to_string(m_table[i].m_distance).c_str());
				}
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}
	}
}

template<typename T>
void distance_table::render_table(const T* curve)
{
	if (!m_render || m_table.empty())
		return;

	// Draw table points
	glm::vec3 offset(0.02f);
	glm::vec4 color(1.0f, 0.0f, 1.0f, 1.0f);
	int n_points = (int)m_table.size();
	for (int i = 0; i < n_points; ++i)
	{
		const glm::vec3& pos = curve->lerp(m_table[i].m_parameter);
		g_debug.debug_draw_aabb(pos - offset, pos + offset, color, false);
	}
}

template<typename T>
void distance_table::uniform_method(const T* curve)
{
	// Set the first element of the table
	set_step(m_step);
	m_table[0].m_distance = 0.0f;
	m_table[0].m_parameter = 0.0f;

	// Compute the table
	float tn = m_step;
	glm::vec3 prev = curve->lerp(0.0f);
	size_t n_elements = m_table.size();
	for (size_t i = 1; i < n_elements; ++i, tn += m_step)
	{
		// Get the current point
		glm::vec3 curr = curve->lerp(tn);

		// Set the parameter
		m_table[i].m_parameter = glm::clamp(tn, 0.0f, 1.0f);

		// Compute the distance
		float prev_dist = m_table[i - 1].m_distance;
		m_table[i].m_distance = prev_dist + glm::length(curr - prev);

		// Update positions
		prev = curr;
	}
}

template<typename T>
void distance_table::adaptive_method(const T* curve)
{
	// Initialize the list
	table_list list({
		table_element(0.0f, 0.0f), table_element(1.0f, 0.0f)
	});

	// Start recursive method
	adaptive_method_rec(curve, list, list.begin(), std::prev(list.end()), curve->lerp(0.0f), curve->lerp(1.0f), m_force_div, 0);

	// Copy result to vector
	int i = 0;
	list_it end = list.end();
	size_t n_elements = list.size();
	m_table.resize(n_elements);
	for (list_it it = list.begin(); it != end; ++it, ++i)
		m_table[i] = *it;
}

template<typename T>
void distance_table::uniform_imgui(const T* curve)
{
	ImGui::Separator();
	ImGui::SliderFloat("Step Value", &m_step, 0.001f, 0.05f);
}

template<typename T>
void distance_table::adaptive_imgui(const T* curve)
{
	ImGui::Separator();
	ImGui::DragInt("Forced Iterations", &m_force_div, 0.1f, 0, 20);
	ImGui::DragFloat("Base Epsilon", &m_epsilon, 0.1f, 0.1f, 50.0f);
}

template<typename T>
void distance_table::adaptive_method_rec(const T* curve, table_list& list, list_it l_it, list_it r_it, 
	const glm::vec3& l_point, const glm::vec3& r_point, int force_division, int level)
{
	// Compute parameter of midpoint
	float l_u = l_it->m_parameter;
	float r_u = r_it->m_parameter;
	float m_u = l_u + (r_u - l_u) / 2.0f;

	// Compute midpoint
	glm::vec3 m_point = curve->lerp(m_u);

	// Compute distances
	// A: Distance between left and mid
	// B: Distance between mid and right
	// C: Distance between left and right
	float A = glm::length(m_point - l_point);
	float B = glm::length(r_point - m_point);
	float C = glm::length(r_point - l_point);

	// Compute epsilon
	float error = m_epsilon / (float)glm::pow(2, level);

	// Check if division is necessary
	if (force_division > 0 || glm::abs(A + B - C) > error)
	{
		// Add midpoint in the list
		list_it m_it = list.insert(r_it, table_element(m_u, l_it->m_distance + A));

		// Update force division
		if (force_division > 0)
			--force_division;

		// Subdivide left
		adaptive_method_rec(curve, list, l_it, m_it, l_point, m_point, force_division, level + 1);

		// Update the right point distance in the table
		r_it->m_distance = m_it->m_distance + B;

		// Subdivide right
		adaptive_method_rec(curve, list, m_it, r_it, m_point, r_point, force_division, level + 1);
	}
}

void curve_controller::imgui()
{
	if (ImGui::CollapsingHeader("Curve Follow Controller"))
	{
		ImGui::Checkbox("Play", &m_play); ImGui::SameLine();
		ImGui::Checkbox("Loop", &m_loop); ImGui::SameLine();
		if (ImGui::Button("Reset"))
		{
			reset();
		}
		ImGui::SliderFloat("t parameter", &m_param, 0.0f, 1.0f);
		ImGui::Separator();

		if (ImGui::BeginCombo("Speed Control", m_cte_speed ? "Constant Speed" : "Distance-Time Function"))
		{
			if (ImGui::Selectable("Constant Speed"))
			{
				reset();
				m_cte_speed = true;
			}

			if (ImGui::Selectable("Distance-Time Function"))
			{
				reset();
				m_cte_speed = false;
			}

			ImGui::EndCombo();
		}

		if (m_cte_speed)
		{
			ImGui::DragFloat("Rate Of Travel", &m_rate_of_travel, 0.1f, 0.1f, 20.0f);
			ImGui::DragFloat("Ref Rate Of Travel", &m_ref_rate, 0.1f, 0.1f, 5.0f);
			ImGui::Separator();
			ImGui::Text(("Total Time To Reach The End: " + std::to_string(m_total_time) + "s").c_str());
			ImGui::Separator();
		}

		else
		{
			if (ImGui::DragFloat("Time To Travel", &m_total_time, 0.f, 0.5f, 20.0f))
			{
				m_timer = 0.0f;
				m_prev_dist = 0.0f;
			}
			ImGui::DragFloat("Ref Rate Of Travel", &m_ref_rate, 0.1f, 0.1f, 5.0f);
			if (ImGui::SliderFloat("t1", &m_t1, 0.0f, 1.0f))
				m_t1 = glm::clamp(m_t1, 0.0f, m_t2);
			if (ImGui::SliderFloat("t2", &m_t2, 0.0f, 1.0f))
				m_t2 = glm::clamp(m_t2, m_t1, 1.0f);

			ImGui::Separator();
			ImGui::Text(("Rate Of Travel: " + std::to_string(m_rate_of_travel) + "u/s").c_str());
			ImGui::Separator();
		}

		ImGui::Checkbox("Use Frenet Frame", &m_frenet); ImGui::SameLine();
		ImGui::Checkbox("Draw Frame", &m_draw_frame);
	}
}

template<typename T>
void curve_controller::update(const T* curve, const distance_table& table)
{
	// Update total dist of curve
	m_total_dist = table.get_total_dist();

	// Update controller
	if (m_cte_speed)
		cte_speed_update(table);
	else
		distance_time_func_update(table);

	// Set animation speed
	m_follower_anim->set_anim_factor(m_rate_of_travel / m_ref_rate);

	// Move follower along the curve
	move_follower(curve);
}

void curve_controller::set_follower(node* follower)
{
	m_follower = follower;
	m_follower_anim = follower->get_component<anim_comp>();
}

template<typename T>
void curve_controller::move_follower(const T* curve)
{
	// Set follower position
	glm::vec3 pos = curve->lerp(m_param);
	m_follower->m_local.set_position(pos);

	// Set follower orientation
	glm::vec3 fwd = glm::normalize(curve->first_derivative(m_param));
	glm::vec3 right;
	glm::vec3 up;

	// Using frenet frame
	if (m_frenet)
	{
		glm::vec3 second_der = glm::normalize(curve->second_derivative(m_param));
		up = glm::normalize(glm::cross(fwd, second_der));
		right = glm::normalize(glm::cross(up, fwd));
	}

	// Using y-axis
	else
	{
		right = glm::normalize(glm::cross(fwd, glm::vec3(0.0f, 1.0f, 0.0f)));
		up = glm::normalize(glm::cross(right, fwd));
	}

	// Set the quaternion
	glm::mat4 look = glm::lookAt(pos, pos - fwd, up);
	m_follower->m_local.set_rotation(glm::quat_cast(glm::inverse(look)));

	// Draw the frame
	if (m_draw_frame)
	{
		g_debug.debug_draw_line(pos, pos + fwd, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		g_debug.debug_draw_line(pos, pos + right, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		g_debug.debug_draw_line(pos, pos + up, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
	}
}

void curve_controller::reset()
{
	m_total_time = 5.0f;
	m_prev_dist = 0.0f;
	m_rate_of_travel = 1.0f;
	m_travelled = 0.0f;
	m_timer = 0.0f;
	m_param = 0.0f;
	m_ref_rate = 0.7f;
}

void curve_controller::cte_speed_update(const distance_table& table)
{
	m_total_time = m_total_dist / m_rate_of_travel;
	
	// Update t param
	if (m_play)
	{
		// Update parameter
		if (m_travelled <= m_total_dist)
			m_travelled += m_rate_of_travel * g_clock.dt();

		// Reset parameter
		if (m_loop && m_travelled > m_total_dist)
			m_travelled = 0.0f;

		// Set the normalized parameter
		m_param = table.get_parameter_from_dist(m_travelled);
	}
}

void curve_controller::distance_time_func_update(const distance_table& table)
{
	// Update t param
	if (m_play)
	{
		// Update parameter
		if (m_timer <= m_total_time)
			m_timer += g_clock.dt();

		// Reset parameter
		if (m_loop && m_timer > m_total_time)
		{
			m_prev_dist = 0.0f;
			m_timer = 0.0f;
		}

		// Set the normalized parameter
		float tn = m_timer / m_total_time;
		m_travelled = distance_time_function(tn) * m_total_dist;
		m_param = table.get_parameter_from_dist(m_travelled);
	}

	m_rate_of_travel = (m_travelled - m_prev_dist) / g_clock.dt();
	m_prev_dist = m_travelled;
}

float curve_controller::distance_time_function(float tn)
{
	float res;
	tn = glm::clamp(tn, 0.0f, 1.0f);
	if (tn < m_t1)
		res = ease_in(tn);
	else if (tn > m_t2)
		res = ease_out(tn);
	else
		res = linear(tn);

	return res / normalizer();
}

float curve_controller::ease_in(float tn)
{
	constexpr float cte1 = 2.0f / glm::pi<float>();
	constexpr float cte2 = 1.0f / cte1;
	return m_t1 * cte1 * (glm::sin((tn / m_t1) * cte2 - cte2) + 1);
}

float curve_controller::linear(float tn)
{
	return (tn - m_t1) + (2.0f * m_t1) / glm::pi<float>();
}

float curve_controller::ease_out(float tn)
{
	constexpr float cte1 = 2.0f / glm::pi<float>();
	constexpr float cte2 = 1.0f / cte1;
	float div = (tn - m_t2) / (1.0f - m_t2);
	return (1.0f - m_t2) * cte1 * glm::sin(div * cte2) + linear(m_t2);
}

float curve_controller::normalizer()
{
	constexpr float cte1 = glm::pi<float>();
	return 2.0f * (m_t1 / cte1) + m_t2 - m_t1 + 2.0f * ((1 - m_t2) / cte1);
}
}


