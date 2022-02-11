/**
* @file transform.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include "transform.h"
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>

namespace cs460 {
transform::transform()
	: m_position(0.0f)
	, m_rotation(1.0f, 0.0f, 0.0f, 0.0f)
	, m_scale(1.0f)
{}

void transform::set_transform(const glm::mat4& trs)
{
	// Set the postion
	m_position = trs[3];
	
	// Set the scale
	for (int i = 0; i < 3; ++i)
		m_scale[i] = glm::length(glm::vec3(trs[i]));

	// Set the rotation
	const glm::mat3 rot_mat(
		glm::normalize(glm::vec3(trs[0])),
		glm::normalize(glm::vec3(trs[1])),
		glm::normalize(glm::vec3(trs[2]))
	);
	
	m_rotation = glm::quat_cast(rot_mat);
}

glm::mat4 transform::compute_matrix() const
{
	// Translate
	glm::mat4 mtx = glm::translate(glm::mat4(1.0f), m_position);

	// Rotate
	mtx = mtx * glm::toMat4(m_rotation);

	// Scale
	mtx = glm::scale(mtx, m_scale);

	return mtx;
}

glm::mat4 transform::compute_inv_matrix() const
{
	// Scale
	glm::mat4 mtx = glm::scale(glm::mat4(1.0f), 1.0f / m_scale);

	// Rotate
	mtx = mtx * glm::toMat4(glm::inverse(m_rotation));

	// Translate
	mtx = glm::translate(mtx, -m_position);

	return mtx;
}

void transform::imgui()
{
	ImGui::Text("Position");
	ImGui::DragFloat3("T", &m_position[0]);
	ImGui::Separator();
	ImGui::Text("Rotation");
	ImGui::DragFloat4("R", &m_rotation[0]);
	ImGui::Separator();
	ImGui::Text("Scale");
	ImGui::DragFloat3("S", &m_scale[0]);
}

transform transform::concatenate(const transform& rhs) const
{
	transform res;

	res.m_scale = m_scale * rhs.m_scale;
	res.m_rotation = m_rotation * rhs.m_rotation;
	res.m_position = m_rotation * (m_scale * rhs.m_position) + m_position;

	return res;
}

transform transform::inv_concatenate(const transform& rhs) const
{
	transform res;
	glm::quat inv_quat = glm::inverse(m_rotation);

	res.m_scale = rhs.m_scale / m_scale;
	res.m_rotation = inv_quat * rhs.m_rotation;
	res.m_position = (1.0f / m_scale) * (inv_quat * (rhs.m_position - m_position));

	return res;
}
}