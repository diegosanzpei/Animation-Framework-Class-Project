/**
* @file camera.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include "input.h"
#include "clock.h"
#include "scene_graph.h"
#include "renderer.h"
#include <iostream>
#include "editor.h"
#include "node.h"

namespace cs460 {
camera::camera()
    : m_view(glm::vec3(0.0f, 0.0f, -1.0f))
    , m_pos(glm::vec3(0.0f, 0.0f, 5.0f))
    , m_up(glm::vec3(0.0f, 1.0f, 0.0f))
    , m_right(glm::vec3(1.0f, 0.0f, 0.0f))
    , m_world_to_local(0.0f)
    , m_world_to_projection(0.0f)
    , m_projection(0.0f)
{
    reset_camera();
    set_projection(45.0f, g_renderer.get_window().size(), 0.01f, 1000.0f);
}

void camera::set_projection(const float fov, const glm::vec2& size, const float near, const float far)
{
    m_projection = glm::perspective(glm::radians(fov), size.x/size.y, near, far);
}

void camera::initialize()
{
}

void camera::update()
{
    if (g_input.keyIsPressed(keyboard::key_v))
        reset_camera();

    // Move camera
    if (m_has_control)
    {
        if (g_input.keyIsDown(keyboard::key_left_alt))
            orbital_camera();
        else
            move_first_person();
    }

    // Get the right and up vector of the camera
    m_right = glm::normalize(glm::cross(m_view, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_up = glm::normalize(glm::cross(m_right, m_view));

    // Matrix to transform the camera from world to local space
    m_world_to_local = glm::lookAt(m_pos, m_pos + m_view, m_up);

    // Matrix to transform the camera from world to projection space
    m_world_to_projection = m_projection * m_world_to_local;
}

const glm::vec3& camera::get_pos() const
{
    return m_pos;
}

const glm::mat4& camera::get_world_to_projection() const
{
    return m_world_to_projection;
}

void camera::move_first_person()
{
    // Get the dt
    float dt = g_clock.dt();

    // Rotate view vector
    float angle_yaw = 0.0f;
    float angle_pitch = 0.0f;
    bool rotate = false;
    if (g_input.mouseIsDown(mouse::button_right))
    {
        // Update camera speed
        float mouse_wheel = g_input.getMouseScroll();
        m_speed += 80.0f * mouse_wheel * dt;
        m_speed = glm::clamp(m_speed, 1.0f, 50.0f);

        // Rotations using mouse
        rotate = true;
        const glm::vec2& mouse_dir = g_input.getCursorDir();
        float cam_speed = 0.2f * dt;
        angle_yaw = -mouse_dir.x * cam_speed;
        angle_pitch = mouse_dir.y * cam_speed;
    }

    // Rotations using arrow keys
    else
    {
        float cam_speed = 1.0f * dt;
        if (g_input.keyIsDown(keyboard::key_up))
        {
            rotate = true;
            angle_pitch = cam_speed;
        }
        else if (g_input.keyIsDown(keyboard::key_down))
        {
            rotate = true;
            angle_pitch = -cam_speed;
        }
        if (g_input.keyIsDown(keyboard::key_right))
        {
            rotate = true;
            angle_yaw = -cam_speed;
        }
        else if (g_input.keyIsDown(keyboard::key_left))
        {
            rotate = true;
            angle_yaw = cam_speed;
        }
    }

    // Rotate view vector around up and right vector
    if (rotate)
    {
        glm::vec3 temp = glm::rotate(m_view, angle_yaw, m_up);
        temp = glm::rotate(temp, angle_pitch, m_right);

        // Avoid weird rotations
        if (!glm::epsilonEqual(temp.x, 0.0f, 0.2f) ||
            !glm::epsilonEqual(temp.z, 0.0f, 0.2f))
            m_view = temp;
    }

    // Move camera (WASD)
    glm::vec3& pos = m_pos;
    float speed = m_speed * dt;

    // Move camera up
    if (g_input.keyIsDown(keyboard::key_e))
        pos.y += speed;

    // Move camera down
    else if (g_input.keyIsDown(keyboard::key_q))
        pos.y -= speed;

    // Move camera forward
    if (g_input.keyIsDown(keyboard::key_w))
        pos += m_view * speed;

    // Move camera backwards
    else if (g_input.keyIsDown(keyboard::key_s))
        pos -= m_view * speed;

    // Move camera to the right
    if (g_input.keyIsDown(keyboard::key_d))
        pos += m_right * speed;

    // Move camera to the left
    else if (g_input.keyIsDown(keyboard::key_a))
        pos -= m_right * speed;
}

void camera::orbital_camera()
{
    // Get the mouse direction
    const glm::vec2& mouse_dir = g_input.getCursorDir();

    // Rotate camera
    float speed = 0.3f * g_clock.dt();

    float scroll = g_input.getMouseScroll();
    m_pos += 50.0f * speed * m_view * scroll;

    if (mouse_dir.x == 0.0f && mouse_dir.y == 0.0f)
        return;

    if (g_input.mouseIsDown(mouse::button_right))
    {
        // Compute the center of rotation
        glm::vec3 origin;
        
        // Get the center of rotation
        if (node* focus = g_editor.get_selected_node())
            origin = focus->m_world.get_position();
        else
            origin = m_pos + m_view * m_radius;

        glm::vec3 cam_dir = m_pos - origin;

        // Rotate camera
        cam_dir = glm::rotate(cam_dir, -mouse_dir.x * speed, glm::vec3(0.0f, 1.0f, 0.0f));
        
        // Recompute position and view vector
        m_pos = origin + cam_dir;
        m_view = glm::normalize(origin - m_pos);
        m_right = glm::cross(m_view, glm::vec3(0.0f, 1.0f, 0.0f));

        // Rotate camera
        glm::vec3 temp = glm::rotate(cam_dir, mouse_dir.y * speed, m_right);
        
        // Avoid weird rotations
        if (!glm::epsilonEqual(temp.x, 0.0f, 1.0f) || !glm::epsilonEqual(temp.z, 0.0f, 1.0f))
        {
            // Recompute position and view
            cam_dir = temp;
            m_pos = origin + cam_dir;
            m_view = glm::normalize(origin - m_pos);
        }
    }

    else if (g_input.mouseIsDown(mouse::button_left))
    {
        m_pos += -speed * mouse_dir.x * m_right;
        m_pos += -speed * mouse_dir.y * m_up;
    }
}

void camera::reset_camera()
{
    m_view = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
    m_pos = glm::vec3(0.0f, 0.0f, 7.0f);
}
}