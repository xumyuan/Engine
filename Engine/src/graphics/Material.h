#pragma once
#include <GL\glew.h>
#include <GLFW\glfw3.h>

namespace engine {
	namespace graphics {
		class Material
		{
			//����PBR
			GLuint diffuseMap, specularMap, emissionMap;
			float shininess;
			GLuint normalMap;
		};
	}
}


