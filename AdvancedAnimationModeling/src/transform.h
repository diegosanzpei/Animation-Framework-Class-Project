/**
* @file transform.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace cs460 {
class transform
{
public:
	transform();
	void set_position(const glm::vec3& pos) { m_position = pos; }
	void set_rotation(const glm::quat& rot) { m_rotation = rot; }
	void set_scale(const glm::vec3& scale) { m_scale = scale; }
	void set_transform(const glm::mat4& trs);

	glm::vec3& get_position() { return m_position; }
	glm::quat& get_rotation() { return m_rotation; }
	glm::vec3& get_scale() { return m_scale; }

	const glm::vec3& get_position() const { return m_position; }
	const glm::quat& get_rotation() const { return m_rotation; }
	const glm::vec3& get_scale() const { return m_scale; }

	// Computes TRS matrix
	glm::mat4 compute_matrix() const;
	glm::mat4 compute_inv_matrix() const;

	// Display component info using imgui
	void imgui();

	// Concatenates the given transform: result = this * rhs
	transform concatenate(const transform& rhs) const;

	// Inverse concatenates the given transform: result = inverse(this) * rhs
	transform inv_concatenate(const transform& rhs) const;

private:
	glm::vec3 m_position;
	glm::quat m_rotation;
	glm::vec3 m_scale;
};
}