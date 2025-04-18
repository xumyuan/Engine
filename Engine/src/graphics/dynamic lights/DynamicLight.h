#pragma once

#include "graphics/Shader.h"

namespace engine {

	struct DynamicLight {
		DynamicLight(const glm::vec3& lightColor);

		virtual void setupUniforms(Shader* shader, int currentLightIndex) = 0;


		glm::vec3 lightColor;
		bool isActive;
		float intensity = 1.0f;
	};

}