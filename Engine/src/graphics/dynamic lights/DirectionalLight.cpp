#include "pch.h"
#include "DirectionalLight.h"

namespace engine {
	DirectionalLight::DirectionalLight(const glm::vec3& lightColor, const glm::vec3& dir)
		: DynamicLight(lightColor), direction(dir) {}
	// TODO: 添加多方向光支持
	void DirectionalLight::setupUniforms(Shader* shader, int currentLightIndex) {
		if (isActive) {
			shader->setUniform3f("dirLight.direction", direction);
			shader->setUniform3f("dirLight.lightColour", lightColor);
		}
	}

}