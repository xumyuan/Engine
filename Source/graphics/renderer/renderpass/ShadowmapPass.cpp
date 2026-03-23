#include "pch.h"
#include "ShadowmapPass.h"

#include <utils/loaders/ShaderLoader.h>
#include <graphics/UniformBufferManager.h>

namespace engine
{

	ShadowmapPass::ShadowmapPass(const RenderScene& renderScene) : RenderPass(renderScene, RenderPassType::ShadowmapPassType), m_OwnsRT(true)
	{
		m_ShadowmapShader = ShaderLoader::loadShader("Shaders/shadowmap.glsl");
		m_RT = new RenderTarget(SHADOWMAP_RESOLUTION_X, SHADOWMAP_RESOLUTION_Y);
		m_RT->addDepthStencilTexture(DepthStencilFormat::DepthOnly).build();
	}

	ShadowmapPass::ShadowmapPass(const RenderScene& renderScene, RenderTarget* customRT) : RenderPass(renderScene, RenderPassType::ShadowmapPassType), m_RT(customRT), m_OwnsRT(false)
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
		ModelRenderer* modelRenderer = m_RenderScene.modelRenderer;
		Terrain* terrain = m_RenderScene.terrain;
		LightCollector* lightCollector = m_RenderScene.lightCollector;

		// 通过 PipelineState 设置 shader 和渲染状态
		rhi::PipelineState pipeline;
		pipeline.program = m_ShadowmapShader->getProgramHandle();
		pipeline.depthTest = true;
		pipeline.depthWrite = true;
		pipeline.cullMode = rhi::CullMode::Back;
		bindPipelineState(pipeline);

		// View setup
		glm::vec3 dirLightShadowmapLookAtPos = camera->getPosition() + (glm::normalize(camera->getFront()) * 50.0f);
		glm::vec3 dirLightShadowmapEyePos = dirLightShadowmapLookAtPos + (-lightCollector->getDirectionalLightDirection(m_RenderScene.rootNode) * 100.0f);
		glm::mat4 directionalLightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, SHADOWMAP_NEAR_PLANE, SHADOWMAP_FAR_PLANE);
		glm::mat4 directionalLightView = glm::lookAt(dirLightShadowmapEyePos, dirLightShadowmapLookAtPos, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 directionalLightViewProjMatrix = directionalLightProjection * directionalLightView;
		// Shadowmap 参数通过 Custom UBO (binding 4) 传递
		if (auto* uboMgr = getUBOManager()) {
			UBOShadowmapPass shadowParams{};
			shadowParams.lightSpaceViewProjectionMatrix = directionalLightViewProjMatrix;
			uboMgr->updateShadowmapPass(shadowParams);
			uboMgr->bindCustom(sizeof(UBOShadowmapPass));
		}

		// Render models
		m_RenderScene.submitModelsToRenderer();
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
