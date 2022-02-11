/**
* @file window.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include <iostream>
#include "GLFW/glfw3.h"
#include "window.h"
#include "input.h"

namespace cs460 {
window::window()
    : m_window(nullptr)
    , m_size(0,0)
{}

bool window::create(int w, int h, const char* window_name, bool hidden)
{
    //Specify if the window should be visible
    glfwWindowHint(GLFW_VISIBLE, !hidden);

    // Window can't be resizable
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Create the window and set the window pointer
    m_window = glfwCreateWindow(w, h, window_name, NULL, NULL);

    if (!m_window)
    {
        std::cout << "Window or OpenGL context creation failed" << std::endl;
        glfwTerminate();
        exit(1);
    }

    m_size.x = w;
    m_size.y = h;

    return true;
}

bool window::update()
{
    if (g_input.keyIsDown(keyboard::key_left_control) && g_input.keyIsPressed(keyboard::key_q))
        return false;

    // Keep Running
    if(!glfwWindowShouldClose(m_window))
    {
        // Show the rendered frame
        glfwSwapBuffers(m_window);

        // Clear the back buffer for the next frame
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Process events
        glfwPollEvents();

        return true;
    }

    // Close the window
    return false;
}

void window::destroy()
{
    glfwDestroyWindow(m_window);
}
}