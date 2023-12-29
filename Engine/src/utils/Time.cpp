#include "Time.h"

namespace engine {

	Time::Time() {
		lastFrame = glfwGetTime();
		delta = 0;
	}

	void Time::update() {
		delta = glfwGetTime() - lastFrame;
		lastFrame = glfwGetTime();
	}

}