#pragma once

#include <glm\common.hpp>

#include "../Shader.h"

namespace engine {
	namespace graphics {

		struct DynamicLight {
			DynamicLight(glm::vec3& amb, glm::vec3& diff, glm::vec3& spec);

			DynamicLight(const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& spec);

			virtual void setupUniforms(Shader& shader, int currentLightIndex) = 0;


			glm::vec3 ambient, diffuse, specular;
			bool isActive;
		};

	}
}