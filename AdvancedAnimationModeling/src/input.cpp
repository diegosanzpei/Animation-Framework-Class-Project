/**
* @file input.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#include "input.h"
#include "GLFW/glfw3.h"
#include "window.h"
#include "loader.h"

namespace cs460 {
input& input::get_instance()
{
	// Create an instance of the class only once
	static input IM;
	return IM;
}

// Update cursor position and key states
void input::update()
{
	updateCursor();
	updateKeys();
	updateGamePad();
}

void input::create(GLFWwindow* window)
{
	setCallbacks(window);
}

void input::updateCursor()
{
	if (m_userScrolled)
		m_userScrolled = false;
	else
		m_verticalScroll = 0.0f;

	// Calculate the direction of the cursor
	if (m_cursorPrevPos != m_cursorCurrPos)
	{
		m_cursorDir = m_cursorCurrPos - m_cursorPrevPos;
		m_cursorDir.y = -m_cursorDir.y;
		m_cursorPrevPos = m_cursorCurrPos;
	}
	else
	{
		m_cursorDir.x = 0.0f;
		m_cursorDir.y = 0.0f;
	}

	// Update the state of the cursor buttons
	for (unsigned int idx = 0; idx < N_CURSOR_BUTTONS; ++idx)
	{
		// Button is being held down
		if (m_cursorCurrButtons[idx] == state::pressed && m_cursorPrevButtons[idx] == state::pressed)
			m_cursorCurrButtons[idx] = state::down;

		// Button is up
		else if (m_cursorCurrButtons[idx] == state::released && m_cursorPrevButtons[idx] == state::released)
			m_cursorCurrButtons[idx] = state::up;

		// Update the previous state of the button
		m_cursorPrevButtons[idx] = m_cursorCurrButtons[idx];
	}
}

void input::dropCallback(GLFWwindow* window, int count, const char** paths)
{
	for (int i = 0; i < count; ++i)
		import_gltf_file(paths[i]);
}

void input::updateKeys()
{
	if (m_anyKeyPressedOnCallback)
	{
		m_anyKeyPressed = m_anyKeyPressedOnCallback;
		m_anyKeyPressedOnCallback = false;
	}

	// Update the state of the cursor buttons
	for (unsigned int idx = 0; idx < N_KEYS; ++idx)
	{
		// Key is being held down
		if (m_CurrKeys[idx] == state::pressed && m_PrevKeys[idx] == state::pressed)
			m_CurrKeys[idx] = state::down;

		// Keys is up
		else if (m_CurrKeys[idx] == state::released && m_PrevKeys[idx] == state::released)
			m_CurrKeys[idx] = state::up;

		// Update the previous state of the key
		m_PrevKeys[idx] = m_CurrKeys[idx];
	}
}

// Returns true if any key has been pressed
bool input::anyKeyPressed()
{
	return m_anyKeyPressed;
}

// Returns true if the specified key has been pressed
bool input::keyIsPressed(key_id idx)
{
	return m_CurrKeys[idx] == state::pressed;
}

// Returns true if the specified key is held down
bool input::keyIsDown(key_id idx)
{
	return m_CurrKeys[idx] == state::down;
}

// Returns true if the specified key is released
bool input::keyIsReleased(key_id idx)
{
	return m_CurrKeys[idx] == state::released;
}

// Returns true if the specified key is up
bool input::keyIsUp(key_id idx)
{
	return m_CurrKeys[idx] == state::up;
}

bool input::mouseIsPressed(button_id idx)
{
	return m_cursorCurrButtons[idx] == state::pressed;
}

bool input::mouseIsDown(button_id idx)
{
	return m_cursorCurrButtons[idx] == state::down;
}

bool input::mouseIsReleased(button_id idx)
{
	return m_cursorCurrButtons[idx] == state::released;
}

bool input::mouseIsUp(button_id idx)
{
	return m_cursorCurrButtons[idx] == state::up;
}

const glm::vec2& input::getCursorPos()
{
	return m_cursorCurrPos;
}

const glm::vec2& input::getCursorDir()
{
	return m_cursorDir;
}

float input::getMouseScroll()
{
	return m_verticalScroll;
}

bool input::gamePadIsPressed(button_id bIdx, gamepad_id gIdx)
{
	if (gamePadIsConnected(gIdx) == false)
	{
		return false;
	}

	return m_gamePadCurrButtons[gIdx][bIdx] == state::pressed;
}

bool input::gamePadIsReleased(button_id bIdx, gamepad_id gIdx)
{
	if (gamePadIsConnected(gIdx) == false)
	{
		return false;
	}

	return m_gamePadCurrButtons[gIdx][bIdx] == state::released;
}

bool input::gamePadIsUp(button_id bIdx, gamepad_id gIdx)
{
	if (gamePadIsConnected(gIdx) == false)
	{
		return false;
	}

	return m_gamePadCurrButtons[gIdx][bIdx] == state::up;
}

bool input::gamePadIsDown(button_id bIdx, gamepad_id gIdx)
{
	if (gamePadIsConnected(gIdx) == false)
	{
		return false;
	}

	return m_gamePadCurrButtons[gIdx][bIdx] == state::down;
}

bool input::gamePadIsConnected(gamepad_id idx)
{
	if (idx < 0 || idx > 3)
	{
		return false;
	}

	return glfwJoystickPresent(idx);
}

const glm::vec2 input::getGamePadStickVec(stick_id sIdx, gamepad_id gIdx)
{
	glm::vec2 result(0.0f);

	if (m_gamePadCurrAxes[gIdx] == nullptr)
	{
		return result;
	}

	if (sIdx == gamepad::left_stick)
	{
		if (m_gamePadCurrAxes[gIdx][0] > OFFSET || m_gamePadCurrAxes[gIdx][0] < -OFFSET)
			result.x = m_gamePadCurrAxes[gIdx][0];
		if (m_gamePadCurrAxes[gIdx][1] > OFFSET || m_gamePadCurrAxes[gIdx][1] < -OFFSET)
			result.y = -m_gamePadCurrAxes[gIdx][1];
	}
	else if (sIdx == gamepad::right_stick)
	{
		if (m_gamePadCurrAxes[gIdx][2] > OFFSET || m_gamePadCurrAxes[gIdx][2] < -OFFSET)
			result.x = m_gamePadCurrAxes[gIdx][2];
		if (m_gamePadCurrAxes[gIdx][3] > OFFSET || m_gamePadCurrAxes[gIdx][3] < -OFFSET)
			result.y = -m_gamePadCurrAxes[gIdx][3];
	}
	return result;
}

const float input::getGamePadTrigger(stick_id tIdx, gamepad_id gIdx)
{
	float result = 0.0f;

	if (m_gamePadCurrAxes[gIdx] == nullptr)
	{
		return result;
	}

	if (tIdx == gamepad::left_trigger)
		result = (m_gamePadCurrAxes[gIdx][4] + 1.0f) / 2.0f;
	else if (tIdx == gamepad::right_trigger)
		result = (m_gamePadCurrAxes[gIdx][5] + 1.0f) / 2.0f;

	if (result > OFFSET)
		return result;
	else
		return 0.0f;
}

void input::setCallbacks(GLFWwindow* window)
{
	// Set callbacks
	glfwSetKeyCallback(window, keyCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetMouseButtonCallback(window, cursorButtonCallback);
	glfwSetDropCallback(window, dropCallback);
}

input::~input()
{
}

// Called by glfw every time a key is pressed, released or held down
void input::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		get_instance().setKeyState(key, state::pressed);
	}
	else if (action == GLFW_RELEASE)
		get_instance().setKeyState(key, state::released);
}

// Receives cursor position in screen coordinates 
// (top-left corner of window)
void input::cursorPosCallback(GLFWwindow* window, double xPos, double yPos)
{
	get_instance().setCursorPos((float)xPos, (float)yPos);
}

// Called by glfw for every cursor button pressed/released
void input::cursorButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
		get_instance().setCursorButtonState(button, state::pressed);

	else if (action == GLFW_RELEASE)
		get_instance().setCursorButtonState(button, state::released);
}

void input::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	get_instance().setVerticalSroll((float)yoffset);
}

// Converts the glfwKey to the index of the key in the manager's container
bool input::getKeyIndex(unsigned int glfwKey, unsigned int* idx)
{
	// Numbers
	if (glfwKey >= 48 && glfwKey <= 57)
		*idx = glfwKey - 48;

	// Letters
	else if (glfwKey >= 65 && glfwKey <= 90)
		*idx = glfwKey - 55;

	// Space bar
	else if (glfwKey == 32)
		*idx = keyboard::key_space;

	// Enter, arrow keys, escape
	else if (glfwKey >= 256 && glfwKey <= 265)
		*idx = glfwKey - 219;

	// Left shift, control, alt
	else if (glfwKey >= 340 && glfwKey <= 342)
		*idx = glfwKey - 293;

	// Right shift, control, alt
	else if (glfwKey >= 344 && glfwKey <= 346)
		*idx = glfwKey - 294;

	// Function keys
	else if (glfwKey >= 290 && glfwKey <= 301)
		*idx = glfwKey - 237;

	// The button is not in the container
	else
		return false;

	// The button is in the container
	return true;
}

// Updates the current state of the given key
void input::setKeyState(int glfwKey, input::state s)
{
	unsigned int index;

	if (getKeyIndex(glfwKey, &index))
		m_CurrKeys[index] = s;

	if (s == state::pressed)
		m_anyKeyPressedOnCallback = true;
}

// Sets the position of the cursor on the current frame
void input::setCursorPos(float xPos, float yPos)
{
	// Set the position of the mouse on the current pos
	m_cursorCurrPos.x = xPos;
	m_cursorCurrPos.y = yPos;
}

void input::setVerticalSroll(float yOffset)
{
	m_verticalScroll = yOffset;
	m_userScrolled = true;
}

void input::setCursorButtonState(int glfwKey, state s)
{
	// In this case, the glfwKey coincides with the index of the container
	m_cursorCurrButtons[glfwKey] = s;
}

void input::updateGamePad()
{
	for (int gamepad = 0; gamepad < 4; ++gamepad)
	{
		// Check if the controller is connected
		if (glfwJoystickPresent(gamepad))
		{
			m_gamePadCurrAxes[gamepad] = glfwGetJoystickAxes(gamepad, &m_nAxes);
			m_gamePadButtons = glfwGetJoystickButtons(gamepad, &m_nButtons);
		}

		else // Controller not connected, continue with the next one
		{
			m_gamePadCurrAxes[gamepad] = nullptr;
			m_gamePadButtons = nullptr;
			continue;
		}

		// Update the state of the gamepad buttons
		for (unsigned int idx = 0; idx < N_GAMEPAD_BUTTONS; ++idx)
		{
			// Button is pressed in this iteration
			if (m_gamePadButtons[idx] == GLFW_PRESS)
			{
				// Button is pressed
				if (m_gamePadCurrButtons[gamepad][idx] == state::up || m_gamePadCurrButtons[gamepad][idx] == state::released)
					m_gamePadCurrButtons[gamepad][idx] = state::pressed;

				// Button is down
				else if (m_gamePadCurrButtons[gamepad][idx] == state::pressed)
					m_gamePadCurrButtons[gamepad][idx] = state::down;
			}

			// Buton is released this iteration
			else
			{
				// Button is released
				if (m_gamePadCurrButtons[gamepad][idx] == state::down || m_gamePadCurrButtons[gamepad][idx] == state::pressed)
					m_gamePadCurrButtons[gamepad][idx] = state::released;

				// Button is up
				else if (m_gamePadCurrButtons[gamepad][idx] == state::released)
					m_gamePadCurrButtons[gamepad][idx] = state::up;
			}
		}
	}
}

// Set callbacks and initialize cursor position
input::input()
	: m_cursorCurrPos(0.0f, 0.0f)
	, m_cursorPrevPos(0.0f, 0.0f)
	, m_anyKeyPressed(false)
	, m_anyKeyPressedOnCallback(false)
	, m_userScrolled(false)
	, m_verticalScroll(0.0f)

{}
}