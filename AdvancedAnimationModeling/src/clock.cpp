/**
* @file clock.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include "clock.h"
#include "GLFW/glfw3.h"

namespace cs460 {
clock& clock::get_instance()
{
	static clock gc;
	return gc;
}

float clock::dt()
{
	return m_delta_time;
}

void clock::update()
{
	// Get the current time stamp
	float current_time_stamp = (float)glfwGetTime();

	// Compute the delta time of the last frame
	m_delta_time = current_time_stamp - m_prev_time_stamp;

	// Update previous time stamp
	m_prev_time_stamp = current_time_stamp;
}
}
