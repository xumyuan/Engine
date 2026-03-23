#include "pch.h"
#include "ForwardLightingPass.h"

#include "physics/fluid/FluidSim.h"
#include "graphics/Window.h"
#include "graphics/camera/FPSCamera.h"

#include <utils/loaders/ShaderLoader.h>
#include <graphics/UniformBufferManager.h>

namespace engine
{

	ForwardLightingPass::ForwardLightingPass(const RenderScene& renderScene) : RenderPass(renderScene, RenderPassType::LightingPassType), m_OwnsRT(true)
	{
		m_ModelShader = ShaderLoader::loadShader("Shaders/forward/pbr_model.glsl");
		m_TerrainShader = ShaderLoader::loadShader("Shaders/forward/pbr_terrain.glsl");
		uint8_t samples = MSAA_SAMPLE_AMOUNT > 1 ? static_cast<uint8_t>(MSAA_SAMPLE_AMOUNT) : 1;
		m_RT = new RenderTarget(Window::getWidth(), Window::getHeight(), samples);

		m_RT->addColorTexture(rhi::TextureFormat::RGBA16F)
			.addDepthStencilTexture(DepthStencilFormat::DepthStencil, false).build();
	}

	ForwardLightingPass::ForwardLightingPass(const RenderScene& renderScene, RenderTarget* customRT) : RenderPass(renderScene, RenderPassType::LightingPassType), m_RT(customRT), m_OwnsRT(false)
	{
		m_ModelShader = ShaderLoader::loadShader("Shaders/forward/pbr_model.glsl");
		m_TerrainShader = ShaderLoader::loadShader("Shaders/forward/pbr_terrain.glsl");
	}

	ForwardLightingPass::~ForwardLightingPass() {
		if (m_OwnsRT) {
			delete m_RT;
		}
	}

	LightingPassOutput ForwardLightingPass::executeRenderPass(ShadowmapPassOutput& shadowmapData, ICamera* camera, bool useIBL) {
		// 通过命令缓冲录制 beginRenderPass
		rhi::RenderPassParams passParams;
		passParams.viewport = { 0, 0, m_RT->getWidth(), m_RT->getHeight() };
		passParams.clearColorFlag = true;
		passParams.clearDepthFlag = true;
		cmd().beginRenderPass(m_RT->getHandle(), passParams);

		// Setup
		ModelRenderer* modelRenderer = m_RenderScene.modelRenderer;
		Terrain* terrain = m_RenderScene.terrain;
		FluidSim* fluid = m_RenderScene.fluid;
		LightCollector* lightCollector = m_RenderScene.lightCollector;
		Skybox* skybox = m_RenderScene.skybox;
		ProbeManager* probeManager = m_RenderScene.probeManager;

		// 通过命令缓冲录制管线绑定
		rhi::PipelineState pipeline;
		pipeline.program = m_ModelShader->getProgramHandle();
		pipeline.depthTest = true;
		pipeline.depthWrite = true;
		pipeline.cullMode = rhi::CullMode::Back;
		pipeline.blendEnable = false;
		pipeline.multisample = m_RT->isMultisampled();
		cmd().bindPipeline(pipeline);

		// View setup + lighting setup via UBO（高层操作仍直接调用）
		auto* uboMgr = getUBOManager();
		if (uboMgr) {
			// PerFrame UBO
			uboMgr->updatePerFrame(camera->getViewMatrix(), camera->getProjectionMatrix(),
				camera->getPosition());
			uboMgr->bindPerFrame();

			// Lighting UBO
			auto& lightingUBO = uboMgr->getLightingData();
			lightCollector->fillLightingUBO(m_RenderScene.rootNode, lightingUBO);
			uboMgr->updateLighting();
			uboMgr->bindLighting();
		}
		// Shadowmap code
		bindShadowmap(m_ModelShader, shadowmapData);
		// IBL code
		UBOIBLParams iblParams{};
		iblParams.reflectionProbeMipCount = REFLECTION_PROBE_MIP_COUNT;
		if (useIBL) {
			iblParams.computeIBL = 1;
			glm::vec3 renderPos(0.0f, 0.0f, 0.0f);
			probeManager->bindProbe(renderPos, m_ModelShader);
		}
		else {
			iblParams.computeIBL = 0;
			Skybox* skyboxForBind = m_RenderScene.skybox;
			if (skyboxForBind && skyboxForBind->getSkyboxCubemap()) {
				skyboxForBind->getSkyboxCubemap()->bind(1);
				m_ModelShader->setUniform("irradianceMap", 1);
				skyboxForBind->getSkyboxCubemap()->bind(2);
				m_ModelShader->setUniform("prefilterMap", 2);
			}
		}
		if (uboMgr) {
			uboMgr->updateIBLParams(iblParams);
			uboMgr->bindCustom(sizeof(UBOIBLParams));
		}
		// Render the scene
		m_RenderScene.submitModelsToRenderer();
		modelRenderer->flushOpaque(m_ModelShader, m_RenderPassType);

		// 切换 terrain shader 管线
		rhi::PipelineState terrainPipeline = pipeline;
		terrainPipeline.program = m_TerrainShader->getProgramHandle();
		cmd().bindPipeline(terrainPipeline);

		// Terrain PerObject UBO
		glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), terrain->getPosition());
		if (uboMgr) {
			uboMgr->updatePerObject(modelMatrix);
			uboMgr->bindPerObject();
			UBOClipPlane clipPlane{};
			clipPlane.usesClipPlane = 0;
			uboMgr->updateClipPlane(clipPlane);
			uboMgr->bindCustom(sizeof(UBOClipPlane));
		}
		bindShadowmap(m_TerrainShader, shadowmapData);
		terrain->Draw(m_TerrainShader, m_RenderPassType);

		FPSCamera* fpscamera = dynamic_cast<FPSCamera*>(camera);
		if (fpscamera && fluid) {
			fluid->drawParticle(dynamic_cast<FPSCamera*>(camera));
		}
		skybox->Draw(camera);

		// 切换回 model shader 渲染透明物体（需要开启 blend、关闭 face cull）
		rhi::PipelineState transparentPipeline = pipeline;
		transparentPipeline.program = m_ModelShader->getProgramHandle();
		transparentPipeline.blendEnable = true;
		transparentPipeline.srcColorBlend = rhi::BlendFactor::SrcAlpha;
		transparentPipeline.dstColorBlend = rhi::BlendFactor::OneMinusSrcAlpha;
		transparentPipeline.srcAlphaBlend = rhi::BlendFactor::SrcAlpha;
		transparentPipeline.dstAlphaBlend = rhi::BlendFactor::OneMinusSrcAlpha;
		transparentPipeline.stencilEnable = false;
		transparentPipeline.cullMode = rhi::CullMode::None;
		cmd().bindPipeline(transparentPipeline);
		modelRenderer->flushTransparent(m_ModelShader, m_RenderPassType);

		// 通过命令缓冲录制 endRenderPass
		cmd().endRenderPass();

		// Render pass output
		LightingPassOutput passOutput;
		passOutput.renderTarget = m_RT->getHandle();
		passOutput.colorTexture = m_RT->getColorTexture();
		passOutput.width = m_RT->getWidth();
		passOutput.height = m_RT->getHeight();
		passOutput.isMultisampled = m_RT->isMultisampled();
		return passOutput;
	}

	void ForwardLightingPass::bindShadowmap(Shader* shader, ShadowmapPassOutput& shadowmapData) {
		shadowmapData.depthTexture->bind(0);
		shader->setUniform("dirLightShadowmap", 0);
		
		// 阴影数据通过 Lighting UBO 传递
		if (auto* uboMgr = getUBOManager()) {
			auto& lightingUBO = uboMgr->getLightingData();
			lightingUBO.dirLightShadowData.shadowBias = 0.01f;
			lightingUBO.dirLightShadowData.lightSpaceViewProjectionMatrix = shadowmapData.directionalLightViewProjMatrix;
			lightingUBO.dirLightShadowData.lightShadowIndex = 1;
			uboMgr->updateLighting();
		}
	}

}
