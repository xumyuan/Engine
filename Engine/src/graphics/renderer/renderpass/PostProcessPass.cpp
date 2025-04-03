#include "pch.h"
#include "PostProcessPass.h"

#include <utils/loaders/ShaderLoader.h>

namespace engine
{

	PostProcessPass::PostProcessPass(Scene3D* scene) : RenderPass(scene, RenderPassType::PostProcessPassType),
		m_ScreenRenderTarget(Window::getWidth(), Window::getHeight(), false),
		m_GammaCorrectTarget(Window::getWidth(), Window::getHeight(), false),
		m_FullRenderTarget(Window::getWidth(), Window::getHeight(), false)
	{
		m_GammaCorrectShader = ShaderLoader::loadShader("src/shaders/post_process/gammaCorrect.glsl");
		m_PassthroughShader = ShaderLoader::loadShader("src/shaders/post_process/copy.glsl");
		m_FxaaShader = ShaderLoader::loadShader("src/shaders/post_process/fxaa.glsl");


		m_GammaCorrectTarget.addColorTexture(Normalized8).addDepthStencilRBO(NormalizedDepthOnly).createFramebuffer();
		m_ScreenRenderTarget.addColorTexture(FloatingPoint16).addDepthStencilRBO(NormalizedDepthOnly).createFramebuffer();
		m_FullRenderTarget.addColorTexture(FloatingPoint16).createFramebuffer();


		DebugPane::bindGammaCorrectionValue(&m_GammaCorrection);
		DebugPane::bindExposureValue(&m_Exposure);
		DebugPane::bindFxaaEnabled(&m_FxaaEnabled);
	}

	PostProcessPass::~PostProcessPass() {}

	void PostProcessPass::executeRenderPass(Framebuffer* framebufferToProcess) {
		glViewport(0, 0, Window::getWidth(), Window::getHeight());

		// 如果输入 RenderTarget 是多重采样的。通过将其位块传送到非多重采样的 RenderTarget 来解决它，以便我们可以对其进行后期处理
		Framebuffer* supersampledTarget = framebufferToProcess;
		if (framebufferToProcess->isMultisampled()) {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferToProcess->getFramebuffer());
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ScreenRenderTarget.getFramebuffer());

			glBlitFramebuffer(0, 0, framebufferToProcess->getWidth(),
				framebufferToProcess->getHeight(), 0, 0, m_ScreenRenderTarget.getWidth(), m_ScreenRenderTarget.getHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

			supersampledTarget = &m_ScreenRenderTarget;
		}
		Framebuffer* target = supersampledTarget;

#if DEBUG_ENABLED
		if (DebugPane::getWireframeMode())
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif


		Framebuffer* framebufferToRenderTo = nullptr;
		// todo: 这里交换fxaa和伽马矫正的顺序会导致图像右上角出现花屏
		// 暂未找到原因
		// 3.26 解决的方法是fullrendertarget的纹理格式为FloatingPoint16

		// 伽马矫正
		gammaCorrect(&m_GammaCorrectTarget, target->getColorBufferTexture());
		target = &m_GammaCorrectTarget;

		// fxaa
		if (m_FxaaEnabled) {
			framebufferToRenderTo = &m_FullRenderTarget;
			fxaa(framebufferToRenderTo, target->getColorBufferTexture());
			target = framebufferToRenderTo;
		}


		Window::bind();
		Window::clear();

		m_GLCache->switchShader(m_PassthroughShader);
		m_PassthroughShader->setUniform("input_texture", 0);
		target->getColorBufferTexture()->bind(0);
		m_ActiveScene->getModelRenderer()->NDC_Plane.Draw();
	}


	void PostProcessPass::gammaCorrect(Framebuffer* target, Texture* hdrTexture) {
		glViewport(0, 0, target->getWidth(), target->getHeight());
		m_GLCache->switchShader(m_GammaCorrectShader);
		m_GLCache->setDepthTest(false);
		m_GLCache->setBlend(false);
		m_GLCache->setFaceCull(true);
		m_GLCache->setCullFace(GL_BACK);
		m_GLCache->setStencilTest(false);
		target->bind();

		m_GammaCorrectShader->setUniform("gamma_inverse", 1.0f / m_GammaCorrection);
		m_GammaCorrectShader->setUniform("exposure", m_Exposure);
		m_GammaCorrectShader->setUniform("screen_texture", 0);

		hdrTexture->bind(0);

		m_ActiveScene->getModelRenderer()->NDC_Plane.Draw();

	}

	void PostProcessPass::fxaa(Framebuffer* target, Texture* texture) {
		glViewport(0, 0, target->getWidth(), target->getHeight());
		m_GLCache->switchShader(m_FxaaShader);

		m_GLCache->setDepthTest(false);
		m_GLCache->setBlend(false);
		m_GLCache->setFaceCull(true);
		m_GLCache->setCullFace(GL_BACK);
		m_GLCache->setStencilTest(false);
		target->bind();

		m_FxaaShader->setUniform("texel_size", glm::vec2(1.0f / (float)Window::getWidth(), 1.0f / (float)Window::getHeight()));

		m_FxaaShader->setUniform("input_texture", 0);
		texture->bind(0);

		m_ActiveScene->getModelRenderer()->NDC_Plane.Draw();
	}

}