/**
* @file input.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
/**
 * @file      input.h
 * @author    Diego Sanz // diego.sanz // 540001618
 * @date      09-18-2020
 * @copyright Copyright (c) 2020
 */

#include <utility> // std::pair
#include <glm/vec2.hpp>
struct GLFWwindow;

namespace cs460 {
const int N_KEYS = 65;
const int N_CURSOR_BUTTONS = 3;
const int N_GAMEPAD_AXES = 6;
const int N_GAMEPAD_BUTTONS = 14;
const float OFFSET = 0.2f;

 //------------------KEYBOARD------------------
using key_id = const int;
namespace keyboard
{
    // IM Container : 0 - 9
    // GLFW : 48 - 57
    key_id key_0 = 0;
    key_id key_1 = 1;
    key_id key_2 = 2;
    key_id key_3 = 3;
    key_id key_4 = 4;
    key_id key_5 = 5;
    key_id key_6 = 6;
    key_id key_7 = 7;
    key_id key_8 = 8;
    key_id key_9 = 9;

    // IM Container : 10 - 35
    // GLFW : 65 - 90
    key_id key_a = 10;
    key_id key_b = 11;
    key_id key_c = 12;
    key_id key_d = 13;
    key_id key_e = 14;
    key_id key_f = 15;
    key_id key_g = 16;
    key_id key_h = 17;
    key_id key_i = 18;
    key_id key_j = 19;
    key_id key_k = 20;
    key_id key_l = 21;
    key_id key_m = 22;
    key_id key_n = 23;
    key_id key_o = 24;
    key_id key_p = 25;
    key_id key_q = 26;
    key_id key_r = 27;
    key_id key_s = 28;
    key_id key_t = 29;
    key_id key_u = 30;
    key_id key_v = 31;
    key_id key_w = 32;
    key_id key_x = 33;
    key_id key_y = 34;
    key_id key_z = 35;

    // IM Container : 36
    // GLFW : 32
    key_id key_space = 36;
    
    // IM Container : 37 - 46
    // GLFW : 256 - 265
    key_id key_escape    = 37;
    key_id key_enter     = 38;
    key_id key_tab       = 39;
    key_id key_backspace = 40;
    key_id key_insert    = 41;
    key_id key_delete    = 42;
    key_id key_right     = 43;
    key_id key_left      = 44;
    key_id key_down      = 45;
    key_id key_up        = 46;
    
    // IM Container : 47 - 49
    // GLFW : 340 - 342
    key_id key_left_shift   = 47;
    key_id key_left_control = 48;
    key_id key_left_alt     = 49;
    
    // IM Container : 50 - 52
    // GLFW : 344 - 346
    key_id key_right_shift   = 50;
    key_id key_right_control = 51;
    key_id key_right_alt     = 52;

    // IM Container : 53 - 64
    // GLFW : 290 - 301
    key_id key_f1  = 53;
    key_id key_f2  = 54;
    key_id key_f3  = 55;
    key_id key_f4  = 56;
    key_id key_f5  = 57;
    key_id key_f6  = 58;
    key_id key_f7  = 59;
    key_id key_f8  = 60;
    key_id key_f9  = 61;
    key_id key_f10 = 62;
    key_id key_f11 = 63;
    key_id key_f12 = 64;
}
//------------------KEYBOARD------------------

//-------------------MOUSE--------------------
using button_id = const int;
namespace mouse
{
    // IM Cointaner : 0 - 2
    // GLFW: : 0 - 2
    button_id button_left   = 0;
    button_id button_right  = 1;
    button_id button_middle = 2;
}
//-------------------MOUSE--------------------

//-----------------GAMEPAD--------------------
using stick_id = const int;
using trigger_id = const int;
using gamepad_id = const int;
namespace gamepad
{
    gamepad_id controller_1 = 0;
    gamepad_id controller_2 = 1;
    gamepad_id controller_3 = 2;
    gamepad_id controller_4 = 3;

    button_id button_a = 0;
    button_id button_b = 1;
    button_id button_x = 2;
    button_id button_y = 3;
    button_id button_lb = 4;
    button_id button_rb = 5;
    button_id button_share = 6;
    button_id button_options = 7;
    button_id button_left_bumper = 8;
    button_id button_right_bumper = 9;
    button_id button_up = 10;
    button_id button_right = 11;
    button_id button_down = 12;
    button_id button_left = 13;

    trigger_id left_trigger = 0;
    trigger_id right_trigger = 1;

    stick_id left_stick = 0;
    stick_id right_stick = 1;
}
//-----------------GAMEPAD--------------------

class input
{
public:
    // Users can't create a copy of the class
    input(const input&) = delete;
    input& operator=(const input&) = delete;

    // Singleton implementation
    static input& get_instance();

    // Process user input
    void update();

    // Set callbacks
    void create(GLFWwindow* window);

    //------------------KEYBOARD------------------
    bool anyKeyPressed();
    bool keyIsPressed(key_id idx);
    bool keyIsDown(key_id idx);
    bool keyIsReleased(key_id idx);
    bool keyIsUp(key_id idx);
    //------------------KEYBOARD------------------

    //-------------------MOUSE--------------------
    bool mouseIsPressed(button_id idx);
    bool mouseIsDown(button_id idx);
    bool mouseIsReleased(button_id idx);
    bool mouseIsUp(button_id idx);
    const glm::vec2& getCursorPos();
    const glm::vec2& getCursorDir();

    // Returns the delta scroll
    float getMouseScroll();
    //-------------------MOUSE--------------------

    //-----------------GAMEPAD--------------------
    bool gamePadIsPressed(button_id bIdx, gamepad_id gIdx = 0);
    bool gamePadIsReleased(button_id bIdx, gamepad_id gIdx = 0);
    bool gamePadIsUp(button_id bIdx, gamepad_id gIdx = 0);
    bool gamePadIsDown(button_id bIdx, gamepad_id gIdx = 0);

    // Returns true if the controlled is connected
    bool gamePadIsConnected(gamepad_id idx);

    // Returns the direction of the specified axis (right/left stick)
    // range[-1,1]
    const glm::vec2 getGamePadStickVec(stick_id sIdx, gamepad_id gIdx = 0);

    // Returns a value between 0 (not pressed) to 1 (fully pressed)
    const float getGamePadTrigger(stick_id tIdx, gamepad_id gIdx = 0);
    //-----------------GAMEPAD--------------------

    void setCallbacks(GLFWwindow* window);

private:
    // Users can't create an instance of the class
    input();
    ~input();

    // States of the keys/buttons
    enum class state { up, pressed, down, released };

    //------------------KEYBOARD------------------
// Function called for every key pressed/released
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    // Get the index of the key in the container
    bool getKeyIndex(unsigned int glfwKey, unsigned int* idx);

    // Set the state of a key
    void setKeyState(int glfwKey, state s);

    void updateKeys();

    // Set to true if the callback gets a press action
    // Resets to false on update()
    bool m_anyKeyPressedOnCallback = false;

    // Set to the value of the above boolean
    // It is set every frame on update()
    bool m_anyKeyPressed = false;

    // Key states
    state m_CurrKeys[N_KEYS] = { state::up };
    state m_PrevKeys[N_KEYS] = { state::up };
    //------------------KEYBOARD------------------

    //-------------------MOUSE--------------------
    // Function called every time the cursor moves over the window
    static void cursorPosCallback(GLFWwindow* window, double xPos, double yPos);

    // Function called for every mouse button pressed/released
    static void cursorButtonCallback(GLFWwindow* window, int button, int action, int mods);

    // Function called every time the user scrolls
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    // Set the position of the cursor
    void setCursorPos(float xPos, float yPos);

    void setVerticalSroll(float yOffset);

    // Set the state of a cursor button
    void setCursorButtonState(int glfwKey, state s);

    void updateCursor();

    float m_verticalScroll;
    bool m_userScrolled;

    glm::vec2 m_cursorCurrPos;
    glm::vec2 m_cursorPrevPos;
    glm::vec2 m_cursorDir;

    // States of the cursor buttons
    state m_cursorCurrButtons[N_CURSOR_BUTTONS] = { state::up };
    state m_cursorPrevButtons[N_CURSOR_BUTTONS] = { state::up };
    //-------------------MOUSE--------------------

    //-----------------GAMEPAD--------------------
    void updateGamePad();

    int m_nAxes;

    int m_nButtons;
    const unsigned char* m_gamePadButtons;

    // States of the gamepad buttons
    state m_gamePadCurrButtons[4][N_GAMEPAD_BUTTONS] = { state::up };
    const float* m_gamePadCurrAxes[4] = { nullptr };
    //-----------------GAMEPAD--------------------

    //-----------------DRAGNDROP------------------
    static void dropCallback(GLFWwindow* window, int count, const char** paths);
    //-----------------DRAGNDROP------------------
};

#define g_input input::get_instance()
}