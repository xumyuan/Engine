#include "pch.h"
#include "PostProcessPass.h"

#include <utils/loaders/ShaderLoader.h>

namespace engine
{

	PostProcessPass::PostProcessPass(Scene3D* scene) : RenderPass(scene, RenderPassType::PostProcessPassType), m_ScreenRenderTarget(Window::getWidth(), Window::getHeight())
	{
		m_PostProcessShader = ShaderLoader::loadShader("src/shaders/postprocess.vert", "src/shaders/postprocess.frag");

		m_ScreenRenderTarget.addTexture2DColorAttachment(false).addDepthStencilRBO(false).createFramebuffer();
		DebugPane::bindGammaCorrectionValue(&m_GammaCorrection);
	}

	PostProcessPass::~PostProcessPass() {}

	void PostProcessPass::executeRenderPass(Framebuffer* framebufferToProcess) {
		glViewport(0, 0, Window::getWidth(), Window::getHeight());

		// 如果输入 RenderTarget 是多重采样的。通过将其位块传送到非多重采样的 RenderTarget 来解决它，以便我们可以对其进行后期处理
		Framebuffer* target = framebufferToProcess;
		if (framebufferToProcess->isMultisampledColourBuffer()) {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferToProcess->getFramebuffer());
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ScreenRenderTarget.getFramebuffer());
			glBlitFramebuffer(0, 0, framebufferToProcess->getWidth(), framebufferToProcess->getHeight(), 0, 0, m_ScreenRenderTarget.getWidth(), m_ScreenRenderTarget.getHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
			target = &m_ScreenRenderTarget;
		}

#if DEBUG_ENABLED
		if (DebugPane::getWireframeMode())
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

		// 绑定着色器及其后处理设置，并绑定屏幕空间纹理
		target->unbind();
		GLCache::getInstance()->switchShader(m_PostProcessShader);
		m_PostProcessShader->setUniform1f("gamma_inverse", 1.0f / m_GammaCorrection);
		m_PostProcessShader->setUniform2f("read_offset", glm::vec2(1.0f / (float)target->getWidth(), 1.0f / (float)target->getHeight()));
		m_PostProcessShader->setUniform1i("blur_enabled", m_Blur);
		m_PostProcessShader->setUniform1i("screen_texture", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, target->getColorBufferTexture());

		Window::clear();
		ModelRenderer* modelRenderer = m_ActiveScene->getModelRenderer();
		modelRenderer->NDC_Plane.Draw();
	}

}