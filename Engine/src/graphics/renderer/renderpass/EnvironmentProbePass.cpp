#include "pch.h"
#include "EnvironmentProbePass.h"

#include <graphics/renderer/renderpass/LightingPass.h>
#include <graphics/renderer/renderpass/ShadowmapPass.h>

namespace engine
{

	EnvironmentProbePass::EnvironmentProbePass(Scene3D* scene) : RenderPass(scene, RenderPassType::EnvironmentProbePassType),
		m_CubemapGenerationFramebuffer(DEFAULT_IBL_RESOLUTION, DEFAULT_IBL_RESOLUTION)
	{
		m_CubemapGenerationFramebuffer.addDepthStencilRBO(false);
	}

	EnvironmentProbePass::~EnvironmentProbePass() {}

	void EnvironmentProbePass::pregenerateProbes() {
		// 创建帧缓冲用于光照探针
		Framebuffer shadowmapFramebuffer(DEFAULT_IBL_RESOLUTION, DEFAULT_IBL_RESOLUTION); // Creating without colour might make a depth only framebuffer :O
		shadowmapFramebuffer.addTexture2DColorAttachment(false).
			addDepthAttachment(false).createFramebuffer();
		Framebuffer lightingFramebuffer(DEFAULT_IBL_RESOLUTION, DEFAULT_IBL_RESOLUTION);
		lightingFramebuffer.addTexture2DColorAttachment(false).
			addDepthStencilRBO(false).createFramebuffer();

		glm::vec3 probePosition = glm::vec3(67.0f, 92.0f, 133.0f);
		glm::vec2 probeResolution = glm::vec2(DEFAULT_IBL_RESOLUTION, DEFAULT_IBL_RESOLUTION);
		EnvironmentProbe* iblProbe = new EnvironmentProbe(probePosition, probeResolution, true);
		iblProbe->generate();

		m_CubemapCamera.setCenterPosition(probePosition);
		ShadowmapPass shadowPass(m_ActiveScene, &shadowmapFramebuffer);
		LightingPass lightingPass(m_ActiveScene, &lightingFramebuffer, false);

		for (int i = 0; i < 6; i++) {
			// Setup the view
			m_CubemapCamera.switchCameraToFace(i);
			// Shadow pass
			ShadowmapPassOutput shadowpassOutput = shadowPass.generateShadowmaps(&m_CubemapCamera);

			// Light pass
			iblProbe->getIrradianceMap()->bind();
			lightingFramebuffer.bind();
			lightingFramebuffer.setColorAttachment(iblProbe->getIrradianceMap()->getCubemapID(), GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
			lightingPass.executeRenderPass(shadowpassOutput, &m_CubemapCamera);
		}
		// Temp cleanup
		EnvironmentProbeManager* probeManager = m_ActiveScene->getProbeManager();
		probeManager->addProbe(iblProbe);
	}

}