#include "pch.h"
#include "Timer.h"

#include <GLFW/glfw3.h>

namespace engine {

	Timer::Timer() {
		startTime = glfwGetTime();
	}

	void Timer::reset() {
		startTime = glfwGetTime();
	}

	

};