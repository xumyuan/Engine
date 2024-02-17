#include "pch.h"
#include "Timer.h"

namespace engine {

	Timer::Timer() {
		startTime = glfwGetTime();
	}

	void Timer::reset() {
		startTime = glfwGetTime();
	}

	

};