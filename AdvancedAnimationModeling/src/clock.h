/**
* @file clock.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once

namespace cs460 {
class clock
{
public:
	static clock& get_instance();
	float dt();
	void update();

private:
	float m_delta_time = 0.0f;
	float m_prev_time_stamp = 0.0f;
};
#define g_clock clock::get_instance()
}