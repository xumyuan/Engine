#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace engine {
	namespace graphics {

#define MAX_KEYS 1024
#define MAX_BUTTONS 32

		class Window {
		private:
			const char* m_Title;
			GLFWwindow* m_Window;
			bool m_HideCursor;

			static int m_Width, m_Height;
			static bool s_Keys[MAX_KEYS];
			static bool s_Buttons[MAX_BUTTONS];
			static double s_MouseX, s_MouseY, s_MouseXDelta, s_MouseYDelta;
			static double s_ScrollX, s_ScrollY;

		public:
			Window(const char* title, int width, int height);
			~Window();
			void update();
			void close() const;
			bool closed() const;
			static void clear();

			static bool isKeyPressed(unsigned int keycode);
			static bool isMouseButtonPressed(unsigned int keycode);

			static inline double getMouseX() { return s_MouseX; }
			static inline double getMouseY() { return s_MouseY; }
			static inline double getMouseXDelta() { return s_MouseXDelta; }
			static inline double getMouseYDelta() { return s_MouseYDelta; }
			static inline double getScrollX() { return s_ScrollX; }
			static inline double getScrollY() { return s_ScrollY; }
			static inline void resetScroll() { s_ScrollX = 0; s_ScrollY = 0; }
			static inline void getMousePosition(double& x, double& y) { x = s_MouseX; y = s_MouseY; }
			static inline int getWidth() { return m_Width; }
			static inline int getHeight() { return m_Height; }

		private:
			bool init();
			void setFullScreenResolution();
			static friend void error_callback(int error, const char* description);
			static friend void window_resize(GLFWwindow* window, int width, int height);
			static friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
			static friend void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
			static friend void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
			static friend void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
			static friend void char_callback(GLFWwindow* window, unsigned int c);
		};

	}
}