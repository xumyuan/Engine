#include "PointLight.h"

namespace engine {
	namespace graphics {

		PointLight::PointLight(const glm::vec3& lightColor, const glm::vec3& pos)
			: DynamicLight(lightColor), position(pos) {}

		// TODO: Assert that the shader is bound in debug
		void PointLight::setupUniforms(Shader& shader, int currentLightIndex) {
			if (isActive) {
				shader.setUniform3f(("pointLights[" + std::to_string(currentLightIndex)
					+ "].position").c_str(), position);
				shader.setUniform3f(("pointLights[" + std::to_string(currentLightIndex) + "].lightColour").c_str(), lightColor);
			}
		}

	}
}