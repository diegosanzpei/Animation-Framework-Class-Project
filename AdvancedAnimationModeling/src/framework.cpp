/**
* @file framework.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include "framework.h"
#include "input.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "editor.h"
#include <glm/glm.hpp>
#include "clock.h"
#include "scene_graph.h"
#include "renderer.h"
#include "resources.h"
#include "debug.h"

namespace cs460 {
void framework::create()
{
	// Create the glfw window
	g_renderer.create(1280, 720, "Diego Sanz - CS460: Advanced Animation Modeling", false);

	// Create the input manager
	g_input.create(g_renderer.get_window().handle());

	// Initialize Dear ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(g_renderer.get_window().handle(), true);
	ImGui_ImplOpenGL3_Init("#version 440");
	ImGui::StyleColorsDark();
}

bool framework::update()
{
	// Update the clock
	g_clock.update();

	// Update the input manager
	g_input.update();
	
	// Update nodes
	g_scene.update();

	// Render nodes
	g_scene.render();

	// Render the framework's GUI
	g_editor.render();

	// Update the glfw window
	return g_renderer.update_window();
}

void framework::destroy()
{
	// Shutdown Dear ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	g_resources.destroy();
	g_renderer.destroy();
	g_scene.destroy();
}
}