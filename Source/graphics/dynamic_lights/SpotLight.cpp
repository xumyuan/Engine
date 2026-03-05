#include "pch.h"
#include "SpotLight.h"

namespace engine {

	SpotLight::SpotLight(const glm::vec3& lightColor, const glm::vec3& pos, const glm::vec3& dir, float cutOffAngle, float outerCutOffAngle)
		: DynamicLight(lightColor), position(pos), direction(dir), cutOff(cutOffAngle), outerCutOff(outerCutOffAngle) {
	}

	// TODO: 添加多聚光支持
	void SpotLight::setupUniforms(Shader* shader, int currentLightIndex) {
		if (isActive) {
			shader->setUniform(("spotLights[" + std::to_string(currentLightIndex) + "].position").c_str(), position);
			shader->setUniform(("spotLights[" + std::to_string(currentLightIndex) + "].direction").c_str(), direction);
			shader->setUniform(("spotLights[" + std::to_string(currentLightIndex) + "].lightColour").c_str(), lightColor);
			shader->setUniform(("spotLights[" + std::to_string(currentLightIndex) + "].cutOff").c_str(), cutOff);
			shader->setUniform(("spotLights[" + std::to_string(currentLightIndex) + "].outerCutOff").c_str(), outerCutOff);
			shader->setUniform(("spotLights[" + std::to_string(currentLightIndex) + "].intensity").c_str(), intensity);
			shader->setUniform(("spotLights[" + std::to_string(currentLightIndex) + "].attenuationRadius").c_str(), attenuationRadius);

		}
	}

}
