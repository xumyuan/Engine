#pragma once

#include <string>
#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <SOIL/SOIL.h>
#include <iostream>
#include "stb_image.h"

namespace engine {
	namespace opengl {

		class Utility {
		public:
			static GLuint loadTextureFromFile(const char* path);
		};

	}
}