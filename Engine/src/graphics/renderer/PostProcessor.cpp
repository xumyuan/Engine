#include "pch.h"
#include "PostProcessor.h"

namespace engine {
	namespace graphics {

		PostProcessor::PostProcessor(MeshRenderer* renderer)
			: m_MeshRenderer(renderer), m_PostProcessShader("src/shaders/postprocess.vert", "src/shaders/postprocess.frag"), m_ScreenRenderTarget(Window::getWidth(), Window::getHeight())
		{
			m_ScreenRenderTarget.addColorAttachment(false).addDepthStencilRBO(false).createFramebuffer();
			ui::DebugPane::bindGammaCorrectionValue(&m_GammaCorrection);
		}

		PostProcessor::~PostProcessor() {

		}

		void PostProcessor::preLightingPostProcess() {
			// SSAO goes here
		}

		void PostProcessor::postLightingPostProcess(opengl::RenderTarget* input) {
			// TODO gamma correction and tone mapping for HDR need to be added to the post process shader
#if DEBUG_ENABLED
			glFinish();
			m_Timer.reset();
#endif

			// If the input RenderTarget is multi-sampled. Resolve it by blitting it to a non-multi-sampled RenderTarget
			opengl::RenderTarget* target = input;
			if (input->isMultisampledColourBuffer()) {
				glBindFramebuffer(GL_READ_FRAMEBUFFER, input->getFramebuffer());
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ScreenRenderTarget.getFramebuffer());
				glBlitFramebuffer(0, 0, input->getWidth(), input->getHeight(), 0, 0, m_ScreenRenderTarget.getWidth(), m_ScreenRenderTarget.getHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
				target = &m_ScreenRenderTarget;
			}

#if DEBUG_ENABLED
			if (ui::DebugPane::getWireframeMode())
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

			// ����ɫ������������ã�������Ļ�ռ�����
			input->unbind();
			GLCache::getInstance()->switchShader(m_PostProcessShader.getShaderID());
			m_PostProcessShader.setUniform2f("read_offset", glm::vec2(1.0f / (float)target->getWidth(), 1.0f / (float)target->getHeight()));
			m_PostProcessShader.setUniform1f("gamma_inverse", 1.0f / m_GammaCorrection);
			m_PostProcessShader.setUniform1i("blur_enabled", m_Blur);
			m_PostProcessShader.setUniform1i("screen_texture", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, target->getColorBufferTexture());

			Window::clear();
			m_MeshRenderer->NDC_Plane.Draw();

#if DEBUG_ENABLED
			glFinish();
			ui::RuntimePane::setPostProcessTimer((float)m_Timer.elapsed());
#endif
		}

	}
}