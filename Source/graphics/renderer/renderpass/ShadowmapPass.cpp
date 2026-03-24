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
		// 通过命令缓冲录制 beginRenderPass
		rhi::RenderPassParams params;
		params.viewport = { 0, 0, m_RT->getWidth(), m_RT->getHeight() };
		params.clearColorFlag = true;
		params.clearDepthFlag = true;
		cmd().beginRenderPass(m_RT->getHandle(), params);

		// Setup
		ModelRenderer* modelRenderer = m_RenderScene.modelRenderer;
		Terrain* terrain = m_RenderScene.terrain;
		LightCollector* lightCollector = m_RenderScene.lightCollector;

		// 通过命令缓冲录制管线绑定
		rhi::PipelineState pipeline;
		pipeline.program = m_ShadowmapShader->getProgramHandle();
		pipeline.depthTest = true;
		pipeline.depthWrite = true;
		pipeline.cullMode = rhi::CullMode::Back;
		cmd().bindPipeline(pipeline);

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
			cmd().updateBuffer(uboMgr->getCustomHandle(), &shadowParams, sizeof(UBOShadowmapPass));
			cmd().bindUBO(UBOBinding::CustomParams,
				uboMgr->getCustomHandle(), sizeof(UBOShadowmapPass));
		}

		// Render models
		m_RenderScene.submitModelsToRenderer();
		rhi::ProgramHandle shadowProgram = m_ShadowmapShader->getProgramHandle();
		modelRenderer->flushOpaque(cmd(), shadowProgram, m_RenderPassType);
		modelRenderer->flushTransparent(cmd(), shadowProgram, m_RenderPassType);

		// Render terrain
		terrain->Draw(cmd(), shadowProgram, m_RenderPassType);

		// 通过命令缓冲录制 endRenderPass
		cmd().endRenderPass();

		// Render pass output
		ShadowmapPassOutput passOutput;
		passOutput.directionalLightViewProjMatrix = directionalLightViewProjMatrix;
		passOutput.renderTarget = m_RT->getHandle();
		passOutput.depthTexture = m_RT->getDepthStencilTexture();
		return passOutput;
	}

}
