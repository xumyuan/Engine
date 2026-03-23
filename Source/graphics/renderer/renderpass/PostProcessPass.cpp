#include "pch.h"
#include "PostProcessPass.h"

#include <graphics/Window.h>
#include <ui/DebugPane.h>
#include <utils/DebugEvent.h>
#include <utils/loaders/ShaderLoader.h>
#include <graphics/UniformBufferManager.h>

namespace engine
{

	PostProcessPass::PostProcessPass(const RenderScene& renderScene) : RenderPass(renderScene, RenderPassType::PostProcessPassType),
		m_ResolveRT(Window::getWidth(), Window::getHeight()),
		m_GammaCorrectTarget(Window::getWidth(), Window::getHeight()),
		m_FullRenderTarget(Window::getWidth(), Window::getHeight())
	{
		m_GammaCorrectShader = ShaderLoader::loadShader("Shaders/post_process/gammaCorrect.glsl");
		m_PassthroughShader = ShaderLoader::loadShader("Shaders/post_process/copy.glsl");
		m_FxaaShader = ShaderLoader::loadShader("Shaders/post_process/fxaa.glsl");

		m_GammaCorrectTarget.addColorTexture(rhi::TextureFormat::RGBA8)
			.addDepthStencilTexture(DepthStencilFormat::DepthOnly, false).build();
		m_ResolveRT.addColorTexture(rhi::TextureFormat::RGBA16F)
			.addDepthStencilTexture(DepthStencilFormat::DepthOnly, false).build();
		m_FullRenderTarget.addColorTexture(rhi::TextureFormat::RGBA16F).build();

		DebugPane::bindGammaCorrectionValue(&m_GammaCorrection);
		DebugPane::bindExposureValue(&m_Exposure);
		DebugPane::bindFxaaEnabled(&m_FxaaEnabled);
	}

	PostProcessPass::~PostProcessPass() {}

	void PostProcessPass::executeRenderPass(LightingPassOutput& lightingOutput) {
		// 如果输入是多重采样的，通过 blit 解析
		Texture* sourceColorTexture = lightingOutput.colorTexture;
		if (lightingOutput.isMultisampled) {
			cmd().blit(lightingOutput.renderTarget, m_ResolveRT.getHandle(),
				0, 0, lightingOutput.width, lightingOutput.height,
				0, 0, m_ResolveRT.getWidth(), m_ResolveRT.getHeight(),
				rhi::RHIDevice::BlitColor);
			sourceColorTexture = m_ResolveRT.getColorTexture();
		}

#if DEBUG_ENABLED
		if (DebugPane::getWireframeMode()) {
			cmd().setPolygonMode(rhi::PolygonMode::Fill);
		}
#endif

		// 伽马矫正
		gammaCorrect(&m_GammaCorrectTarget, sourceColorTexture);
		Texture* currentTexture = m_GammaCorrectTarget.getColorTexture();

		// fxaa
		if (m_FxaaEnabled) {
			fxaa(&m_FullRenderTarget, currentTexture);
			currentTexture = m_FullRenderTarget.getColorTexture();
		}

		// 输出到默认帧缓冲（屏幕）
		Window::bind();
		Window::clear();

		rhi::PipelineState pipeline;
		pipeline.program = m_PassthroughShader->getProgramHandle();
		pipeline.depthTest = false;
		pipeline.blendEnable = false;
		pipeline.stencilEnable = false;
		pipeline.cullMode = rhi::CullMode::Back;
		cmd().bindPipeline(pipeline);

		m_PassthroughShader->setUniform("input_texture", 0);
		currentTexture->bind(0);
		ModelRenderer::drawNdcPlane();
	}


	void PostProcessPass::gammaCorrect(RenderTarget* target, Texture* hdrTexture) {
		// 通过命令缓冲录制 beginRenderPass
		rhi::RenderPassParams params;
		params.viewport = { 0, 0, target->getWidth(), target->getHeight() };
		params.clearColorFlag = true;
		params.clearDepthFlag = true;
		cmd().beginRenderPass(target->getHandle(), params);

		rhi::PipelineState pipeline;
		pipeline.program = m_GammaCorrectShader->getProgramHandle();
		pipeline.depthTest = false;
		pipeline.blendEnable = false;
		pipeline.stencilEnable = false;
		pipeline.cullMode = rhi::CullMode::Back;
		cmd().bindPipeline(pipeline);

		// PostProcess 参数通过 Custom UBO (binding 4) 传递
		if (auto* uboMgr = getUBOManager()) {
			UBOPostProcessParams ppParams{};
			ppParams.gamma_inverse = 1.0f / m_GammaCorrection;
			ppParams.exposure = m_Exposure;
			ppParams.texel_size = glm::vec2(0.0f); // gammaCorrect 不需要 texel_size
			uboMgr->updatePostProcessParams(ppParams);
			uboMgr->bindCustom(sizeof(UBOPostProcessParams));
		}

		m_GammaCorrectShader->setUniform("screen_texture", 0);
		hdrTexture->bind(0);

		ModelRenderer::drawNdcPlane();

		// 通过命令缓冲录制 endRenderPass
		cmd().endRenderPass();
	}

	void PostProcessPass::fxaa(RenderTarget* target, Texture* texture) {
		// 通过命令缓冲录制 beginRenderPass
		rhi::RenderPassParams params;
		params.viewport = { 0, 0, target->getWidth(), target->getHeight() };
		params.clearColorFlag = true;
		params.clearDepthFlag = true;
		cmd().beginRenderPass(target->getHandle(), params);

		rhi::PipelineState pipeline;
		pipeline.program = m_FxaaShader->getProgramHandle();
		pipeline.depthTest = false;
		pipeline.blendEnable = false;
		pipeline.stencilEnable = false;
		pipeline.cullMode = rhi::CullMode::Back;
		cmd().bindPipeline(pipeline);

		// FXAA 使用 PerFrame UBO 的 texelSize
		if (auto* uboMgr = getUBOManager()) {
			uboMgr->updatePerFrame(glm::mat4(1.0f), glm::mat4(1.0f), glm::vec3(0.0f),
				glm::vec2(Window::getWidth(), Window::getHeight()));
			uboMgr->bindPerFrame();
		}

		m_FxaaShader->setUniform("input_texture", 0);
		texture->bind(0);

		ModelRenderer::drawNdcPlane();

		// 通过命令缓冲录制 endRenderPass
		cmd().endRenderPass();
	}

}
