#include "pch.h"
#include "DirectionalLight.h"

namespace engine {
	DirectionalLight::DirectionalLight(const glm::vec3& lightColor, const glm::vec3& dir)
		: DynamicLight(lightColor), direction(dir) {}
	// TODO: ��Ӷ෽���֧��
	void DirectionalLight::setupUniforms(Shader* shader, int currentLightIndex) {
		if (isActive) {
			shader->setUniform(("dirLights[" + std::to_string(currentLightIndex) + "].direction").c_str(), direction);
			shader->setUniform(("dirLights[" + std::to_string(currentLightIndex) + "].lightColour").c_str(), lightColor);
			shader->setUniform(("dirLights[" + std::to_string(currentLightIndex) + "].intensity").c_str(), intensity);
		}
	}
}