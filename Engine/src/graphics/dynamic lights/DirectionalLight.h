#pragma once
#include "DynamicLight.h"

namespace engine {
	namespace graphics {
		struct DirectionalLight : public DynamicLight {
			DirectionalLight();
			DirectionalLight(const glm::vec3& lightColor, const glm::vec3& dir);

			virtual void setupUniforms(Shader& shader, int currentLightIndex) override;

			glm::vec3 direction;
		};
	}
}


