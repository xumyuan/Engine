#include "pch.h"
#include "SpotLight.h"

namespace engine {

	SpotLight::SpotLight(const glm::vec3& lightColor, const glm::vec3& pos, const glm::vec3& dir, float cutOffAngle, float outerCutOffAngle)
		: DynamicLight(lightColor), position(pos), direction(dir), cutOff(cutOffAngle), outerCutOff(outerCutOffAngle) {}

	// TODO: 添加多聚光支持
	void SpotLight::setupUniforms(Shader* shader, int currentLightIndex) {
		if (isActive) {
			shader->setUniform("spotLight.position", position);
			shader->setUniform("spotLight.direction", direction);
			shader->setUniform("spotLight.lightColour", lightColor);
			shader->setUniform("spotLight.cutOff", cutOff);
			shader->setUniform("spotLight.outerCutOff", outerCutOff);
		}
	}

}