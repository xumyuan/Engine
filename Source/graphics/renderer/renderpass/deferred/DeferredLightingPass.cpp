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
		// 通过命令缓冲录制 beginRenderPass
		rhi::RenderPassParams passParams;
		passParams.viewport = { 0, 0, m_RT->getWidth(), m_RT->getHeight() };
		passParams.clearColorFlag = true;
		passParams.clearDepthFlag = true;
		cmd().beginRenderPass(m_RT->getHandle(), passParams);

		// 初始管线状态：关闭深度测试和多重采样
		rhi::PipelineState pipeline;
		pipeline.program = m_LightingShader->getProgramHandle();
		pipeline.depthTest = false;
		pipeline.multisample = false;
		pipeline.cullMode = rhi::CullMode::Back;
		cmd().bindPipeline(pipeline);

		// 先结束当前 render pass，再执行 blit（保证 begin/end 成对）
		cmd().endRenderPass();

		// Move the depth + stencil of the GBuffer to our render target
		cmd().blit(inputGbuffer.renderTarget, m_RT->getHandle(),
			0, 0, inputGbuffer.width, inputGbuffer.height,
			0, 0, m_RT->getWidth(), m_RT->getHeight(),
			rhi::RHIDevice::BlitDepth | rhi::RHIDevice::BlitStencil);

		// Re-bind our RT after blit (blit may change FBO binding)
		rhi::RenderPassParams rebindParams;
		rebindParams.viewport = { 0, 0, m_RT->getWidth(), m_RT->getHeight() };
		rebindParams.clearColorFlag = false;
		rebindParams.clearDepthFlag = false;
		rebindParams.clearStencilFlag = false;
		cmd().beginRenderPass(m_RT->getHandle(), rebindParams);

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
		cmd().bindPipeline(pipeline);

		// UBO 方式：PerFrame + Lighting
		rhi::ProgramHandle lightingProgram = m_LightingShader->getProgramHandle();
		if (auto* uboMgr = getUBOManager()) {
			uboMgr->preparePerFrame(camera->getViewMatrix(), camera->getProjectionMatrix(),
				camera->getPosition());
			cmd().updateBuffer(uboMgr->getPerFrameHandle(),
				&uboMgr->getPerFrameData(), sizeof(UBOPerFrame));
			cmd().bindUBO(UBOBinding::PerFrame,
				uboMgr->getPerFrameHandle(), sizeof(UBOPerFrame));

			auto& lightingUBO = uboMgr->getLightingData();
			lightCollector->fillLightingUBO(m_RenderScene.rootNode, lightingUBO);
			cmd().updateBuffer(uboMgr->getLightingHandle(),
				&uboMgr->getLightingDataConst(), sizeof(UBOLighting));
			cmd().bindUBO(UBOBinding::Lighting,
				uboMgr->getLightingHandle(), sizeof(UBOLighting));
		}

		// Bind GBuffer data
		cmd().bindTextureUnit(inputGbuffer.albedoTexture->getRHIHandle(), 6);
		cmd().setUniformInt(lightingProgram, "albedoTexture", 6);

		cmd().bindTextureUnit(inputGbuffer.normalTexture->getRHIHandle(), 7);
		cmd().setUniformInt(lightingProgram, "normalTexture", 7);

		cmd().bindTextureUnit(inputGbuffer.materialInfoTexture->getRHIHandle(), 8);
		cmd().setUniformInt(lightingProgram, "materialInfoTexture", 8);

		// Bind SSAO texture
		UBOIBLParams iblParams{};
		iblParams.reflectionProbeMipCount = REFLECTION_PROBE_MIP_COUNT;
		if (preLightingOutput.ssaoTexture != nullptr) {
			cmd().bindTextureUnit(preLightingOutput.ssaoTexture->getRHIHandle(), 9);
			cmd().setUniformInt(lightingProgram, "ssaoTexture", 9);
			iblParams.useSSAO = 1;
		}
		else {
			iblParams.useSSAO = 0;
		}

		cmd().bindTextureUnit(inputGbuffer.depthStencilTexture->getRHIHandle(), 10);
		cmd().setUniformInt(lightingProgram, "depthTexture", 10);

		// Shadowmap code
		BindShadowmap(cmd(), lightingProgram, inputShadowmapData);

		// IBL Bindings
		glm::vec3 cameraPosition = camera->getPosition();
		probeManager->bindProbe(cameraPosition, cmd(), lightingProgram);

		cmd().setUniformInt(lightingProgram, "pointLightShadowCubemap", 1);

		// Perform lighting on the terrain (turn IBL off)
		iblParams.computeIBL = 0;
		if (auto* uboMgr = getUBOManager()) {
			cmd().updateBuffer(uboMgr->getCustomHandle(), &iblParams, sizeof(UBOIBLParams));
			cmd().bindUBO(UBOBinding::CustomParams,
				uboMgr->getCustomHandle(), sizeof(UBOIBLParams));
		}
		// stencil: 匹配 terrain 值
		rhi::StencilState terrainStencil = stencilReadOnly;
		terrainStencil.ref = StencilValue::TerrainStencilValue;
		pipeline.stencilFront = terrainStencil;
		pipeline.stencilBack = terrainStencil;
		cmd().bindPipeline(pipeline);
		ModelRenderer::drawNdcPlane(cmd());

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
			cmd().updateBuffer(uboMgr->getCustomHandle(), &iblParams, sizeof(UBOIBLParams));
			cmd().bindUBO(UBOBinding::CustomParams,
				uboMgr->getCustomHandle(), sizeof(UBOIBLParams));
		}
		// stencil: 匹配 model 值
		rhi::StencilState modelStencil = stencilReadOnly;
		modelStencil.ref = StencilValue::ModelStencilValue;
		pipeline.stencilFront = modelStencil;
		pipeline.stencilBack = modelStencil;
		cmd().bindPipeline(pipeline);
		ModelRenderer::drawNdcPlane(cmd());

		// Reset state: 恢复深度测试
		pipeline.depthTest = true;
		pipeline.stencilEnable = false;
		cmd().bindPipeline(pipeline);

		cmd().pushDebugGroup("Render Skybox");
		Skybox* skybox = m_RenderScene.skybox;
		skybox->Draw(cmd(), camera);
		cmd().popDebugGroup();

		// 通过命令缓冲录制 endRenderPass
		cmd().endRenderPass();

		// Render pass output
		LightingPassOutput passOutput;
		passOutput.renderTarget = m_RT->getHandle();
		passOutput.colorTexture = m_RT->getColorTexture();
		passOutput.width = m_RT->getWidth();
		passOutput.height = m_RT->getHeight();
		passOutput.isMultisampled = false;
		return passOutput;
	}

	void DeferredLightingPass::BindShadowmap(rhi::CommandBuffer& cmdBuf, rhi::ProgramHandle program, ShadowmapPassOutput& shadowmapData)
	{
		cmdBuf.bindTextureUnit(shadowmapData.depthTexture->getRHIHandle(), 0);
		cmdBuf.setUniformInt(program, "dirLightShadowmap", 0);
		
		// 阴影数据通过 Lighting UBO 传递
		if (auto* uboMgr = getUBOManager()) {
			auto& lightingUBO = uboMgr->getLightingData();
			lightingUBO.dirLightShadowData.shadowBias = 0.01f;
			lightingUBO.dirLightShadowData.lightSpaceViewProjectionMatrix = shadowmapData.directionalLightViewProjMatrix;
			lightingUBO.dirLightShadowData.lightShadowIndex = 1;
			cmdBuf.updateBuffer(uboMgr->getLightingHandle(),
				&uboMgr->getLightingDataConst(), sizeof(UBOLighting));
		}
	}
}
