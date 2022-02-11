#include "inverse_kinematics.h"
#include "node.h"
#include <imgui.h>
#include "debug.h"
#include "editor.h"
#include "scene_graph.h"

namespace cs460 {
void ik_base::initialize()
{
}

void ik_base::update()
{
	// Check if target is moving
	const glm::vec3& t = get_owner()->m_world.get_position();
	m_target_moving = (t != m_prev_pos);
	m_prev_pos = t;

	// Start processing
	if (m_target_moving)
	{
		m_status = processing;
		m_counter = 0;
	}

	// Try to reach the target
	if (m_status == processing)
		move_joints();

	// Debug draw
	debug_draw_target();
	if (m_render_bones)
		debug_draw_bones();
}

void ik_base::destroy()
{
}

void ik_base::imgui()
{
	ImGui::Text("IK Solver: ");
	ImGui::Text("Status: "); ImGui::SameLine();

	ImGui::Separator();

	if (m_status == processing)
		ImGui::Text("Processing");

	else if (m_status == success)
		ImGui::Text("Success");

	else
		ImGui::Text(" Failure");

	ImGui::Text("Distance Threshold");
	ImGui::SliderFloat("dt", &m_threshold, 0.01f, 1.0f);

	ImGui::Text("Max Iterations Per Frame");
	if (ImGui::InputInt("it", &m_iterations))
		m_iterations = glm::clamp(m_iterations, 1, 10000);

	ImGui::Separator();

	ImGui::Text("Manipulator:");
	if (ImGui::Button("Add Joint"))
		add_joint();

	else if (ImGui::Button("Remove Joint"))
		remove_joint();

	ImGui::Separator();

	ImGui::Text("Link Lengths:");
	size_t n_joints = m_joints.size();
	for (size_t i = 1; i < n_joints; ++i)
	{
		if (ImGui::SliderFloat(("Link" + std::to_string(i)).c_str(), &m_dists[i], 0.01f, 5.0f))
		{
			glm::vec3 v = glm::normalize(m_joints[i]->m_local.get_position());
			m_joints[i]->m_local.set_position(m_dists[i] * v);
		}
	}
	
}

void ik_base::clear_joints()
{
	m_joints.clear();
	m_dists.clear();
}

void ik_base::add_joint(node* joint)
{
	// Add joint
	m_joints.push_back(joint);

	// Compute distance
	const glm::vec3& j = joint->m_world.get_position();
	const glm::vec3& p = joint->m_parent->m_world.get_position();
	m_dists.push_back(glm::length(j - p));
}

void ik_base::default_init()
{
	// Default ik with 5 nodes
	for (int i = 0; i < 5; ++i)
		add_joint();

	get_owner()->m_local.set_position(glm::vec3(5.0f, 0.0f, 0.0f));
	get_owner()->m_world.set_position(glm::vec3(5.0f, 0.0f, 0.0f));
	m_prev_pos = glm::vec3(5.0f, 0.0f, 0.0f);
}

void ik_base::debug_draw_bones()
{
	// Draw the bones
	size_t last_joint = m_joints.size() - 1;
	for (size_t i = 0; i < last_joint; ++i)
	{
		const glm::vec3& j0 = m_joints[i]->m_world.get_position();
		const glm::vec3& j1 = m_joints[i + 1]->m_world.get_position();
		float offset = 0.01f;
		float size = 0.03f;

		glm::vec3 fwd = glm::normalize(j1 - j0);

		g_debug.debug_draw_aabb(j1 - glm::vec3(size), j1 + glm::vec3(size), m_color2, false);
		g_debug.debug_draw_bone(j0 - fwd * offset, j1 + fwd * offset, glm::vec4(1.0f));

		if (m_status == processing)
			g_debug.debug_draw_bone(j0, j1, m_color_process, false);
		else if (m_status == failure)
			g_debug.debug_draw_bone(j0, j1, m_color3, false);
		else
			g_debug.debug_draw_bone(j0, j1, m_color, false);
	}
}

void ik_base::debug_draw_target()
{
	float size = 0.1f;
	float size2 = 0.105f;
	glm::vec3 target = get_owner()->m_world.get_position();
	g_debug.debug_draw_aabb(target - glm::vec3(size), target + glm::vec3(size), m_color3, false);
	g_debug.debug_draw_aabb(target - glm::vec3(size2), target + glm::vec3(size2), glm::vec4(1.0f), true);
}

void ik_base::add_joint()
{
	size_t size = m_joints.size();
	node* parent = (size == 0) ? g_scene.get_root() : m_joints.back();
	
	// Create node
	node* n = new node;
	n->m_name = "joint" + std::to_string(size);
	
	float dist = (size != 0) ? 1.0f : 0.0f;
	n->m_local.set_position(glm::vec3(dist, 0.0f, 0.0f));
	
	// Attach node to parent
	parent->add_child(n);
	
	// Save pointer to node
	m_joints.push_back(n);
	
	n->m_world = parent->m_world.concatenate(n->m_local);
	m_dists.push_back(glm::length(parent->m_world.get_position() - n->m_world.get_position()));
}

void ik_base::remove_joint()
{
	size_t n_joints = m_joints.size();
	if (n_joints <= 3)
		return;

	// Remove children
	node* parent = m_joints[n_joints - 2];
	parent->m_children.clear();
	
	// Free memory
	delete m_joints.back();
	m_joints.pop_back();
}

bool ik_base::solution_found()
{
	// Get target and end effector positions
	const glm::vec3& t = get_owner()->m_world.get_position();
	const glm::vec3& e = m_joints.back()->m_world.get_position();
	
	const glm::vec3& v = t - e;
	if (glm::dot(v, v) <= m_threshold * m_threshold)
	{
		m_status = success;
		return true;
	}

	return false;
}

glm::quat ik_base::axis_angle_to_quat(const glm::vec3& v1, const glm::vec3& v2)
{
	// Compute angle between vectors
	float dot = glm::dot(v1, v2);
	float theta = glm::acos(glm::clamp(dot, -1.0f, 1.0f));

	// Compute rotation axis
	dot = glm::abs(dot);
	float epsilon = 0.00001f;
	glm::vec3 r = (dot + epsilon < 1.0f) ? glm::cross(v1, v2) : glm::vec3(0.0f, 0.0f, 1.0f);

	// Convert to quaternion
	return glm::angleAxis(theta, glm::normalize(r));
}

void ik_base::update_world_transforms(int start_idx)
{
	size_t n_joints = m_joints.size();
	for (size_t i = start_idx; i < n_joints; ++i)
	{
		node* update = m_joints[i];
		node* parent = update->m_parent;

		if (!parent)
			update->m_world = update->m_local;
		else
			update->m_world = parent->m_world.concatenate(update->m_local);
	}
}

void ccd::move_joints()
{
	// Target pos
	const glm::vec3& t = get_owner()->m_world.get_position();

	// End effector
	const node* end_effector = m_joints.back();

	float min_dist = m_threshold * m_threshold;

	int n_joints = (int)m_joints.size();
	for (int i = n_joints - 2; i >= 0; --i)
	{
		// End effector pos
		const glm::vec3& e = end_effector->m_world.get_position();

		// Current joint pos
		node* joint = m_joints[i];
		const glm::vec3& j = joint->m_world.get_position();

		// Compute vectors
		const glm::vec3& v1 = glm::normalize(e - j);
		const glm::vec3& v2 = glm::normalize(t - j);

		// Compute axis-angle rotation in quaternion form
		glm::quat q = axis_angle_to_quat(v1, v2);
		
		// Apply rotation in local space
		joint->m_local.set_rotation(q * joint->m_local.get_rotation());

		// Update world transform of the childs
		update_world_transforms(i);

		// Check if soulution has been found
		if (solution_found())
			return;
	}

	// Update iteration counter
	++m_counter;

	// Check if last iteration failed
	if (m_counter >= m_iterations)
		m_status = failure;
}

void ccd::imgui()
{
	if (!m_use_gui)
		return;

	bool open = true;
	ImGui::Begin("Cyclic Coordinate Descent IK", &open, ImGuiWindowFlags_NoMove);
	ik_base::imgui();
	ImGui::End();
}

void fabrik::clear_joints()
{
	m_positions.clear();
	ik_base::clear_joints();
}

void fabrik::add_joint(node* joint)
{
	ik_base::add_joint(joint);
	m_positions.emplace_back();
}

void fabrik::imgui()
{
	if (!m_use_gui)
		return;

	bool open = true;
	ImGui::Begin("FABRIK", &open, ImGuiWindowFlags_NoMove);
	ik_base::imgui();
	ImGui::End();
}

void fabrik::move_joints()
{
	int n_joints = (int)m_joints.size();

	// Get the target position
	const glm::vec3& t = get_owner()->m_world.get_position();
	float min_dist = m_threshold * m_threshold;
	int last_joint = n_joints - 1;

	// Pessimistic initialization of solver status
	m_status = failure;

	// Make a copy of the node positions
	for (int i = 0; i < n_joints; ++i)
		m_positions[i] = m_joints[i]->m_world.get_position();

	for (int it = 0; it < m_iterations; ++it)
	{
		// Forward step:

		// Set the end effector to the target position
		m_positions[last_joint] = t;

		// Iterate through the manipulator
		for (int i = n_joints - 1; i > 0; --i)
		{
			// Get the distance of the bone
			float dist = m_dists[i];

			// Compute vector from joint to parent joint
			glm::vec3& joint = m_positions[i];
			glm::vec3& parent = m_positions[i - 1];
			glm::vec3 v = glm::normalize(parent - joint);

			// Update the parent position
			parent = joint + dist * v;
		}

		// Backward step:

		// Restore original position of root joint
		m_positions[0] = m_joints[0]->m_world.get_position();

		// Iterate through the manipulator
		for (int i = 0; i < last_joint; ++i)
		{
			// Get the distance of the bone
			float dist = m_dists[i + 1];

			// Compute vector from parent to child joint
			glm::vec3& parent = m_positions[i];
			glm::vec3& joint = m_positions[i + 1];
			glm::vec3 v = glm::normalize(joint - parent);

			// Update joint position
			joint = parent + dist * v;
		}

		// Check if solution has been found
		if (solution_found())
			break;
	}

	// Update joint orientations
	for (int i = 0; i < last_joint; ++i)
	{
		// Get the joint position in world
		node* joint = m_joints[i];
		const glm::vec3& joint_pos = joint->m_world.get_position();

		// Get the old child position in world
		const glm::vec3& child_pos = m_joints[i + 1]->m_world.get_position();

		// Get the new child position in world
		const glm::vec3& new_child_pos = m_positions[i + 1];

		// Vectors from joint to child positions
		glm::vec3 v1 = glm::normalize(child_pos - joint_pos);
		glm::vec3 v2 = glm::normalize(new_child_pos - joint_pos);

		// Compute axis-angle rotation in quaternion form
		glm::quat q = axis_angle_to_quat(v1, v2);

		// Apply rotation in world space
		joint->m_world.set_rotation(q * joint->m_world.get_rotation());

		// Update the local transform
		joint->m_local = joint->m_parent->m_world.inv_concatenate(joint->m_world);

		// Update world transform of the childs
		update_world_transforms(i);
	}
}

void fabrik::add_joint()
{
	ik_base::add_joint();
	m_positions.emplace_back();
}

void fabrik::remove_joint()
{
	if (m_joints.size() <= 3)
		return;

	ik_base::remove_joint();
	m_positions.pop_back();
}
}