#pragma once
#include <GLFW/glfw3.h>
#include <cmath>

class Gamepad {
private:
    int m_JoystickId;
    float m_Deadzone;
    GLFWgamepadstate m_State;
    bool m_IsConnected;
    DebuggerClass* imgui_helper;

    // Funcție utilitară pentru a aplica deadzone-ul pe o axă
    float filterDeadzone(float value) const {
        if (std::fabs(value) < m_Deadzone) {
            return 0.0f;
        }
        // Opțional: scalează valoarea rămasă pentru a avea o tranziție lină de la 0 la 1
        float sign = (value > 0.0f) ? 1.0f : -1.0f;
        return sign * ((std::fabs(value) - m_Deadzone) / (1.0f - m_Deadzone));
    }

public:
    // Constructor: implicit citește primul controller și are un deadzone de 15%
    Gamepad(int joystickId = GLFW_JOYSTICK_1, float deadzone = 0.5f)
        : m_JoystickId(joystickId), m_Deadzone(deadzone), m_IsConnected(false) {
        m_State = {};
    }

    void bindDebugger(DebuggerClass *_imgui_helper)
    {
        imgui_helper = _imgui_helper;
    }

    // Actualizează starea controllerului (trebuie apelată în fiecare cadru/frame)
    void update() {
        imgui_helper->gamepadState = m_IsConnected;
        if (glfwJoystickPresent(m_JoystickId) && glfwJoystickIsGamepad(m_JoystickId)) {
            m_IsConnected = glfwGetGamepadState(m_JoystickId, &m_State);
        }
        else {
            m_IsConnected = false;
        }
    }

    bool isConnected() const { return m_IsConnected; }

    // --- GETTERI PENTRU AXE (cu deadzone aplicat) ---

    // Stick Stânga - Orizontal (Axa X) -> returnează [-1.0, 1.0]
    float getLeftStickX() const {
        if (!m_IsConnected) return 0.0f;
        return filterDeadzone(m_State.axes[GLFW_GAMEPAD_AXIS_LEFT_X]);
    }

    // Stick Stânga - Vertical (Axa Y) -> Inversat ca să fie: Sus = 1.0, Jos = -1.0
    float getLeftStickY() const {
        if (!m_IsConnected) return 0.0f;
        return -filterDeadzone(m_State.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);
    }

    // Stick Dreapta - Orizontal (Axa X) -> returnează [-1.0, 1.0]
    float getRightStickX() const {
        if (!m_IsConnected) return 0.0f;
        return filterDeadzone(m_State.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]);
    }

    // Stick Dreapta - Vertical (Axa Y) -> Inversat ca să fie: Sus = 1.0, Jos = -1.0
    float getRightStickY() const {
        if (!m_IsConnected) return 0.0f;
        return -filterDeadzone(m_State.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]);
    }

    // --- GETTERI PENTRU TRIGGERE (scalate de la 0.0 la 1.0) ---

    // Trigger Stânga (L2) -> returnează [0.0 (liber), 1.0 (apăsat maxim)]
    float getLeftTrigger() const {
        if (!m_IsConnected) return 0.0f;
        float rawValue = m_State.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]; // Vine ca [-1.0, 1.0]
        return (rawValue + 1.0f) / 2.0f; // Transformă în [0.0, 1.0]
    }

    // Trigger Dreapta (R2) -> returnează [0.0 (liber), 1.0 (apăsat maxim)]
    float getRightTrigger() const {
        if (!m_IsConnected) return 0.0f;
        float rawValue = m_State.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]; // Vine ca [-1.0, 1.0]
        return (rawValue + 1.0f) / 2.0f; // Transformă în [0.0, 1.0]
    }

    // --- METODĂ PENTRU BUTOANE ---
    bool isButtonPressed(int buttonId) const {
        if (!m_IsConnected) return false;
        return m_State.buttons[buttonId] == GLFW_PRESS;
    }
};