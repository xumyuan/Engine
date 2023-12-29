#pragma once

#include <GLFW\glfw3.h>
#include <gl\GL.h>

namespace engine {

	struct Time {
		GLdouble lastFrame;
		GLdouble delta;

		Time();
		void update();
	};

}