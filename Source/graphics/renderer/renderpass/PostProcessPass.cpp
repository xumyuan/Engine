#include "pch.h"
#include "PostProcessPass.h"

#include <utils/DebugEvent.h>
#include <utils/loaders/ShaderLoader.h>

namespace engine
{

	PostProcessPass::PostProcessPass(Scene3D* scene) : RenderPass(scene, RenderPassType::PostProcessPassType),
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
		auto* device = getRHIDevice();

		// 如果输入是多重采样的，通过 blit 到非多采样 RT 解析
		Texture* sourceColorTexture = lightingOutput.colorTexture;
		if (lightingOutput.isMultisampled) {
			device->blit(lightingOutput.renderTarget, m_ResolveRT.getHandle(),
				0, 0, lightingOutput.width, lightingOutput.height,
				0, 0, m_ResolveRT.getWidth(), m_ResolveRT.getHeight(),
				rhi::RHIDevice::BlitColor);
			sourceColorTexture = m_ResolveRT.getColorTexture();
		}

#if DEBUG_ENABLED
		if (DebugPane::getWireframeMode()) {
			if (auto* dev = engine::getDebugDevice())
				dev->setPolygonMode(rhi::PolygonMode::Fill);
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

		m_GLCache->switchShader(m_PassthroughShader);
		m_PassthroughShader->setUniform("input_texture", 0);
		currentTexture->bind(0);
		ModelRenderer::drawNdcPlane();
	}


	void PostProcessPass::gammaCorrect(RenderTarget* target, Texture* hdrTexture) {
		target->beginPass();

		m_GLCache->switchShader(m_GammaCorrectShader);
		m_GLCache->setDepthTest(false);
		m_GLCache->setBlend(false);
		m_GLCache->setFaceCull(true);
		m_GLCache->setCullFace(GL_BACK);
		m_GLCache->setStencilTest(false);

		m_GammaCorrectShader->setUniform("gamma_inverse", 1.0f / m_GammaCorrection);
		m_GammaCorrectShader->setUniform("exposure", m_Exposure);
		m_GammaCorrectShader->setUniform("screen_texture", 0);

		hdrTexture->bind(0);

		ModelRenderer::drawNdcPlane();

		target->endPass();
	}

	void PostProcessPass::fxaa(RenderTarget* target, Texture* texture) {
		target->beginPass();

		m_GLCache->switchShader(m_FxaaShader);
		m_GLCache->setDepthTest(false);
		m_GLCache->setBlend(false);
		m_GLCache->setFaceCull(true);
		m_GLCache->setCullFace(GL_BACK);
		m_GLCache->setStencilTest(false);

		m_FxaaShader->setUniform("texel_size", glm::vec2(1.0f / (float)Window::getWidth(), 1.0f / (float)Window::getHeight()));

		m_FxaaShader->setUniform("input_texture", 0);
		texture->bind(0);

		ModelRenderer::drawNdcPlane();

		target->endPass();
	}

}
