#pragma once
#include "DynamicLight.h"

namespace engine {
	struct DirectionalLight : public DynamicLight {
		DirectionalLight();
		DirectionalLight(const glm::vec3& lightColor, const glm::vec3& dir);

		glm::vec3 direction;
	};
}



