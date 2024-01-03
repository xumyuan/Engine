#include "Timer.h"

namespace engine {

	Timer::Timer() {
		startTime = glfwGetTime();
	}

	void Timer::reset() {
		startTime = glfwGetTime();
	}

	double Timer::elapsed() {
		return glfwGetTime() - startTime;
	}

};