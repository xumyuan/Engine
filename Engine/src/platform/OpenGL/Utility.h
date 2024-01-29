#pragma once

#include <string>
#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <SOIL/SOIL.h>
#include <iostream>
#include "stb_image.h"
#include <vector>
#include <glm/common.hpp>
#include "../../utils/Logger.h"
#include "../../Defs.h"

namespace engine {
	namespace opengl {

		class Utility {
		public:
			//static GLuint loadTextureFromFile(const char* path, bool containsTransparencyOnSides = false);

			static GLuint loadCubemapFromFiles(const std::vector<const char*>& vec);
		};

	}
}