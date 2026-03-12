#pragma once

#include <glm/glm.hpp>

namespace engine {

	struct DynamicLight {
		DynamicLight(const glm::vec3& lightColor);

		glm::vec3 lightColor;
		bool isActive;
		float intensity = 1.0f;
	};

}
