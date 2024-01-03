#pragma once

#include <GLFW\glfw3.h>
#include <gl\GL.h>

namespace engine {

	struct Time {
	private:
		GLdouble lastFrame;
		GLdouble delta;
	public:
		Time();
		void update();

		inline GLdouble getDeltaTime() { return delta; }
	};

}