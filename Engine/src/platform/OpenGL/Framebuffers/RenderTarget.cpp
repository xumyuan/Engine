#include "RenderTarget.h"

namespace engine {
	namespace opengl {

		RenderTarget::RenderTarget(unsigned int width, unsigned int height)
			: m_Width(width), m_Height(height), m_FBO(0), m_ColorTexture(nullptr), m_DepthStencilRBO(0), m_DepthTexture(0)
		{
			glGenFramebuffers(1, &m_FBO);
		}

		RenderTarget::~RenderTarget() {
			glDeleteFramebuffers(1, &m_FBO);
		}

		void RenderTarget::createFramebuffer() {
			bind();
			if (m_ColorTexture == nullptr) {
				// Indicate that there won't be a colour buffer for this FBO
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			}

			// Check if the creation failed
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				utils::Logger::getInstance().error("logged_files/error.txt", "Framebuffer initialization", "Could not initialize the framebuffer");
				return;
			}
			unbind();
		}

		RenderTarget& RenderTarget::addColorAttachment(bool multisampledBuffer) {
			m_IsMultisampledColourBuffer = multisampledBuffer;

			bind();
			m_ColorTexture = new graphics::Texture();

			// Generate colour texture attachment
			if (multisampledBuffer) {
				m_ColorTexture->generate2DMultisampleTexture(m_Width, m_Height, GL_RGBA16F, MSAA_SAMPLE_AMOUNT);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_ColorTexture->getTextureId(), 0);
			}
			else {
				m_ColorTexture->setTextureMinFilter(GL_LINEAR);
				m_ColorTexture->setTextureMagFilter(GL_LINEAR);
				m_ColorTexture->setTextureWrapS(GL_CLAMP_TO_EDGE); // Both need to clamp to edge or you might see strange colours around the
				m_ColorTexture->setTextureWrapT(GL_CLAMP_TO_EDGE); // border due to interpolation and how it works with GL_REPEAT
				m_ColorTexture->generate2DTexture(m_Width, m_Height, GL_RGBA16F, GL_RGB, nullptr);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorTexture->getTextureId(), 0);
			}

			unbind();
			return *this;
		}

		RenderTarget& RenderTarget::addDepthStencilRBO(bool multisampledBuffer) {
			bind();

			// Generate depth+stencil rbo attachment
			glGenRenderbuffers(1, &m_DepthStencilRBO);
			glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilRBO);
			if (multisampledBuffer)
				glRenderbufferStorageMultisample(GL_RENDERBUFFER, MSAA_SAMPLE_AMOUNT, GL_DEPTH24_STENCIL8, m_Width, m_Height);
			else
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);

			// Attach depth+stencil attachment
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthStencilRBO);

			unbind();
			return *this;
		}

		RenderTarget& RenderTarget::addDepthAttachment(bool multisampled) {
			bind();

			// Generate depth attachment
			if (multisampled) {

			}
			else {

			}
			glGenTextures(1, &m_DepthTexture);
			glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			float borderColour[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);
			glBindTexture(GL_TEXTURE_2D, 0);

			// Attach depthmap
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);

			unbind();
			return *this;
		}

		void RenderTarget::bind() {
			glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
		}

		void RenderTarget::unbind() {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void RenderTarget::clear() {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}

	}
}