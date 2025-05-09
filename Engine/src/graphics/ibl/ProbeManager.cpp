#include "pch.h"
#include "ProbeManager.h"

namespace engine {

	ProbeManager::ProbeManager(ProbeBlendSetting sceneProbeBlendSetting)
		: m_ProbeBlendSetting(sceneProbeBlendSetting), m_Skybox(nullptr)
	{}

	ProbeManager::~ProbeManager() {
		for (auto iter = m_LightProbes.begin(); iter != m_LightProbes.end(); ++iter) {
			delete (*iter);
		}
		for (auto iter = m_ReflectionProbes.begin(); iter != m_ReflectionProbes.end(); ++iter) {
			delete (*iter);
		}
		m_LightProbes.clear();
		m_ReflectionProbes.clear();
	}

	void ProbeManager::init(Skybox* skybox) {
		m_Skybox = skybox;
	}

	void ProbeManager::addProbe(LightProbe* probe) {
		m_LightProbes.push_back(probe);
	}

	void ProbeManager::addProbe(ReflectionProbe* probe) {
		m_ReflectionProbes.push_back(probe);
	}

	void ProbeManager::bindProbe(glm::vec3& renderPosition, Shader* shader) {
		// If simple blending is enabled just use the closest probe
		if (m_ProbeBlendSetting == PROBES_SIMPLE) {
			// Light Probes
			if (m_LightProbes.size() > 0) {
				m_LightProbes[0]->bind(shader);
			}
			else {
				// Fallback to skybox
				m_Skybox->getSkyboxCubemap()->bind(1);
				shader->setUniform("irradianceMap", 1);
			}

			// Reflection Probes
			if (m_ReflectionProbes.size() > 0) {
				m_ReflectionProbes[0]->bind(shader);
			}
			else {
				// Fallback to skybox
				shader->setUniform("reflectionProbeMipCount", REFLECTION_PROBE_MIP_COUNT);
				m_Skybox->getSkyboxCubemap()->bind(2);
				shader->setUniform("prefilterMap", 2);
				ReflectionProbe::getBRDFLUT()->bind(3);
				shader->setUniform("brdfLUT", 3);
			}
		}
		// If probes are disabled just use the skybox
		else if (m_ProbeBlendSetting == PROBES_DISABLED) {
			// Light Probes
			m_Skybox->getSkyboxCubemap()->bind(1);
			shader->setUniform("irradianceMap", 1);

			// Reflection Probes
			shader->setUniform("reflectionProbeMipCount", REFLECTION_PROBE_MIP_COUNT);
			m_Skybox->getSkyboxCubemap()->bind(2);
			shader->setUniform("prefilterMap", 2);
			ReflectionProbe::getBRDFLUT()->bind(3);
			shader->setUniform("brdfLUT", 3);
		}
	}
}