#include "pch.h"
#include "ShadowmapPass.h"

#include <utils/loaders/ShaderLoader.h>

namespace engine
{

	ShadowmapPass::ShadowmapPass(Scene3D* scene) : RenderPass(scene, RenderPassType::ShadowmapPassType), m_OwnsRT(true)
	{
		m_ShadowmapShader = ShaderLoader::loadShader("Shaders/shadowmap.glsl");
		m_RT = new RenderTarget(SHADOWMAP_RESOLUTION_X, SHADOWMAP_RESOLUTION_Y);
		m_RT->addDepthStencilTexture(DepthStencilFormat::DepthOnly).build();
	}

	ShadowmapPass::ShadowmapPass(Scene3D* scene, RenderTarget* customRT) : RenderPass(scene, RenderPassType::ShadowmapPassType), m_RT(customRT), m_OwnsRT(false)
	{
		m_ShadowmapShader = ShaderLoader::loadShader("Shaders/shadowmap.glsl");
	}

	ShadowmapPass::~ShadowmapPass() {
		if (m_OwnsRT) {
			delete m_RT;
		}
	}

	ShadowmapPassOutput ShadowmapPass::generateShadowmaps(ICamera* camera) {
		m_RT->beginPass();

		// Setup
		ModelRenderer* modelRenderer = m_ActiveScene->getModelRenderer();
		Terrain* terrain = m_ActiveScene->getTerrain();
		DynamicLightManager* lightManager = m_ActiveScene->getDynamicLightManager();

		// View setup
		m_GLCache->switchShader(m_ShadowmapShader);
		glm::vec3 dirLightShadowmapLookAtPos = camera->getPosition() + (glm::normalize(camera->getFront()) * 50.0f);
		glm::vec3 dirLightShadowmapEyePos = dirLightShadowmapLookAtPos + (-lightManager->getDirectionalLightDirection() * 100.0f);
		glm::mat4 directionalLightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, SHADOWMAP_NEAR_PLANE, SHADOWMAP_FAR_PLANE);
		glm::mat4 directionalLightView = glm::lookAt(dirLightShadowmapEyePos, dirLightShadowmapLookAtPos, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 directionalLightViewProjMatrix = directionalLightProjection * directionalLightView;
		m_ShadowmapShader->setUniform("lightSpaceViewProjectionMatrix", directionalLightViewProjMatrix);

		// Render models
		m_ActiveScene->addModelsToRenderer();
		modelRenderer->flushOpaque(m_ShadowmapShader, m_RenderPassType);
		modelRenderer->flushTransparent(m_ShadowmapShader, m_RenderPassType);

		// Render terrain
		terrain->Draw(m_ShadowmapShader, m_RenderPassType);

		m_RT->endPass();

		// Render pass output
		ShadowmapPassOutput passOutput;
		passOutput.directionalLightViewProjMatrix = directionalLightViewProjMatrix;
		passOutput.renderTarget = m_RT->getHandle();
		passOutput.depthTexture = m_RT->getDepthStencilTexture();
		return passOutput;
	}

}
