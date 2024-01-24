#include "DirectionalLight.h"
namespace engine {
	namespace graphics {
		DirectionalLight::DirectionalLight(glm::vec3& amb, glm::vec3& diff, glm::vec3& spec, glm::vec3& dir)
			: DynamicLight(amb, diff, spec), direction(dir) {}

		DirectionalLight::DirectionalLight(const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& spec, const glm::vec3& dir)
			: DynamicLight(amb, diff, spec), direction(dir) {}
		// TODO: 添加多方向光支持
		void DirectionalLight::setupUniforms(Shader& shader, int currentLightIndex) {
			if (isActive) {
				shader.setUniform3f("dirLight.ambient", ambient);
				shader.setUniform3f("dirLight.diffuse", diffuse);
				shader.setUniform3f("dirLight.specular", specular);
				shader.setUniform3f("dirLight.direction", direction);
			}
		}

	}
}