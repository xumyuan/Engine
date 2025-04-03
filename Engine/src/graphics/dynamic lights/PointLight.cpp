#include "pch.h"
#include "PointLight.h"

namespace engine {

	PointLight::PointLight(const glm::vec3& lightColor, const glm::vec3& pos)
		: DynamicLight(lightColor), position(pos) {}

	// TODO: Assert that the shader is bound in debug
	void PointLight::setupUniforms(Shader* shader, int currentLightIndex) {
		if (isActive) {
			shader->setUniform(("pointLights[" + std::to_string(currentLightIndex)
				+ "].position").c_str(), position);
			shader->setUniform(("pointLights[" + std::to_string(currentLightIndex) + "].lightColour").c_str(), lightColor);
		}
	}

}