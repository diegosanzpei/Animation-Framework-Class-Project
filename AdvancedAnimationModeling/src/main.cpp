/**
* @file main.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include "framework.h"

int main()
{
	// Create the framework
	cs460::framework fw;
	fw.create();

	// Run the framework
	while (fw.update()) {}

	// Destroy the framework
	fw.destroy();

	return 0;
}