/**
* @file component.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once

namespace cs460 {
struct node;
class component
{
public:
	virtual void initialize() {}
	virtual void update() {}
	virtual void imgui() {}
	virtual void destroy() {}

	node* get_owner() { return m_owner; }
	void set_owner(node* parent) { m_owner = parent; }

private:
	node* m_owner = nullptr;
};
}