#include "pch.h"
#include "LightProbe.h"

namespace engine {

	LightProbe::LightProbe(glm::vec3& probePosition, glm::vec2& probeResolution) :
		m_Position(probePosition),
		m_ProbeResolution(probeResolution),
		m_IrradianceMap(nullptr),
		m_Generated(false) {

	}

	LightProbe::~LightProbe() {
		delete m_IrradianceMap;
	}

	void LightProbe::generate() {
		// 生成环境探针并设置生成标志
		CubemapSettings settings;
		m_IrradianceMap = new Cubemap(settings);
		for (int i = 0; i < 6; i++) {
			m_IrradianceMap->generateCubemapFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, (unsigned int)m_ProbeResolution.x, (unsigned int)m_ProbeResolution.y, GL_RGB, nullptr);
		}

		m_Generated = true;
	}

	void LightProbe::bind(Shader* shader) {
		m_IrradianceMap->bind(1);
		shader->setUniform("irradianceMap", 1);
	}

}
