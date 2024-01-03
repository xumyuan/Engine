#pragma once
#include <GLFW\glfw3.h>

namespace engine {

	class Timer {
	private:
		double startTime;
	public:
		Timer();

		void reset();

		inline double elapsed() { return glfwGetTime() - startTime; }
	};

}