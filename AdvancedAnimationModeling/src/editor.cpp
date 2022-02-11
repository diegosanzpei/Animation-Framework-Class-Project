/**
* @file editor.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include "editor.h"
#include <imgui.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "scene_graph.h"
#include <iostream>
#include <ImGuizmo.h>
#include "input.h"
#include "loader.h"
#include "camera.h"
#include "node.h"
#include "mesh_comp.h"
#include "renderer.h"
#include "resources.h"
#include <glm/ext/matrix_projection.hpp>
#include "debug.h"
#include "curve_node_comp.h"
#include "anim_comp.h"

namespace cs460 {
editor::editor()
    : m_guizmode(ImGuizmo::OPERATION::TRANSLATE)
    , m_perspective(ImGuizmo::WORLD)
{}
void editor::imgui_new_frame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void editor::render_inspector()
{
    bool open = true;
    ImGui::Begin("Inspector", &open, ImGuiWindowFlags_NoMove);

    ImGui::RadioButton("world", &m_perspective, ImGuizmo::WORLD); ImGui::SameLine();
    ImGui::RadioButton("local", &m_perspective, ImGuizmo::LOCAL);
    
    m_selected_node->imgui();

    ImGui::End();
}

bool editor::render_guizmo()
{
    if (!m_use_guizmo || !m_selected_node)
        return false;

    // Set guizmo operation (translate, rotate, scale)
    if (g_input.keyIsUp(keyboard::key_left_control))
    {
        if (g_input.keyIsPressed(keyboard::key_1))
            m_guizmode = ImGuizmo::OPERATION::TRANSLATE;
        else if (g_input.keyIsPressed(keyboard::key_2))
            m_guizmode = ImGuizmo::OPERATION::SCALE;
        else if (g_input.keyIsPressed(keyboard::key_3))
            m_guizmode = ImGuizmo::OPERATION::ROTATE;
    }

    // Get model to world matrix of node
    glm::mat4 m2w = m_selected_node->m_world.compute_matrix();

    // Get the world to view camera matrix
    const glm::mat4 view = g_scene.get_camera().get_view_matrix();

    // Get projection matrix
    const glm::mat4& proj = g_scene.get_camera().get_projection_matrix();

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate(&view[0][0], &proj[0][0], ImGuizmo::OPERATION(m_guizmode), 
        ImGuizmo::MODE(m_perspective), &m2w[0][0]);

    if (ImGuizmo::IsUsing())
    {
        // Check if the node has a parent
        if (m_selected_node->m_parent)
        {
            // Get node and parent transforms
            transform& node_local = m_selected_node->m_local;
            transform& node_world = m_selected_node->m_world;
            transform& parent_world = m_selected_node->m_parent->m_world;

            // Decompose matrix and set it as the new node world
            node_world.set_transform(m2w);

            // Compute the new node local transform
            node_local = parent_world.inv_concatenate(node_world);
        }

        // Otherwise, just set the m2w as the local transform of the node
        else
            m_selected_node->m_local.set_transform(m2w);

        return true;
    }

    return false;
}

void editor::load_gltf_files()
{
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::BeginMenu("Load"))
        {
            if (ImGui::MenuItem("Skull.gltf"))
            {
                import_gltf_file("data/assets/skull/skull.gltf");
            }

            if (ImGui::MenuItem("BoomBox.gltf"))
            {
                import_gltf_file("data/assets/BoomBox/BoomBox.gltf");
            }

            if (ImGui::MenuItem("BrainStem.gltf"))
            {
                import_gltf_file("data/assets/BrainStem/BrainStem.gltf");
            }

            if (ImGui::MenuItem("Fox.gltf"))
            {
                import_gltf_file("data/assets/Fox/Fox.gltf");
            }

            if (ImGui::MenuItem("Sponza.gltf"))
            {
                import_gltf_file("data/assets/sponza/Sponza.gltf");
            }

            if (ImGui::MenuItem("Buggy.gltf"))
            {
                import_gltf_file("data/assets/buggy/Buggy.gltf");
            }

            if (ImGui::MenuItem("CessiumMan.gltf"))
            {
                import_gltf_file("data/assets/rigged figure/CesiumMan.gltf");
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void editor::render_node_bv()
{
    if (mesh_comp* m = m_selected_node->get_component<mesh_comp>())
        m->render_vb();
}

void editor::render_node_skin()
{
    if (mesh_comp* m = m_selected_node->get_component<mesh_comp>())
        m->render_skin();
}

void editor::render_bvs()
{
    // Get the bv render mode
    renderer::render_mode mode = g_renderer.bv_rendering_mode();

    // Render bounding volumes
    if (mode == renderer::render_mode::render_all)
        g_scene.render_bvs();

    else if (m_selected_node)
    {
        if (mode == renderer::render_mode::render_selected)
            render_node_bv();
    }
}

void editor::render_skins()
{
    // Get the skin render mode
    renderer::render_mode mode = g_renderer.skin_rendering_mode();

    // Render skins
    if (mode == renderer::render_mode::render_all)
        g_scene.render_skins();

    else if (m_selected_node)
    {
        if (mode == renderer::render_mode::render_selected)
            render_node_skin();
    }
}

void editor::curve_creator()
{
    bool open = true;
    if (ImGui::Begin("Picewise Curve Creator", &open, ImGuiWindowFlags_NoMove))
    {
        //if (ImGui::Button("Linear Curve"))
        //    g_scene.create_curve(scene_graph::curve_type::linear);
        if (ImGui::Button("Hermite Curve"))
            g_scene.create_curve(scene_graph::curve_type::hermite);
        if (ImGui::Button("Catmull-Rom Curve"))
            g_scene.create_curve(scene_graph::curve_type::catmull_rom);
        if (ImGui::Button("Bezier Curve"))
            g_scene.create_curve(scene_graph::curve_type::bezier);
       
        ImGui::Separator();
       
        ImGui::PushID(0);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(1.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(1.0f, 0.8f, 0.8f));
        if (ImGui::Button("Delete Curves"))
            g_scene.clear_scene();
        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }

    ImGui::End();
}

void editor::picking()
{
    glm::vec3 ray = get_mouse_ray();
    const glm::vec3& origin = g_scene.get_camera().get_pos();

    float t_min = -1;
    raycast_rec(g_scene.get_root(), origin, ray, &t_min);
    
    if (t_min < 0.0f)
        m_selected_node = nullptr;
}

float editor::ray_vs_aabb(const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& min, const glm::vec3& max)
{
    float t1 = (min.x - origin.x) / dir.x;
    float t2 = (max.x - origin.x) / dir.x;
    float t3 = (min.y - origin.y) / dir.y;
    float t4 = (max.y - origin.y) / dir.y;
    float t5 = (min.z - origin.z) / dir.z;
    float t6 = (max.z - origin.z) / dir.z;

    float tmin = glm::max(glm::max(glm::min(t1, t2), glm::min(t3, t4)), glm::min(t5, t6));
    float tmax = glm::min(glm::min(glm::max(t1, t2), glm::max(t3, t4)), glm::max(t5, t6));

    // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
    if (tmax < 0) {
        return -1;
    }

    // if tmin > tmax, ray doesn't intersect AABB
    if (tmin > tmax) {
        return -1;
    }

    if (tmin < 0) {
        return tmax;
    }
    return tmin;
}

void editor::render_grid()
{
    float size = 20;
    int div = 40;

    glm::vec3 start(-10, 0, -10);
    glm::vec3 end = start + size * glm::vec3(1, 0, 1);
    glm::vec3 color(1.0f, 1.0f, 1.0f);
    float alpha = 0.8f;
    float step = size / div;
    int mid = (div + 1) / 2;

    for (int i = 0; i <= div; ++i)
    {
        float z = start.z + i * step;
        float x = start.x + i * step;

        glm::vec3 p0(start.x, 0, z);
        glm::vec3 p1(end.x, 0, z);
        glm::vec3 p2(x, 0, start.z);
        glm::vec3 p3(x, 0, end.z);

        if (i == mid)
        {
            g_debug.debug_draw_line(p0, p1, glm::vec4(1.0f, 0.0f, 0.0f, 0.8f), false);
            g_debug.debug_draw_line(p2, p3, glm::vec4(0.0f, 0.0f, 1.0f, 0.8f), false);
        }
        else
        {
            alpha = (i % 2) == 0 ? 0.4f : 0.2f;
            g_debug.debug_draw_line(p0, p1, glm::vec4(color, alpha), false);
            g_debug.debug_draw_line(p2, p3, glm::vec4(color, alpha), false);
        }
    }
}

glm::vec3 editor::get_mouse_ray()
{
    // Get window
    const window& win = g_renderer.get_window();

    // Mouse position in top left screen coordinates
    glm::vec2 pos = g_input.getCursorPos();

    // Convert to bottom left screen coordinates
    pos.y = win.size().y - pos.y;

    // Unproject the mouse position
    const camera& cam = g_scene.get_camera();
    const glm::mat4& model = cam.get_view_matrix();
    const glm::mat4& proj = cam.get_projection_matrix();
    glm::vec4 viewport(0.0f, 0.0f, win.size().x, win.size().y);
    glm::vec3 nearPos = glm::unProject(glm::vec3(pos.x, pos.y, 0.0f), model, proj, viewport);
    glm::vec3 farPos = glm::unProject(glm::vec3(pos.x, pos.y, 1.0f), model, proj, viewport);

    // Get the ray in world position
    return glm::normalize(farPos - nearPos);
}

void editor::player_controls()
{
    scene_graph::scene_type st = g_scene.get_scene();

    bool open = true;
    ImGui::Begin("Player Controls", &open, ImGuiWindowFlags_NoMove);
    ImGui::Text("Use an Xbox Controller");
    ImGui::Text("Move Player: Left Joy Stick");

    if (st == scene_graph::scene_type::directional_movement)
    {
        ImGui::Text("Move Camera: Right Joy Stick");
        ImGui::Text("Sprint: Left Joy Stick Completely Tilted + Right Trigger");
        ImGui::Text("Rumba: Idle + Left Trigger");
    }
    ImGui::End();
    
    if (st == scene_graph::scene_type::directional_movement)
        ImGui::SetNextWindowSize(ImVec2(350, 90));
    else
        ImGui::SetNextWindowSize(ImVec2(240, 220));

    open = true;
    ImGui::Begin("Blend Graph:", &open, ImGuiWindowFlags_NoMove);
    if (m_selected_node)
        m_selected_node->get_component<anim_comp>()->get_blend_tree().display_blend_graph();
    ImGui::End();
}

void editor::imgui_render_frame()
{
    // End the frame
    ImGui::EndFrame();

    // Prepare data to be rendered
    ImGui::Render();

    // Render ImGui
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void editor::render_scene_graph_rec(node* node, bool opened)
{
    // Get the number of childs
    size_t n_childs = node->m_children.size();

    // Set the tree node flags
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (opened)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    if (m_selected_node == node) { flags |= ImGuiTreeNodeFlags_Selected; }
    if (n_childs == 0)           { flags |= ImGuiTreeNodeFlags_Leaf; }
    
    // Render the current tree node
    bool open = ImGui::TreeNodeEx(node->m_name.c_str(), flags);

    // Check if the current node has been selected
    if (ImGui::IsItemClicked())
    {
        m_scene_graph_node_selected = true;
        m_selected_node = node;
    }

    // If current tree node is open, render child nodes
    if (open)
    {
        // Render each child
        for (unsigned int i = 0; i < n_childs; ++i)
            render_scene_graph_rec(node->m_children[i]);
    
        ImGui::TreePop();
    }
}

void editor::raycast_rec(node* node, const glm::vec3& origin, const glm::vec3& dir, float* t_min)
{
    // Get the number of childs
    size_t n_childs = node->m_children.size();

    if (mesh_comp* m = node->get_component<mesh_comp>())
    {
        glm::vec3 min, max;
        m->get_vb_min_max(min, max);
        float t = ray_vs_aabb(origin, dir, min, max);
        if (t >= 0 && (t < *t_min || *t_min < 0.0f))
        {
            m_selected_node = node;
            *t_min = t;
        }
    }

    else if (node->get_component<curve_node_comp>())
    {
        glm::vec3 offset(0.05f);
        const glm::vec3& pos = node->m_world.get_position();
        float t = ray_vs_aabb(origin, dir, pos - offset, pos + offset);
        if (t >= 0 && (t < *t_min || *t_min < 0.0f))
        {
            m_selected_node = node;
            *t_min = t;
        }
    }

    // Raycast each child
    for (unsigned int i = 0; i < n_childs; ++i)
        raycast_rec(node->m_children[i], origin, dir, t_min);
}

void editor::render_scene_graph()
{
    bool open = true;
    ImGui::Begin("Scene Graph", &open, ImGuiWindowFlags_NoMove);
    
    // Get the root node of the scene graph
    node* root_node = g_scene.get_root();

    // Iterate recursively through the nodes
    m_scene_graph_node_selected = false;
    render_scene_graph_rec(root_node, true);

    ImGui::End();
}

void editor::render()
{
    // Debug drawing
    render_bvs();
    render_skins();

    // Start the new ImGui frame
    imgui_new_frame();

    // Get scene type
    scene_graph::scene_type st = g_scene.get_scene();
    if ((st == scene_graph::scene_type::directional_movement) ||
        (st == scene_graph::scene_type::targeted_movement))
    {
        player_controls();

        // End the ImGui frame and render it
        imgui_render_frame();
        return;
    }

    // Scene graph
    render_scene_graph();

    // Show selected node details
    if (m_selected_node)
        render_inspector();

    // Main menu bar
    load_gltf_files();

    // World settings
    g_scene.imgui();

    // Render settings
    g_renderer.imgui();
    
    if (st == scene_graph::scene_type::curves)
        curve_creator();
    else 
        g_resources.imgui();
    
    // Guizmo
    if (!render_guizmo() && !m_scene_graph_node_selected)
    {
        if (m_can_pick && !ImGui::GetIO().WantCaptureMouse && g_input.mouseIsPressed(mouse::button_left))
            picking();
    }

    // End the ImGui frame and render it
    imgui_render_frame();
}

editor& editor::get_instance()
{
    static editor e;
    return e;
}
}