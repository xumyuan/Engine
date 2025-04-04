#pragma once

#include "DynamicLight.h"

namespace engine {

	struct SpotLight : public DynamicLight {
	public:
		SpotLight(const glm::vec3& lightColor, const glm::vec3& pos, const glm::vec3& dir, float cutOffAngle, float outerCutOffAngle);


		virtual void setupUniforms(Shader* shader, int currentLightIndex = 0) override;

		glm::vec3 position, direction;
		float cutOff, outerCutOff;
	};

}