#pragma once
#include <GLFW\glfw3.h>

namespace engine {

	class Timer {
	private:
		double startTime;
	public:
		Timer() {
			startTime = glfwGetTime();
		}

		void reset() {
			startTime = glfwGetTime();
		}

		double elapsed() {
			return glfwGetTime() - startTime;
		}
	};

} 