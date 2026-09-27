#include "pch.h"
#include "ProbeManager.h"
#include "graphics/UniformBufferManager.h"
#include "graphics/TextureBindings.h"

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

	void ProbeManager::bindProbe(glm::vec3& renderPosition, rhi::CommandBuffer& cmd) {
		rhi::TextureHandle skyboxCubemap = m_Skybox->getSkyboxCubemap()->getRHIHandle();

		// If simple blending is enabled just use the closest probe
		if (m_ProbeBlendSetting == PROBES_SIMPLE) {
			if (m_LightProbes.size() > 0) {
				m_LightProbes[0]->bind(cmd);
			}
			else {
				cmd.bindTextureUnit(skyboxCubemap, TextureUnit::IrradianceMap);
			}

			// reflectionProbeMipCount 通过 IBLParams UBO 由调用方设置
			if (m_ReflectionProbes.size() > 0) {
				m_ReflectionProbes[0]->bind(cmd);
			}
			else {
				cmd.bindTextureUnit(skyboxCubemap, TextureUnit::PrefilterMap);
				cmd.bindTextureUnit(ReflectionProbe::getBRDFLUT()->getRHIHandle(), TextureUnit::BrdfLUT);
			}
		}
		// If probes are disabled just use the skybox
		else if (m_ProbeBlendSetting == PROBES_DISABLED) {
			cmd.bindTextureUnit(skyboxCubemap, TextureUnit::IrradianceMap);
			cmd.bindTextureUnit(skyboxCubemap, TextureUnit::PrefilterMap);
			cmd.bindTextureUnit(ReflectionProbe::getBRDFLUT()->getRHIHandle(), TextureUnit::BrdfLUT);
		}
	}
}