#include "pch.h"
#include "PostProcessPass.h"

#include <utils/loaders/ShaderLoader.h>

namespace engine
{

	PostProcessPass::PostProcessPass(Scene3D* scene) : RenderPass(scene, RenderPassType::PostProcessPassType),
		m_ScreenRenderTarget(Window::getWidth(), Window::getHeight()),
		m_GammaCorrectTarget(Window::getWidth(), Window::getHeight())
	{
		m_GammaCorrectShader = ShaderLoader::loadShader("src/shaders/post_process/gamma/gammaCorrect.vert", "src/shaders/post_process/gamma/gammaCorrect.frag");
		m_PassthroughShader = ShaderLoader::loadShader("src/shaders/post_process/copy.vert", "src/shaders/post_process/copy.frag");
		m_FxaaShader = ShaderLoader::loadShader("src/shaders/post_process/fxaa/fxaa.vert", "src/shaders/post_process/fxaa/fxaa.frag");

		m_ScreenRenderTarget.addTexture2DColorAttachment(false).addDepthRBO(false).createFramebuffer();

		m_GammaCorrectTarget.addTexture2DColorAttachment(false, false).addDepthRBO(false).createFramebuffer();

		DebugPane::bindGammaCorrectionValue(&m_GammaCorrection);
		DebugPane::bindExposureValue(&m_Exposure);
		DebugPane::bindFxaaEnabled(&m_FxaaEnabled);
	}

	PostProcessPass::~PostProcessPass() {}

	void PostProcessPass::executeRenderPass(Framebuffer* framebufferToProcess) {
		glViewport(0, 0, Window::getWidth(), Window::getHeight());

		// 如果输入 RenderTarget 是多重采样的。通过将其位块传送到非多重采样的 RenderTarget 来解决它，以便我们可以对其进行后期处理
		Framebuffer* target = framebufferToProcess;
		if (framebufferToProcess->isMultisampled()) {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferToProcess->getFramebuffer());
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ScreenRenderTarget.getFramebuffer());

			glBlitFramebuffer(0, 0, framebufferToProcess->getWidth(),
				framebufferToProcess->getHeight(), 0, 0, m_ScreenRenderTarget.getWidth(), m_ScreenRenderTarget.getHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

			target = &m_ScreenRenderTarget;
		}

#if DEBUG_ENABLED
		if (DebugPane::getWireframeMode())
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif


		Framebuffer* framebufferToRenderTo = target;
		// todo: 这里交换fxaa和伽马矫正的顺序会导致图像右上角出现花屏
		// 暂未找到原因

		// fxaa
		if (m_FxaaEnabled) {
			framebufferToRenderTo = target;
			fxaa(framebufferToRenderTo, target->getColorBufferTexture());
			target = framebufferToRenderTo;
		}

		// 伽马矫正
		gammaCorrect(&m_GammaCorrectTarget, target->getColorBufferTexture());
		target = &m_GammaCorrectTarget;

		Window::bind();
		Window::clear();

		m_GLCache->switchShader(m_PassthroughShader);
		m_PassthroughShader->setUniform1i("input_texture", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, target->getColorBufferTexture());

		m_ActiveScene->getModelRenderer()->NDC_Plane.Draw();
	}


	void PostProcessPass::gammaCorrect(Framebuffer* target, unsigned int hdrTexture) {
		glViewport(0, 0, target->getWidth(), target->getHeight());
		m_GLCache->switchShader(m_GammaCorrectShader);
		m_GLCache->setDepthTest(false);
		m_GLCache->setBlend(false);
		m_GLCache->setFaceCull(true);
		m_GLCache->setCullFace(GL_BACK);
		m_GLCache->setStencilTest(false);
		target->bind();

		m_GammaCorrectShader->setUniform1f("gamma_inverse", 1.0f / m_GammaCorrection);
		m_GammaCorrectShader->setUniform1f("exposure", m_Exposure);
		m_GammaCorrectShader->setUniform1i("screen_texture", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, hdrTexture);
		//hdrTexture->bind(0);

		m_ActiveScene->getModelRenderer()->NDC_Plane.Draw();

	}

	void PostProcessPass::fxaa(Framebuffer* target, unsigned int texture) {
		glViewport(0, 0, target->getWidth(), target->getHeight());
		m_GLCache->switchShader(m_FxaaShader);

		m_GLCache->setDepthTest(false);
		m_GLCache->setBlend(false);
		m_GLCache->setFaceCull(true);
		m_GLCache->setCullFace(GL_BACK);
		m_GLCache->setStencilTest(false);
		target->bind();

		m_FxaaShader->setUniform2f("texel_size", glm::vec2(1.0f / (float)Window::getWidth(), 1.0f / (float)Window::getHeight()));

		m_FxaaShader->setUniform1i("input_texture", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		m_ActiveScene->getModelRenderer()->NDC_Plane.Draw();
	}

}