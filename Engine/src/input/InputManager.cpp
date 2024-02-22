#include "pch.h"
#include "InputManager.h"

namespace engine {

	// Static declarations
	bool InputManager::s_Keys[MAX_KEYS];
	float InputManager::s_KeyPressure[MAX_KEYS];
	bool InputManager::s_Buttons[MAX_BUTTONS];
	double InputManager::s_MouseX, InputManager::s_MouseY, InputManager::s_MouseXDelta, InputManager::s_MouseYDelta;
	double InputManager::s_ScrollX, InputManager::s_ScrollY;

	InputManager::InputManager() {
		s_ScrollX = s_ScrollY = 0;
		s_MouseXDelta = s_MouseYDelta = 0;

		memset(s_Keys, 0, sizeof(bool) * MAX_KEYS);
		memset(s_KeyPressure, 0, sizeof(float) * MAX_KEYS);
		memset(s_Buttons, 0, sizeof(bool) * MAX_BUTTONS);
	}

	InputManager::~InputManager() {}

	void InputManager::update() {
		s_MouseXDelta = s_MouseYDelta = 0;
		s_ScrollX = 0; s_ScrollY = 0;

		m_JoystickManager.update();
	}

	bool InputManager::isKeyPressed(unsigned int keycode) {
		if (keycode >= MAX_KEYS) {
			Logger::getInstance().error("logged_files/input_errors.txt", "Input Check", "Key checked is out of bounds (ie not supported)");
			return false;
		}
		else {
			return s_Keys[keycode];
		}
	}

	float InputManager::getKeyPressure(unsigned int keycode) {
		if (keycode >= MAX_KEYS) {
			Logger::getInstance().error("logged_files/input_errors.txt", "Input Check", "Key checked is out of bounds (ie not supported)");
			return 0.0f;
		}
		else {
			return s_KeyPressure[keycode];
		}
	}

	bool InputManager::isMouseButtonPressed(unsigned int code) {
		if (code >= MAX_BUTTONS) {
			Logger::getInstance().error("logged_files/input_errors.txt", "Input Check", "Mouse button checked is out of bounds (ie not supported)");
			return false;
		}
		else {
			return s_Buttons[code];
		}
	}


	void InputManager::keyCallback(int key, int scancode, int action, int mods) {
		s_Keys[key] = action != GLFW_RELEASE;
		s_KeyPressure[key] = s_Keys[key];
	}

	void InputManager::mouseButtonCallback(int button, int action, int mods) {
		s_Buttons[button] = action != GLFW_RELEASE;
	}

	void InputManager::cursorPositionCallback(double xpos, double ypos) {
		s_MouseXDelta = xpos - s_MouseX;
		s_MouseYDelta = ypos - s_MouseY;
		s_MouseX = xpos;
		s_MouseY = ypos;
	}

	void InputManager::scrollCallback(double xoffset, double yoffset) {
		s_ScrollX = xoffset;
		s_ScrollY = yoffset;
	}

	void InputManager::joystickCallback(int joystick, int event) {
		m_JoystickManager.joystickConnectionCallback(joystick, event);
	}

}