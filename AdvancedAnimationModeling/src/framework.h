/**
* @file framework.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once

namespace cs460 {
class framework
{
public:
	void create();
	bool update();
	void destroy();

private:
	unsigned int m_vao, m_vbo;
};
}