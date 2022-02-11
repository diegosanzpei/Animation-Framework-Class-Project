#include "blending.h"
#include "resources.h"
#include <imgui.h>
#include <algorithm>
#include "input.h"
#include "delaunator.h"

namespace cs460 {
// Produce an animation pose using the given animation and animation time
void produce_pos(const int model_idx, const int anim_idx, anim_pose& pose, float time)
{
	const animation* anim = &g_resources.get_model_rsc(model_idx).m_anims.at(anim_idx);

	// Get channel and samplers of the animation
	const auto& channels = anim->m_chanels;
	const auto& samplers = anim->m_samplers;

	// Infinite time
	time = time - glm::floor(time / anim->m_max_time) * anim->m_max_time;
	pose.clear();

	// Go through each channel
	size_t n_channels = channels.size();
	for (size_t i = 0; i < n_channels; ++i)
	{
		const auto& ch = channels[i];
		const auto& sp = samplers.at(ch.m_sampler);

		// Get the transform
		auto& t = pose[ch.m_node];

		// Set transform data
		if (ch.m_path_type == animation::channel::path_type::translation)
		{
			t.second |= target_type::translation;
			t.first.set_position(ch.lerp_pos(time, sp));
		}
		else if (ch.m_path_type == animation::channel::path_type::rotation)
		{
			t.second |= target_type::rotation;
			t.first.set_rotation(ch.lerp_rot(time, sp, true));
		}
		else if (ch.m_path_type == animation::channel::path_type::scale)
		{
			t.second |= target_type::scale;
			t.first.set_scale(ch.lerp_pos(time, sp));
		}
	}
}

// Blend the two animation poses using lerp
void blend_pose_lerp(const anim_pose& to, const anim_pose& from, anim_pose& res, float blend_param)
{
	// Prepare the result pose
	res.clear();

	// Get iterators to the end of each pose
	auto from_end = from.end();
	auto to_end = to.end();

	for (auto it = from.begin(); it != from_end; ++it)
		res[it->first];
	for (auto it = to.begin(); it != to_end; ++it)
		res[it->first];

	// Lerp poses
	auto res_end = res.end();
	for (auto it = res.begin(); it != res_end; ++it)
	{
		// Get the node index
		unsigned int node_idx = it->first;

		// Check if the transform data is not in from pose
		if (from.find(node_idx) == from_end)
			it->second = to.at(node_idx);

		// Check if the transform data is not in to pose
		else if (to.find(node_idx) == to_end)
			it->second = from.at(node_idx);

		// Otherwise, lerp from and to pose
		else
		{
			// Get info
			transform& res_tr = it->second.first;
			auto& res_flags = it->second.second;

			const transform& from_tr = from.at(node_idx).first;
			const auto& from_flags = from.at(node_idx).second;

			const transform& to_tr = to.at(node_idx).first;
			const auto& to_flags = to.at(node_idx).second;

			// Lerp translation:

			// Case 0: Lerp translation
			if ((from_flags & translation) != 0 && (to_flags & translation) != 0)
			{
				const glm::vec3& p0 = from_tr.get_position();
				const glm::vec3& p1 = to_tr.get_position();
				res_tr.set_position(p0 + (p1 - p0) * blend_param);
				res_flags |= target_type::translation;
			}

			// Case 1: Translation data only on from pose
			else if ((from_flags & translation) != 0)
			{
				res_tr.set_position(from_tr.get_position());
				res_flags |= translation;
			}

			// Case 2: Translation data only on to pose
			else if ((to_flags & translation) != 0)
			{
				res_tr.set_position(to_tr.get_position());
				res_flags |= translation;
			}

			// Lerp rotation:

			// Case 0: Lerp rotation
			if ((from_flags & rotation) != 0 && (to_flags & rotation) != 0)
			{
				const glm::quat& q0 = from_tr.get_rotation();
				const glm::quat& q1 = to_tr.get_rotation();
				res_tr.set_rotation(glm::normalize(glm::slerp(q0, q1, blend_param)));
				res_flags |= target_type::rotation;
			}

			// Case 1: Rotation data only on from pose
			else if ((from_flags & rotation) != 0)
			{
				res_tr.set_rotation(from_tr.get_rotation());
				res_flags |= rotation;
			}

			// Case 2: Rotation data only on to pose
			else if ((to_flags & rotation) != 0)
			{
				res_tr.set_rotation(to_tr.get_rotation());
				res_flags |= rotation;
			}

			// Lerp scale:

			// Case 0: Scale rotation
			if ((from_flags & scale) != 0 && (to_flags & scale) != 0)
			{
				const glm::vec3& s0 = from_tr.get_scale();
				const glm::vec3& s1 = to_tr.get_scale();
				res_tr.set_scale(s0 + (s1 - s0) * blend_param);
				res_flags |= target_type::scale;
			}

			// Case 1: Scale data only on from pose
			else if ((from_flags & scale) != 0)
			{
				res_tr.set_scale(from_tr.get_scale());
				res_flags |= scale;
			}

			// Case 2: Scale data only on to pose
			else if ((to_flags & scale) != 0)
			{
				res_tr.set_scale(to_tr.get_scale());
				res_flags |= scale;
			}
		}
	}
}

void barycentric_2_points_blend(anim_pose::const_iterator p0, anim_pose::const_iterator p1, float a0, float a1, anim_pose::iterator res)
{
	// Get info
	transform& res_tr = res->second.first;
	auto& res_flags = res->second.second;

	const transform& p0_tr = p0->second.first;
	const auto& p0_flags = p0->second.second;

	const transform& p1_tr = p1->second.first;
	const auto& p1_flags = p1->second.second;

	// Lerp translation:
	float tmp_a0 = ((p0_flags & translation) != 0) ? a0 : 0.0f;
	float tmp_a1 = ((p1_flags & translation) != 0) ? a1 : 0.0f;
	if (tmp_a0 != 0 || tmp_a1 != 0)
	{
		const glm::vec3& t0 = p0_tr.get_position();
		const glm::vec3& t1 = p1_tr.get_position();
		res_tr.set_position(tmp_a0 * t0 + tmp_a1 * t1);
		res_flags |= target_type::translation;
	}

	// Lerp Rotation:
	tmp_a0 = ((p0_flags & rotation) != 0) ? a0 : 0.0f;
	tmp_a1 = ((p1_flags & rotation) != 0) ? a1 : 0.0f;
	if (tmp_a0 != 0 || tmp_a1 != 0)
	{
		const glm::quat& q0 = p0_tr.get_rotation();
		const glm::quat& q1 = p1_tr.get_rotation();
		res_tr.set_rotation(glm::normalize(tmp_a0 * q0 + tmp_a1 * q1));
		res_flags |= target_type::rotation;
	}

	// Lerp Scale:
	tmp_a0 = ((p0_flags & scale) != 0) ? a0 : 0.0f;
	tmp_a1 = ((p1_flags & scale) != 0) ? a1 : 0.0f;
	if (tmp_a0 != 0 || tmp_a1 != 0)
	{
		const glm::vec3& s0 = p0_tr.get_scale();
		const glm::vec3& s1 = p1_tr.get_scale();
		res_tr.set_scale(tmp_a0 * s0 + tmp_a1 * s1);
		res_flags |= target_type::scale;
	}
}

void barycentric_3_points_blend(anim_pose::const_iterator p0, anim_pose::const_iterator p1, anim_pose::const_iterator p2, float a0, float a1, float a2, anim_pose::iterator res)
{
	// Get info
	transform& res_tr = res->second.first;
	auto& res_flags = res->second.second;

	const transform& p0_tr = p0->second.first;
	const auto& p0_flags = p0->second.second;

	const transform& p1_tr = p1->second.first;
	const auto& p1_flags = p1->second.second;

	const transform& p2_tr = p2->second.first;
	const auto& p2_flags = p2->second.second;

	// Lerp translation:
	float tmp_a0 = ((p0_flags & translation) != 0) ? a0 : 0.0f;
	float tmp_a1 = ((p1_flags & translation) != 0) ? a1 : 0.0f;
	float tmp_a2 = ((p2_flags & translation) != 0) ? a2 : 0.0f;
	if (tmp_a0 != 0 || tmp_a1 != 0 || tmp_a2 != 0)
	{
		const glm::vec3& t0 = p0_tr.get_position();
		const glm::vec3& t1 = p1_tr.get_position();
		const glm::vec3& t2 = p2_tr.get_position();
		res_tr.set_position(tmp_a0 * t0 + tmp_a1 * t1 + tmp_a2 * t2);
		res_flags |= target_type::translation;
	}

	// Lerp Rotation:
	tmp_a0 = ((p0_flags & rotation) != 0) ? a0 : 0.0f;
	tmp_a1 = ((p1_flags & rotation) != 0) ? a1 : 0.0f;
	tmp_a2 = ((p2_flags & rotation) != 0) ? a2 : 0.0f;
	if (tmp_a0 != 0 || tmp_a1 != 0 || tmp_a2 != 0)
	{
		const glm::quat& q0 = p0_tr.get_rotation();
		const glm::quat& q1 = p1_tr.get_rotation();
		const glm::quat& q2 = p2_tr.get_rotation();
		res_tr.set_rotation(glm::normalize(tmp_a0 * q0 + tmp_a1 * q1 + tmp_a2 * q2));
		res_flags |= target_type::rotation;
	}

	// Lerp Scale:
	tmp_a0 = ((p0_flags & scale) != 0) ? a0 : 0.0f;
	tmp_a1 = ((p1_flags & scale) != 0) ? a1 : 0.0f;
	tmp_a2 = ((p2_flags & scale) != 0) ? a2 : 0.0f;
	if (tmp_a0 != 0 || tmp_a1 != 0 || tmp_a2 != 0)
	{
		const glm::vec3& s0 = p0_tr.get_scale();
		const glm::vec3& s1 = p1_tr.get_scale();
		const glm::vec3& s2 = p2_tr.get_scale();
		res_tr.set_scale(tmp_a0 * s0 + tmp_a1 * s1 + tmp_a2 * s2);
		res_flags |= target_type::scale;
	}
}

// Blend the two animation poses using barycentric coordinates
void blend_pose_barycentric(const anim_pose& p0, const anim_pose& p1, const anim_pose& p2, float a0, float a1, float a2, anim_pose& res)
{
	// Prepare the result pose
	res.clear();

	// Get iterators to the end of each pose
	auto p0_end = p0.end();
	auto p1_end = p1.end();
	auto p2_end = p2.end();

	// Initialize the result pose
	for (auto it = p0.begin(); it != p0_end; ++it)
		res[it->first];
	for (auto it = p1.begin(); it != p1_end; ++it)
		res[it->first];
	for (auto it = p2.begin(); it != p2_end; ++it)
		res[it->first];

	// Barycentric blend
	auto end = res.end();
	for (auto it = res.begin(); it != end; ++it)
	{
		unsigned int node_idx = it->first;

		// Find the poses
		auto pose_0 = p0.find(node_idx);
		auto pose_1 = p1.find(node_idx);
		auto pose_2 = p2.find(node_idx);

		// Case 0: Transform data only in pose 0
		if (pose_1 == p1_end && pose_2 == p2_end)
			it->second = pose_0->second;

		// Case 1: Transform data only in pose 1
		else if (pose_0 == p0_end && pose_2 == p2_end)
			it->second = pose_1->second;

		// Case 2: Transform data only in pose 2
		else if (pose_0 == p0_end && pose_1 == p1_end)
			it->second = pose_1->second;

		// Case 3: Transform data only in pose 0 and 1
		else if (pose_2 == p2_end)
			barycentric_2_points_blend(pose_0, pose_1, a0, a1, it);

		// Case 4: Transform data only in pose 1 and 2
		else if (pose_0 == p0_end)
			barycentric_2_points_blend(pose_1, pose_2, a1, a2, it);

		// Case 5: Transform data only in pose 2 and 0
		else if (pose_1 == p1_end)
			barycentric_2_points_blend(pose_2, pose_0, a2, a0, it);

		// Case 2: Blend Transform data
		else
			barycentric_3_points_blend(pose_0, pose_1, pose_2, a0, a1, a2, it);
	}
}

// Blend between the current poses of the children nodes to obtain the current pose of the node
void blend_node::produce_pose(glm::vec2& blend_param, float time)
{
	if (m_children.empty())
		produce_pos(m_model, m_anim, m_current_pose, time);

	else
		blend_children(blend_param, time);
}

// Finds the two child blend nodes that encapsulate the given parameter
void blend_node_1d::find_segment(float param, blend_node*& to, blend_node*& from)
{
	// Find the index of the segments
	int left_node = 0;
	int right_node = (int)m_children.size() - 1;
	find_segment_rec(param, &left_node, &right_node);

	// Set the segment
	from = m_children[left_node];
	to = m_children[right_node];
}

void blend_node_1d::sort_childs()
{
	std::sort(m_children.begin(), m_children.end(), [](const blend_node* lhs, const blend_node* rhs) {
		return lhs->m_blend_pos.x < rhs->m_blend_pos.x;
	});
}

void blend_node_1d::erase_child(blend_node* node)
{
	auto it = std::find(m_children.begin(), m_children.end(), node);
	if (it != m_children.end())
		m_children.erase(it);
}

void blend_node_1d::blend_graph(const glm::vec2& blend_param)
{
	size_t n_childs = m_children.size();

	// Draw List
	if (n_childs >= 2)
	{
		// Child contains the draw list
		ImGui::BeginChild("draw list", ImVec2(ImGui::GetWindowWidth() - 20.0f, 50.0f), true);
		ImGui::EndChild();

		// Set up the draw list
		ImVec2 min = ImGui::GetItemRectMin();
		ImVec2 max = ImGui::GetItemRectMax();
		ImVec2 size(max.x - min.x, max.y - min.y);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->PushClipRect(min, max);
		draw_list->AddRectFilled(min, max, IM_COL32(100, 100, 100, 100));

		// Blend node space
		float mid = min.y + (max.y - min.y) / 2.0f;
		ImVec2 start(min.x + 10.0f, mid);
		ImVec2 end  (max.x - 10.0f, mid);
		float diff = end.x - start.x;

		ImU32 clip_color = IM_COL32(200, 100, 200, 150);
		ImU32 time_line_color = IM_COL32(200, 200, 200, 170);
		ImU32 bp_color = IM_COL32(255, 000, 000, 180);

		// Draw the first and last clip
		draw_list->AddLine(start, end, time_line_color, 0.5f);
		draw_list->AddCircleFilled(start, 5.0f, clip_color);
		draw_list->AddCircleFilled(end, 5.0f, clip_color);

		// Draw the rest of the clips
		size_t last_child = n_childs - 1;
		float bp_min = m_children.front()->m_blend_pos.x;
		float bp_max = m_children.back()->m_blend_pos.x;
		float bp_diff = bp_max - bp_min;
		for (size_t i = 1; i < last_child; ++i)
		{
			float bp = m_children[i]->m_blend_pos.x;
			float n = (bp - bp_min) / bp_diff;
			ImVec2 clip_pos(start.x + n * diff, mid);
			draw_list->AddCircleFilled(clip_pos, 5.0f, clip_color);
		}

		// Draw the blend parameter
		float n = (blend_param.x - bp_min) / bp_diff;
		ImVec2 bp_pos(start.x + n * diff, mid);
		draw_list->AddCircleFilled(bp_pos, 5.0f, bp_color);
		draw_list->PopClipRect();
	}
}

bool blend_node_1d::enough_blend_nodes()
{
	return m_children.size() >= 2;
}

void blend_node_1d::get_min_max_blend_param(glm::vec2& min, glm::vec2& max)
{
	if (m_children.size() < 2)
	{
		min = glm::vec2(0.0f);
		max = glm::vec2(1.0f);
	}

	else
	{
		min = m_children.front()->m_blend_pos;
		max = m_children.back()->m_blend_pos;
	}
}

// Binary search helper function
bool blend_node_1d::find_segment_rec(float param, int* left, int* right)
{
	if (*left == *right - 1)
		return true;

	// Get the middle blend node
	int mid = *left + (*right - *left) / 2;

	float mid_param = m_children[mid]->m_blend_pos.x;

	// Found the blend node
	if (param == mid_param)
	{
		*left = mid;
		*right = mid;
		return true;
	}

	// Recurse to the left
	if (param < mid_param)
	{
		*right = mid;
		return find_segment_rec(param, left, right);
	}

	// Otherwise, recurse to the right
	else
	{
		*left = mid;
		return find_segment_rec(param, left, right);
	}

	return false;
}

void blend_node_1d::blend_children(glm::vec2& blend_param, float time)
{
	// Clamp the blend parameter
	float min = m_children[0]->m_blend_pos.x;
	float max = m_children.back()->m_blend_pos.x;
	blend_param.x = glm::clamp(blend_param.x, min, max);

	// Get the child blend nodes to lerp in between with
	blend_node *to, *from;
	find_segment(blend_param.x, to, from);

	// Produce the current pose of the child blend nodes
	to->produce_pose(blend_param, time);
	from->produce_pose(blend_param, time);

	// Compute the normalized blend parameter
	float local_diff = blend_param.x - from->m_blend_pos.x;
	float total_diff = to->m_blend_pos.x - from->m_blend_pos.x;

	float norm_param = (total_diff != 0) ? local_diff / total_diff : 0.0f;

	// Lerp the poses to produce the pose of this node
	blend_pose_lerp(to->m_current_pose, from->m_current_pose, m_current_pose, norm_param);
}

void blend_node_1d::insert_node(int model_idx, int anim_idx, const glm::vec2& blend_pos)
{
	// Create the node
	blend_node_1d* node = new blend_node_1d;
	node->m_blend_pos = blend_pos;
	node->m_model = model_idx;
	node->m_anim = anim_idx;
	node->m_parent = this;

	// Add the node
	m_children.push_back(node);

	// Sort the children
	sort_childs();
}

void blend_node_1d::imgui(int tree_level, int child_id, blend_node*& selected)
{
	size_t n_childs = m_children.size();

	// Set the tree node flags
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
	if (tree_level != 0) { flags |= ImGuiTreeNodeFlags_OpenOnArrow; }
	if (selected == this) { flags |= ImGuiTreeNodeFlags_Selected; }

	bool open = ImGui::TreeNodeEx(("1D Blend Node " + std::to_string(tree_level) + std::to_string(child_id)).c_str(), flags);

	// Check if the current node has been selected
	if (ImGui::IsItemClicked() && tree_level != 0)
	{
		selected = this;
	}

	if (open)
	{
		// Leaf node
		if (n_childs == 0 && tree_level != 0)
		{
			// Get the animations of the model
			const std::vector<animation>& anims = g_resources.get_model_rsc(m_model).m_anims;

			// Show all the available animations for the node
			ImGui::Text("Node Animation:");
			if (ImGui::BeginCombo("node anim", anims.at(m_anim).m_name.c_str()))
			{
				size_t n_anims = anims.size();
				for (size_t i = 0; i < n_anims; ++i)
				{
					if (ImGui::Selectable(anims[i].m_name.c_str()))
					{
						m_anim = (int)i;
						break;
					}
				}
				ImGui::EndCombo();
			}

			// Show the blend position
			ImGui::Text("Node Blend Position:");
			if (ImGui::InputFloat("blend pos", &m_blend_pos.x, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
				m_parent->sort_childs();
		}

		// Root node
		else
		{
			// Add a blend node child
			if (ImGui::Button("Add Blend Node"))
			{
				if (n_childs == 0)
					insert_node(m_model, 0, glm::vec2(0.0f, 0.0f));
				else
					insert_node(m_model, 0, m_children.back()->m_blend_pos + glm::vec2(1.0f, 0.0f));
			}

			// Show gui of the child nodes
			else
			{
				for (size_t i = 0; i < n_childs; ++i)
					m_children[i]->imgui(tree_level + 1, (int)i, selected);
			}
		}

		ImGui::TreePop();
	}
}

void blend_node_2d::generate_triangles()
{
	if (!enough_blend_nodes())
		return;

	// Set the input vector for triangulation
	size_t n_nodes = m_children.size();
	size_t n_coords = n_nodes * 2;
	std::vector<double> coords(n_coords);
	for (size_t node = 0; node < n_nodes; ++node)
	{
		// Get the blend position
		const glm::vec2& coord = m_children[node]->m_blend_pos;

		// Set the blend position in the input vector
		size_t coord_idx = node * 2;
		coords[coord_idx] = coord.x;
		coords[coord_idx + 1] = coord.y;
	}

	// Triangulation with Delaunay
	delaunator::Delaunator del(coords);

	// Save the triangle indices
	size_t size = del.triangles.size();
	size_t n_triangles = size / 3;
	m_triangles.resize(n_triangles);
	for (size_t i = 0; i < n_triangles; ++i)
	{
		size_t idx = 3 * i;
		m_triangles[i][0] = (unsigned int)del.triangles[idx];
		m_triangles[i][1] = (unsigned int)del.triangles[idx + 1];
		m_triangles[i][2] = (unsigned int)del.triangles[idx + 2];
	}
}

void blend_node_2d::find_nodes_barycentric(const glm::vec2& blend_param, blend_node*& n0, blend_node*& n1, blend_node*& n2, float& a0, float& a1, float& a2)
{
	// Number of triangles
	size_t n_triangles = m_triangles.size();

	// Check with each triangle
	for (size_t i = 0; i < n_triangles; ++i)
	{
		const auto& t_idx = m_triangles[i];

		// Get the blend nodes that form the triangle
		blend_node* bn0 = m_children[t_idx[0]];
		blend_node* bn1 = m_children[t_idx[1]];
		blend_node* bn2 = m_children[t_idx[2]];

		// Get the positions of each node
		const glm::vec2& p0 = bn0->m_blend_pos;
		const glm::vec2& p1 = bn1->m_blend_pos;
		const glm::vec2& p2 = bn2->m_blend_pos;

		// Check if parameter is inside the triangle
		if (compute_barycentric(blend_param, p0, p1, p2, a0, a1, a2))
		{
			// Set blend node pointers
			n0 = bn0; n1 = bn1; n2 = bn2;
			m_inside_triangle = (int)i;
			return;
		}
	}

	// Blend parameter is not inside of any triangle
	m_inside_triangle = -1;
}

void blend_node_2d::blend_children(glm::vec2& blend_param, float time)
{
	// Clamp the blend parameter
	blend_param.x = glm::clamp(blend_param.x, m_min.x, m_max.x);
	blend_param.y = glm::clamp(blend_param.y, m_min.y, m_max.y);

	blend_node* n0, * n1, * n2;
	float a0, a1, a2;
	find_nodes_barycentric(blend_param, n0, n1, n2, a0, a1, a2);

	// Produce the pose of each node
	if (m_inside_triangle >= 0)
	{
		n0->produce_pose(blend_param, time);
		n1->produce_pose(blend_param, time);
		n2->produce_pose(blend_param, time);

		// Lerp the poses to produce the pose of this node
		blend_pose_barycentric(
			n0->m_current_pose, n1->m_current_pose, n2->m_current_pose,
			a0, a1, a2, m_current_pose);
	}
	else
		m_current_pose.clear();
}

void blend_node_2d::insert_node(int model_idx, int anim_idx, const glm::vec2& blend_pos)
{
	// Create the node
	blend_node_2d* node = new blend_node_2d;
	node->m_blend_pos = blend_pos;
	node->m_anim = anim_idx;
	node->m_model = model_idx;
	node->m_parent = this;

	// Update min max values
	m_min.x = glm::min(m_min.x, blend_pos.x);
	m_min.y = glm::min(m_min.y, blend_pos.y);
	m_max.x = glm::max(m_max.x, blend_pos.x);
	m_max.y = glm::max(m_max.y, blend_pos.y);

	// Add the node
	m_children.push_back(node);

	// Sort the children
	generate_triangles();
}

void blend_node_2d::imgui(int tree_level, int child_id, blend_node*& selected)
{
	size_t n_childs = m_children.size();

	// Set the tree node flags
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
	if (tree_level != 0) { flags |= ImGuiTreeNodeFlags_OpenOnArrow; }
	if (selected == this) { flags |= ImGuiTreeNodeFlags_Selected; }

	bool open = ImGui::TreeNodeEx(("2D Blend Node " + std::to_string(tree_level) + std::to_string(child_id)).c_str(), flags);

	// Check if the current node has been selected
	if (ImGui::IsItemClicked() && tree_level != 0)
	{
		selected = this;
	}

	if (open)
	{
		// Leaf node
		if (n_childs == 0 && tree_level != 0)
		{
			// Get the animations of the model
			const std::vector<animation>& anims = g_resources.get_model_rsc(m_model).m_anims;

			// Show all the available animations for the node
			ImGui::Text("Node Animation:");
			if (ImGui::BeginCombo("node anim", anims.at(m_anim).m_name.c_str()))
			{
				size_t n_anims = anims.size();
				for (size_t i = 0; i < n_anims; ++i)
				{
					if (ImGui::Selectable(anims[i].m_name.c_str()))
					{
						m_anim = (int)i;
						break;
					}
				}
				ImGui::EndCombo();
			}

			// Show the blend position
			ImGui::Text("Node Blend Position:");
			if (ImGui::InputFloat2("blend pos", &m_blend_pos.x, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
			{
				// Update min max values
				m_parent->update_min_max_blend_param();
				static_cast<blend_node_2d*>(m_parent)->generate_triangles();
			}
		}

		// Root node
		else
		{
			// Add a blend node child
			if (ImGui::Button("Add Blend Node"))
			{
				if (n_childs == 0)
					insert_node(m_model, 0, glm::vec2(0.0f, 0.0f));
				else
					insert_node(m_model, 0, glm::vec2(0.0f));
			}

			// Show gui of the child nodes
			else
			{
				for (size_t i = 0; i < n_childs; ++i)
					m_children[i]->imgui(tree_level + 1, (int)i, selected);
			}
		}

		ImGui::TreePop();
	}
}

void blend_node_2d::erase_child(blend_node* node)
{
	if (m_children.size() <= 3)
		return;

	auto it = std::find(m_children.begin(), m_children.end(), node);
	if (it != m_children.end())
		m_children.erase(it);
	
	update_min_max_blend_param();
	generate_triangles();
}

void blend_node_2d::blend_graph(const glm::vec2& blend_param)
{
	size_t n_childs = m_children.size();

	// Draw List
	if (n_childs >= 3)
	{
		// Child contains the draw list
		ImGui::BeginChild("draw list", ImVec2(ImGui::GetWindowWidth() - 50.0f, 180.0f), true);
		ImGui::EndChild();

		// Set up the draw list
		ImVec2 min = ImGui::GetItemRectMin();
		ImVec2 max = ImGui::GetItemRectMax();
		ImVec2 size(max.x - min.x, max.y - min.y);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->PushClipRect(min, max);
		draw_list->AddRectFilled(min, max, IM_COL32(100, 100, 100, 100));

		// Blend node space
		glm::vec2 start(min.x + 10.0f, min.y + 10.0f);
		glm::vec2 end(max.x - 10.0f, max.y - 10.0f);
		glm::vec2 diff(end.x - start.x, end.y - start.y);
		glm::vec2 bp_diff = m_max - m_min;

		// Blend parameter pos
		glm::vec2 n((blend_param.x - m_min.x) / bp_diff.x, (blend_param.y - m_min.y) / bp_diff.y);
		ImVec2 bp_pos(start.x + n.x * diff.x, start.y + (1.0f - n.y) * diff.y);

		ImU32 clip_color = IM_COL32(200, 100, 200, 255);
		ImU32 line_color = IM_COL32_WHITE;
		ImU32 inside_triangle_color = IM_COL32(0, 100, 80, 100);
		ImU32 bp_color = IM_COL32(255, 000, 000, 255);

		// Compute the positions in the graph
		size_t n_nodes = m_children.size();
		std::vector<ImVec2> node_pos(n_nodes);
		for (int i = 0; i < n_nodes; ++i)
		{
			n.x = (m_children[i]->m_blend_pos.x - m_min.x) / bp_diff.x;
			n.y = (m_children[i]->m_blend_pos.y - m_min.y) / bp_diff.y;

			node_pos[i].x = start.x + n.x * diff.x;
			node_pos[i].y = start.y + (1.0f - n.y) * diff.y;

			// Draw the node
			draw_list->AddCircleFilled(node_pos[i], 5, clip_color);
		}

		// Draw the triangles
		size_t n_triangles = m_triangles.size();
		for (size_t i = 0; i < n_triangles; ++i)
		{
			// Get the triangles
			const auto& t = m_triangles[i];

			if (i == m_inside_triangle)
				draw_list->AddTriangleFilled(node_pos[t[0]], node_pos[t[1]], node_pos[t[2]], inside_triangle_color);
			
			draw_list->AddTriangle(node_pos[t[0]], node_pos[t[1]], node_pos[t[2]], line_color);
		}

		// Draw the blend parameter
		draw_list->AddCircleFilled(bp_pos, 5, bp_color);

		draw_list->PopClipRect();
	}
}

bool blend_node_2d::enough_blend_nodes()
{
	return m_children.size() >= 3;
}

void blend_node_2d::get_min_max_blend_param(glm::vec2& min, glm::vec2& max)
{
	min = m_min;
	max = m_max;
}

void blend_node_2d::update_min_max_blend_param()
{
	size_t n_children = m_children.size();
	if (n_children == 0)
		return;

	// Initialize min, max
	m_min = m_children.front()->m_blend_pos;
	m_max = m_min;
	
	// Search for the min and max params
	for (size_t i = 1; i < n_children; ++i)
	{
		blend_node* child = m_children[i];

		m_min.x = glm::min(m_min.x, child->m_blend_pos.x);
		m_min.y = glm::min(m_min.y, child->m_blend_pos.y);
		m_max.x = glm::max(m_max.x, child->m_blend_pos.x);
		m_max.y = glm::max(m_max.y, child->m_blend_pos.y);
	}
}

// Performs just the cross product for the z-axis
float blend_node_2d::cross_product(const glm::vec2& v0, const glm::vec2& v1)
{
	return (v0.x * v1.y - v0.y * v1.x);
}

bool blend_node_2d::compute_barycentric(const glm::vec2& point, const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, float& a0, float& a1, float& a2)
{
	// Compute the area of the triangle
	float total_area = glm::abs(cross_product(p2 - p0, p1 - p0));

	const glm::vec2& pp0 = p0 - point;
	const glm::vec2& pp1 = p1 - point;
	const glm::vec2& pp2 = p2 - point;

	a0 = cross_product(pp2, pp1) / total_area;
	a1 = cross_product(pp0, pp2) / total_area;
	a2 = cross_product(pp1, pp0) / total_area;

	return (a0 >= 0.0f && a1 >= 0.0f && a2 >= 0.0f);
}

// Free the memory of the entire blend tree
void blend_tree::destroy()
{
	if (m_root)
		destroy_rec(m_root);

	m_root = nullptr;
}

void blend_tree::imgui()
{
	if (!m_root)
		return;
	
	// Delete selected node
	if (m_selected && g_input.keyIsPressed(keyboard::key_delete))
	{
		m_root->erase_child(m_selected);
		m_selected = nullptr;
	}

	ImGui::Begin("Blend Editor");

	// Blend parameter
	ImGui::Text("Blend Parameter: ");
	glm::vec2 min, max;
	m_root->get_min_max_blend_param(min, max);
	ImGui::SliderFloat("x", &m_blend_param.x, min.x, max.x);
	ImGui::SliderFloat("y", &m_blend_param.y, min.y, max.y);
	m_blend_param.x = glm::clamp(m_blend_param.x, min.x, max.x);
	m_blend_param.y = glm::clamp(m_blend_param.y, min.y, max.y);

	// Draw list
	ImGui::Separator();
	ImGui::Text("Blend Graph:");
	m_root->blend_graph(m_blend_param);
	ImGui::Separator();

	// Blend Tree
	if (ImGui::TreeNode("Blend Tree"))
	{
		m_root->imgui(0, 0, m_selected);
		ImGui::TreePop();
	}
	ImGui::End();
}

const anim_pose* blend_tree::produce_pose(float time)
{
	if (!m_root || !m_root->enough_blend_nodes())
	{
		return nullptr;
	}

	// Produce the final pose
	m_root->produce_pose(m_blend_param, time);
	return &m_root->m_current_pose;
}

void blend_tree::create_1d_blend_tree()
{
	destroy();
	m_root = new blend_node_1d;
}

void blend_tree::create_2d_blend_tree()
{
	destroy();
	m_root = new blend_node_2d;
}

void blend_tree::insert_blend_node(int model_idx, int anim_idx, const glm::vec2& blend_pos)
{
	// Insert the node
	m_root->m_model = model_idx;
	m_root->insert_node(model_idx, anim_idx, blend_pos);
}

void blend_tree::destroy_rec(blend_node* node)
{
	size_t n_children = node->m_children.size();
	for (size_t i = 0; i < n_children; ++i)
		destroy_rec(node->m_children[i]);
	delete node;
}
}