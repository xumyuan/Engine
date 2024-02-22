#include "pch.h"
#include "EnvironmentProbe.h"

namespace engine {

	EnvironmentProbe::EnvironmentProbe(glm::vec3& probePosition, glm::vec2& probeResolution, bool isStatic) {

	}

	void EnvironmentProbe::generate() {
		// Generate the environment probe and set the generated position, and also set the generated flag
	}

	void EnvironmentProbe::bind(Shader& shader) {
		m_IrradianceMap->bind(1);
		shader.setUniform1i("irradianceMap", 1);
		m_PrefilterMap->bind(2);
		shader.setUniform1i("prefilterMap", 2);
		m_BRDF_LUT->bind(3);
		shader.setUniform1i("brdfLUT", 3);
	}

}