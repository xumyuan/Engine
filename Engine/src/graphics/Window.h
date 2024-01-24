#pragma once

#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <iostream>
#include "../utils/Logger.h"
#include "../Defs.h"

namespace engine {
	namespace graphics {

#define MAX_KEYS 1024
#define MAX_BUTTONS 32

		class Window {
		private:
			const char* m_Title;
			int m_Width, m_Height;
			GLFWwindow* m_Window;


			bool m_Keys[MAX_KEYS];
			bool m_Buttons[MAX_BUTTONS];
			double mx, my;
			double scrollX, scrollY;
		public:
			Window(const char* title, int width, int height);
			~Window();
			void update();
			void clear() const;
			void close() const;
			bool closed() const;
			bool isKeyPressed(unsigned int keycode) const;
			bool isMouseButtonPressed(unsigned int keycode) const;

			inline double getMouseX() const { return mx; }
			inline double getMouseY() const { return my; }
			inline double getScrollX() const { return scrollX; }
			inline double getScrollY() const { return scrollY; }
			inline void resetScroll() { scrollX = 0; scrollY = 0; }
			inline void getMousePosition(double& x, double& y) { x = mx; y = my; }
			inline int getWidth() const { return m_Width; }
			inline int getHeight() const { return m_Height; }
		private:
			bool init();
			void setFullScreenResolution();
			static friend void error_callback(int error, const char* description);
			static friend void window_resize(GLFWwindow* window, int width, int height);
			static friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
			static friend void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
			static friend void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
			static friend void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
		};

	}
}