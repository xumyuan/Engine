#include "pch.h"
#include "FluidRenderPass.h"

#include <graphics/Window.h>
#include <graphics/Skybox.h>
#include <graphics/TextureBindings.h>
#include <graphics/UniformBufferManager.h>
#include <graphics/renderer/ModelRenderer.h>
#include <physics/fluid/FluidSim.h>
#include <utils/loaders/ShaderLoader.h>

namespace engine {

	FluidRenderPass::FluidRenderPass(const RenderScene& renderScene)
		: RenderPass(renderScene, RenderPassType::FluidPassType),
		m_DepthRT(Window::getWidth(), Window::getHeight()),
		m_ThicknessRT(Window::getWidth(), Window::getHeight()),
		m_SmoothRTA(Window::getWidth(), Window::getHeight()),
		m_SmoothRTB(Window::getWidth(), Window::getHeight()),
		m_CompositeRT(Window::getWidth(), Window::getHeight())
	{
		m_DepthShader = ShaderLoader::loadShader("Shaders/fluid/ssf_depth.glsl");
		m_ThicknessShader = ShaderLoader::loadShader("Shaders/fluid/ssf_thickness.glsl");
		m_SmoothShader = ShaderLoader::loadShader("Shaders/fluid/ssf_smooth.glsl");
		m_CompositeShader = ShaderLoader::loadShader("Shaders/fluid/ssf_composite.glsl");

		// 深度附件只用于保留最近的粒子表面，不需要采样
		m_DepthRT.addColorTexture(rhi::TextureFormat::R32F)
			.addDepthStencilTexture(DepthStencilFormat::DepthOnly, false).build();
		m_ThicknessRT.addColorTexture(rhi::TextureFormat::R16F).build();
		m_SmoothRTA.addColorTexture(rhi::TextureFormat::R32F).build();
		m_SmoothRTB.addColorTexture(rhi::TextureFormat::R32F).build();
		m_CompositeRT.addColorTexture(rhi::TextureFormat::RGBA16F).build();
	}

	FluidRenderPass::~FluidRenderPass() {}

	LightingPassOutput FluidRenderPass::executeRenderPass(const LightingPassOutput& sceneInput,
		const GeometryPassOutput& gbuffer, ICamera* camera)
	{
		FluidSim* fluid = m_RenderScene.fluid;
		auto* uboMgr = getUBOManager();
		// 各 pass 都以 texelFetch 按像素对齐读取，要求输入与本 pass 分辨率一致且非多重采样
		if (!fluid || fluid->getParticleNum() == 0 || !uboMgr || !gbuffer.depthStencilTexture
			|| sceneInput.isMultisampled
			|| sceneInput.width != m_CompositeRT.getWidth() || sceneInput.height != m_CompositeRT.getHeight()) {
			return sceneInput;
		}

		fluid->uploadLatestPositions();

		cmd().pushDebugGroup("Screen Space Fluid");

		// screenSize 用于 shader 中的像素换算（点精灵尺寸、滤波半径）
		uboMgr->preparePerFrame(camera->getViewMatrix(), camera->getProjectionMatrix(), camera->getPosition(),
			glm::vec2(m_CompositeRT.getWidth(), m_CompositeRT.getHeight()));
		cmd().updateBuffer(uboMgr->getPerFrameHandle(), &uboMgr->getPerFrameData(), sizeof(UBOPerFrame));
		cmd().bindUBO(UBOBinding::PerFrame, uboMgr->getPerFrameHandle(), sizeof(UBOPerFrame));

		renderDepth(gbuffer.depthStencilTexture);
		renderThickness(gbuffer.depthStencilTexture);
		Texture* fluidDepth = smoothDepth();
		composite(sceneInput, fluidDepth);

		cmd().popDebugGroup();

		LightingPassOutput output;
		output.renderTarget = m_CompositeRT.getHandle();
		output.colorTexture = m_CompositeRT.getColorTexture();
		output.width = m_CompositeRT.getWidth();
		output.height = m_CompositeRT.getHeight();
		output.isMultisampled = false;
		return output;
	}

	void FluidRenderPass::renderDepth(Texture* sceneDepth) {
		cmd().pushDebugGroup("SSF Depth");

		rhi::RenderPassParams params;
		params.viewport = { 0, 0, m_DepthRT.getWidth(), m_DepthRT.getHeight() };
		params.clearColor[0] = params.clearColor[1] = params.clearColor[2] = params.clearColor[3] = 0.0f;
		params.clearColorFlag = true;
		params.clearDepthFlag = true;
		params.clearStencilFlag = false;
		cmd().beginRenderPass(m_DepthRT.getHandle(), params);

		rhi::PipelineState pipeline;
		pipeline.program = m_DepthShader->getProgramHandle();
		pipeline.primitiveType = rhi::PrimitiveType::Points;
		pipeline.depthTest = true;
		pipeline.depthWrite = true;
		pipeline.cullMode = rhi::CullMode::None;
		cmd().bindPipeline(pipeline);

		uploadParams(glm::ivec2(0));
		cmd().bindTextureUnit(sceneDepth->getRHIHandle(), TextureUnit::SSFSceneDepth);
		m_RenderScene.fluid->drawPoints(cmd());

		cmd().endRenderPass();
		cmd().popDebugGroup();
	}

	void FluidRenderPass::renderThickness(Texture* sceneDepth) {
		cmd().pushDebugGroup("SSF Thickness");

		rhi::RenderPassParams params;
		params.viewport = { 0, 0, m_ThicknessRT.getWidth(), m_ThicknessRT.getHeight() };
		params.clearColor[0] = params.clearColor[1] = params.clearColor[2] = params.clearColor[3] = 0.0f;
		params.clearColorFlag = true;
		params.clearDepthFlag = false;
		params.clearStencilFlag = false;
		cmd().beginRenderPass(m_ThicknessRT.getHandle(), params);

		// 所有粒子的弦长累加，不做深度测试
		rhi::PipelineState pipeline;
		pipeline.program = m_ThicknessShader->getProgramHandle();
		pipeline.primitiveType = rhi::PrimitiveType::Points;
		pipeline.depthTest = false;
		pipeline.depthWrite = false;
		pipeline.cullMode = rhi::CullMode::None;
		pipeline.blendEnable = true;
		pipeline.srcColorBlend = rhi::BlendFactor::One;
		pipeline.dstColorBlend = rhi::BlendFactor::One;
		pipeline.srcAlphaBlend = rhi::BlendFactor::One;
		pipeline.dstAlphaBlend = rhi::BlendFactor::One;
		cmd().bindPipeline(pipeline);

		cmd().bindTextureUnit(sceneDepth->getRHIHandle(), TextureUnit::SSFSceneDepth);
		m_RenderScene.fluid->drawPoints(cmd());

		cmd().endRenderPass();
		cmd().popDebugGroup();
	}

	Texture* FluidRenderPass::smoothDepth() {
		cmd().pushDebugGroup("SSF Smooth");

		rhi::PipelineState pipeline;
		pipeline.program = m_SmoothShader->getProgramHandle();
		pipeline.depthTest = false;
		pipeline.depthWrite = false;
		pipeline.cullMode = rhi::CullMode::None;

		Texture* input = m_DepthRT.getColorTexture();
		RenderTarget* targets[2] = { &m_SmoothRTA, &m_SmoothRTB };
		int next = 0;

		const glm::ivec2 directions[2] = { glm::ivec2(1, 0), glm::ivec2(0, 1) };
		for (int iteration = 0; iteration < m_Settings.smoothIterations; ++iteration) {
			for (const glm::ivec2& direction : directions) {
				RenderTarget* target = targets[next];

				// 每个像素都会被覆盖写入，无需清除
				rhi::RenderPassParams params;
				params.viewport = { 0, 0, target->getWidth(), target->getHeight() };
				params.clearColorFlag = false;
				params.clearDepthFlag = false;
				params.clearStencilFlag = false;
				cmd().beginRenderPass(target->getHandle(), params);
				cmd().bindPipeline(pipeline);

				uploadParams(direction);
				cmd().bindTextureUnit(input->getRHIHandle(), TextureUnit::SSFSmoothInput);
				ModelRenderer::drawNdcPlane(cmd());

				cmd().endRenderPass();
				input = target->getColorTexture();
				next ^= 1;
			}
		}

		cmd().popDebugGroup();
		return input;
	}

	void FluidRenderPass::composite(const LightingPassOutput& sceneInput, Texture* fluidDepth) {
		cmd().pushDebugGroup("SSF Composite");

		rhi::RenderPassParams params;
		params.viewport = { 0, 0, m_CompositeRT.getWidth(), m_CompositeRT.getHeight() };
		params.clearColorFlag = false;
		params.clearDepthFlag = false;
		params.clearStencilFlag = false;
		cmd().beginRenderPass(m_CompositeRT.getHandle(), params);

		rhi::PipelineState pipeline;
		pipeline.program = m_CompositeShader->getProgramHandle();
		pipeline.depthTest = false;
		pipeline.depthWrite = false;
		pipeline.cullMode = rhi::CullMode::None;
		cmd().bindPipeline(pipeline);

		// Lighting UBO（binding 2）沿用光照 pass 的绑定，取第一盏方向光做高光
		uploadParams(glm::ivec2(0));
		cmd().bindTextureUnit(sceneInput.colorTexture->getRHIHandle(), TextureUnit::SSFSceneColor);
		cmd().bindTextureUnit(fluidDepth->getRHIHandle(), TextureUnit::SSFFluidDepth);
		cmd().bindTextureUnit(m_ThicknessRT.getColorTexture()->getRHIHandle(), TextureUnit::SSFThickness);
		if (Skybox* skybox = m_RenderScene.skybox) {
			cmd().bindTextureUnit(skybox->getSkyboxCubemap()->getRHIHandle(), TextureUnit::SSFEnvironment);
		}
		ModelRenderer::drawNdcPlane(cmd());

		cmd().endRenderPass();
		cmd().popDebugGroup();
	}

	void FluidRenderPass::uploadParams(glm::ivec2 blurDirection) {
		auto* uboMgr = getUBOManager();

		UBOSSFParams params{};
		params.particle = glm::vec4(m_RenderScene.fluid->getParticleRadius(), m_Settings.thicknessScale, 0.0f, 0.0f);
		params.blur = glm::vec4(glm::vec2(blurDirection), m_Settings.blurWorldRadius, m_Settings.blurDepthFalloff);
		params.absorption = glm::vec4(m_Settings.absorption, m_Settings.refractionStrength);
		params.shading = glm::vec4(m_Settings.fresnelF0, m_Settings.specularPower, m_Settings.maxBlurRadiusPx, 0.0f);

		cmd().updateBuffer(uboMgr->getCustomHandle(), &params, sizeof(UBOSSFParams));
		cmd().bindUBO(UBOBinding::CustomParams, uboMgr->getCustomHandle(), sizeof(UBOSSFParams));
	}

}
