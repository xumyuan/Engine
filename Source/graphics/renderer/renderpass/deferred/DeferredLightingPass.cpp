#include "pch.h"
#include "DeferredLightingPass.h"

#include <graphics/Window.h>
#include <graphics/Shader.h>
#include <graphics/texture/Cubemap.h>
#include <graphics/camera/ICamera.h>
#include <graphics/renderer/renderpass/deferred/DeferredGeometryPass.h>
#include <graphics/UniformBufferManager.h>
#include <utils/loaders/ShaderLoader.h>

namespace engine
{
	DeferredLightingPass::DeferredLightingPass(const RenderScene& renderScene) : RenderPass(renderScene,RenderPassType::LightingPassType)
	{
		m_LightingShader = ShaderLoader::loadShader("Shaders/deferred/PBR_LightingPass.glsl");

		m_RT = new RenderTarget(Window::getWidth(), Window::getHeight());
		m_RT->addColorTexture(rhi::TextureFormat::RGBA16F)
			.addDepthStencilTexture(DepthStencilFormat::DepthStencil).build();
	}

	DeferredLightingPass::~DeferredLightingPass()
	{
		delete m_RT;
	}

	LightingPassOutput DeferredLightingPass::ExecuteLightingPass(ShadowmapPassOutput& inputShadowmapData, GeometryPassOutput& inputGbuffer, PreLightingPassOutput& preLightingOutput, ICamera* camera, bool useIBL)
	{
		m_RT->beginPass();

		// 初始管线状态：关闭深度测试和多重采样
		rhi::PipelineState pipeline;
		pipeline.program = m_LightingShader->getProgramHandle();
		pipeline.depthTest = false;
		pipeline.multisample = false;
		pipeline.cullMode = rhi::CullMode::Back;
		bindPipelineState(pipeline);

		// Move the depth + stencil of the GBuffer to our render target
		auto* device = getRHIDevice();
		device->blit(inputGbuffer.renderTarget, m_RT->getHandle(),
			0, 0, inputGbuffer.width, inputGbuffer.height,
			0, 0, m_RT->getWidth(), m_RT->getHeight(),
			rhi::RHIDevice::BlitDepth | rhi::RHIDevice::BlitStencil);

		// Re-bind our RT after blit (blit may change FBO binding)
		rhi::RenderPassParams rebindParams;
		rebindParams.viewport = { 0, 0, m_RT->getWidth(), m_RT->getHeight() };
		rebindParams.clearColorFlag = false;
		rebindParams.clearDepthFlag = false;
		rebindParams.clearStencilFlag = false;
		m_RT->beginPass(rebindParams);

		// Setup stencil state: 开启 stencil 测试，写 mask 设为 0x00（只读不写）
		rhi::StencilState stencilReadOnly;
		stencilReadOnly.func = rhi::CompareOp::Equal;
		stencilReadOnly.writeMask = 0x00;
		stencilReadOnly.readMask = 0xFF;
		stencilReadOnly.stencilFail = rhi::StencilOp::Keep;
		stencilReadOnly.depthFail = rhi::StencilOp::Keep;
		stencilReadOnly.depthPass = rhi::StencilOp::Keep;

		LightCollector* lightCollector = m_RenderScene.lightCollector;
		ProbeManager* probeManager = m_RenderScene.probeManager;

		// 重新绑定 shader pipeline（blit 后 shader 状态可能需要恢复）
		pipeline.stencilEnable = true;
		pipeline.stencilFront = stencilReadOnly;
		pipeline.stencilBack = stencilReadOnly;
		bindPipelineState(pipeline);

		// UBO 方式：PerFrame + Lighting
		if (auto* uboMgr = getUBOManager()) {
			uboMgr->updatePerFrame(camera->getViewMatrix(), camera->getProjectionMatrix(),
				camera->getPosition());
			uboMgr->bindPerFrame();

			auto& lightingUBO = uboMgr->getLightingData();
			lightCollector->fillLightingUBO(m_RenderScene.rootNode, lightingUBO);
			uboMgr->updateLighting();
			uboMgr->bindLighting();
		}

		// Bind GBuffer data
		inputGbuffer.albedoTexture->bind(6);
		m_LightingShader->setUniform("albedoTexture", 6);

		inputGbuffer.normalTexture->bind(7);
		m_LightingShader->setUniform("normalTexture", 7);

		inputGbuffer.materialInfoTexture->bind(8);
		m_LightingShader->setUniform("materialInfoTexture", 8);

		// Bind SSAO texture
		UBOIBLParams iblParams{};
		iblParams.reflectionProbeMipCount = REFLECTION_PROBE_MIP_COUNT;
		if (preLightingOutput.ssaoTexture != nullptr) {
			preLightingOutput.ssaoTexture->bind(9);
			m_LightingShader->setUniform("ssaoTexture", 9);
			iblParams.useSSAO = 1;
		}
		else {
			iblParams.useSSAO = 0;
		}

		inputGbuffer.depthStencilTexture->bind(10);
		m_LightingShader->setUniform("depthTexture", 10);

		// Shadowmap code
		BindShadowmap(m_LightingShader, inputShadowmapData);

		// IBL Bindings
		glm::vec3 cameraPosition = camera->getPosition();
		probeManager->bindProbe(cameraPosition, m_LightingShader);

		// 将未使用的 samplerCube uniform (pointLightShadowCubemap) 指向已绑定 cubemap 的纹理单元，
		// 避免其默认值 0 指向只有 GL_TEXTURE_2D 的 unit 导致 GL_INVALID_OPERATION: program texture usage
		m_LightingShader->setUniform("pointLightShadowCubemap", 1); 

		// Perform lighting on the terrain (turn IBL off)
		iblParams.computeIBL = 0;
		if (auto* uboMgr = getUBOManager()) {
			uboMgr->updateIBLParams(iblParams);
			uboMgr->bindCustom(sizeof(UBOIBLParams));
		}
		// stencil: 匹配 terrain 值
		rhi::StencilState terrainStencil = stencilReadOnly;
		terrainStencil.ref = StencilValue::TerrainStencilValue;
		pipeline.stencilFront = terrainStencil;
		pipeline.stencilBack = terrainStencil;
		bindPipelineState(pipeline);
		ModelRenderer::drawNdcPlane();

		// Perform lighting on the models in the scene
		if (useIBL)
		{
			iblParams.computeIBL = 1;
		}
		else
		{
			iblParams.computeIBL = 0;
		}
		if (auto* uboMgr = getUBOManager()) {
			uboMgr->updateIBLParams(iblParams);
			uboMgr->bindCustom(sizeof(UBOIBLParams));
		}
		// stencil: 匹配 model 值
		rhi::StencilState modelStencil = stencilReadOnly;
		modelStencil.ref = StencilValue::ModelStencilValue;
		pipeline.stencilFront = modelStencil;
		pipeline.stencilBack = modelStencil;
		bindPipelineState(pipeline);
		ModelRenderer::drawNdcPlane();

		// Reset state: 恢复深度测试
		pipeline.depthTest = true;
		pipeline.stencilEnable = false;
		bindPipelineState(pipeline);

		BEGIN_EVENT("Render Skybox");
		Skybox* skybox = m_RenderScene.skybox;
		skybox->Draw(camera);
		END_EVENT();

		m_RT->endPass();

		// Render pass output
		LightingPassOutput passOutput;
		passOutput.renderTarget = m_RT->getHandle();
		passOutput.colorTexture = m_RT->getColorTexture();
		passOutput.width = m_RT->getWidth();
		passOutput.height = m_RT->getHeight();
		passOutput.isMultisampled = false;
		return passOutput;
	}

	void DeferredLightingPass::BindShadowmap(Shader* shader, ShadowmapPassOutput& shadowmapData)
	{
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
