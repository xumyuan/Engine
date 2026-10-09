#include "pch.h"
#include "PostProcessPass.h"

#include <graphics/Window.h>
#include <ui/DebugPane.h>

#include <imgui.h>
#include <utils/DebugEvent.h>
#include <utils/loaders/ShaderLoader.h>
#include <graphics/UniformBufferManager.h>
#include <graphics/UniformBufferData.h>
#include <graphics/TextureBindings.h>

namespace engine
{

	PostProcessPass::PostProcessPass(const RenderScene& renderScene) : RenderPass(renderScene, RenderPassType::PostProcessPassType),
		m_ResolveRT(Window::getWidth(), Window::getHeight()),
		m_GammaCorrectTarget(Window::getWidth(), Window::getHeight()),
		m_FullRenderTarget(Window::getWidth(), Window::getHeight())
	{
		m_GammaCorrectShader = ShaderLoader::loadShader("Source/Shaders/post_process/gammaCorrect.glsl");
		m_PassthroughShader = ShaderLoader::loadShader("Source/Shaders/post_process/copy.glsl");
		m_FxaaShader = ShaderLoader::loadShader("Source/Shaders/post_process/fxaa.glsl");

		m_GammaCorrectTarget.addColorTexture(rhi::TextureFormat::RGBA8)
			.addDepthStencilTexture(DepthStencilFormat::DepthOnly, false).build();
		m_ResolveRT.addColorTexture(rhi::TextureFormat::RGBA16F)
			.addDepthStencilTexture(DepthStencilFormat::DepthOnly, false).build();
		m_FullRenderTarget.addColorTexture(rhi::TextureFormat::RGBA16F).build();

		m_DebugSection = DebugPane::addSection("Post Process", [this]() {
			ImGui::Checkbox("FXAA", &m_FxaaEnabled);
			ImGui::SliderFloat("Gamma", &m_GammaCorrection, 0.5f, 3.0f, "%.2f");
			ImGui::SliderFloat("Exposure", &m_Exposure, 0.1f, 5.0f, "%.2f");
		}, 40, true);
	}

	PostProcessPass::~PostProcessPass()
	{
		DebugPane::removeSection(m_DebugSection);
	}

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
		cmd().bindDefaultFramebuffer(Window::getWidth(), Window::getHeight());
		cmd().clear(0x07);

		rhi::PipelineState pipeline;
		pipeline.program = m_PassthroughShader->getProgramHandle();
		pipeline.depthTest = false;
		pipeline.blendEnable = false;
		pipeline.stencilEnable = false;
		pipeline.cullMode = rhi::CullMode::Back;
		cmd().bindPipeline(pipeline);

		cmd().bindTextureUnit(currentTexture->getRHIHandle(), TextureUnit::PostProcessInput);
		ModelRenderer::drawNdcPlane(cmd());
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
			cmd().updateBuffer(uboMgr->getCustomHandle(), &ppParams, sizeof(UBOPostProcessParams));
			cmd().bindUBO(UBOBinding::CustomParams, uboMgr->getCustomHandle(), sizeof(UBOPostProcessParams));
		}

		cmd().bindTextureUnit(hdrTexture->getRHIHandle(), TextureUnit::PostProcessInput);

		ModelRenderer::drawNdcPlane(cmd());

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

		// fxaa.glsl 从 PostProcessParams（binding 4）读取 texel_size。
		// gamma 刚把这份 UBO 的 texel_size 写成 0，这里按目标分辨率覆写。
		if (auto* uboMgr = getUBOManager()) {
			UBOPostProcessParams ppParams{};
			ppParams.gamma_inverse = 1.0f / m_GammaCorrection;
			ppParams.exposure = m_Exposure;
			const float width = static_cast<float>(target->getWidth());
			const float height = static_cast<float>(target->getHeight());
			if (width > 0.0f && height > 0.0f)
				ppParams.texel_size = glm::vec2(1.0f / width, 1.0f / height);
			cmd().updateBuffer(uboMgr->getCustomHandle(), &ppParams, sizeof(UBOPostProcessParams));
			cmd().bindUBO(UBOBinding::CustomParams,
				uboMgr->getCustomHandle(), sizeof(UBOPostProcessParams));
		}

		cmd().bindTextureUnit(texture->getRHIHandle(), TextureUnit::PostProcessInput);

		ModelRenderer::drawNdcPlane(cmd());

		// 通过命令缓冲录制 endRenderPass
		cmd().endRenderPass();
	}

}
