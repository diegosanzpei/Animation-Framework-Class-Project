/**
* @file window.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include <glm/vec2.hpp>

struct GLFWwindow;

namespace cs460 {
class window {
private:
	GLFWwindow* m_window;
	glm::ivec2 m_size;

public:
	window();
	~window() {};
	bool create(int w, int h, const char* window_name, bool hidden);
	bool update();
	void destroy();

	glm::ivec2 size() const {return m_size;}
	GLFWwindow* handle() const {return m_window;}
};
}