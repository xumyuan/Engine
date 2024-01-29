#include "SpotLight.h"

namespace engine {
	namespace graphics {

		SpotLight::SpotLight(glm::vec3& amb, glm::vec3& diff, glm::vec3& spec, glm::vec3& pos, glm::vec3& dir, float cutOffAngle, float outerCutOffAngle, float cons, float lin, float quad)
			: DynamicLight(amb, diff, spec), position(pos), direction(dir), cutOff(cutOffAngle), outerCutOff(outerCutOffAngle), constant(cons), linear(lin), quadratic(quad) {}

		SpotLight::SpotLight(const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& spec, const glm::vec3& pos, const glm::vec3& dir, float cutOffAngle, float outerCutOffAngle, float cons, float lin, float quad)
			: DynamicLight(amb, diff, spec), position(pos), direction(dir), cutOff(cutOffAngle), outerCutOff(outerCutOffAngle), constant(cons), linear(lin), quadratic(quad) {}

		// TODO: 添加多聚光支持
		void SpotLight::setupUniforms(Shader& shader, int currentLightIndex) {
			if (isActive) {
				shader.setUniform3f("spotLight.position", position);
				shader.setUniform3f("spotLight.direction", direction);
				shader.setUniform3f("spotLight.ambient", ambient);
				shader.setUniform3f("spotLight.diffuse", diffuse);
				shader.setUniform3f("spotLight.specular", specular);
				shader.setUniform1f("spotLight.cutOff", cutOff);
				shader.setUniform1f("spotLight.outerCutOff", outerCutOff);
				shader.setUniform1f("spotLight.constant", constant);
				shader.setUniform1f("spotLight.linear", linear);
				shader.setUniform1f("spotLight.quadratic", quadratic);
			}
		}

	}
}