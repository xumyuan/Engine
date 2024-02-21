#pragma once

#include "DynamicLight.h"

namespace engine {

	struct PointLight : public DynamicLight {
	public:
		PointLight(const glm::vec3& lightColor, const glm::vec3& pos);

		virtual void setupUniforms(Shader& shader, int currentLightIndex) override;


		glm::vec3 position;
		float constant, linear, quadratic;
	};

}