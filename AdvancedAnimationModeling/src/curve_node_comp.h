/**
* @file anim_comp.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include "component.h"

namespace cs460 {
struct curve_node_comp : public component
{
	virtual void initialize() {}
	virtual void update() {}
	virtual void imgui() {}
private:
};
}