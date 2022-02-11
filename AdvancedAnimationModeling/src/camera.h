/**
* @file camera.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include <glm/glm.hpp>
#include "component.h"

namespace cs460 {
class transform;
class camera : public component
{
public:
    camera();

    // Sets the projection matrix (from camera model space to projection space)
    void set_projection(const float fov, const glm::vec2& size, const float near, const float far);

    virtual void initialize();

    // Update the matrices
    virtual void update();

    const glm::vec3& get_pos() const;

    // Returns a reference to the camera's world to projection matrix
    const glm::mat4& get_world_to_projection() const;
    const glm::mat4& get_view_matrix() const { return m_world_to_local; }
    const glm::mat4& get_projection_matrix() const { return m_projection; }

    void camera_has_control(bool has_control) { m_has_control = has_control; }
    void reset_camera();

private:
    void move_first_person();
    void orbital_camera();

    glm::vec3 m_pos;

    float m_speed = 5.0f;
    float m_radius = 5.0f;

    bool m_has_control = true;

    glm::mat4 m_projection;
    glm::vec3 m_view;
    glm::vec3 m_right;
    glm::vec3 m_up;

    glm::mat4 m_world_to_local;
    glm::mat4 m_world_to_projection;

    friend class player_controller;
    friend class scene_graph;
};
}