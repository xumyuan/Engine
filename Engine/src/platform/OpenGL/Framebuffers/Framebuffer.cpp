#include "pch.h"
#include "Framebuffer.h"

namespace engine {
	Framebuffer::Framebuffer(unsigned int width, unsigned int height, bool isMultisampled)
		:m_FBO(0), m_Width(width), m_Height(height),
		m_IsMultisampled(isMultisampled),
		m_ColorTexture(), m_DepthStencilTexture(), m_DepthStencilRBO(0)
	{
		glGenFramebuffers(1, &m_FBO);
	}

	Framebuffer::~Framebuffer() {
		glDeleteRenderbuffers(1, &m_DepthStencilRBO);

		glDeleteFramebuffers(1, &m_FBO);
	}

	void Framebuffer::createFramebuffer() {
		bind();
		if (!m_ColorTexture.isGenerated()) {
			// 该帧缓冲区没有颜色附件
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		}

		// 检查帧缓冲区是否完整
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			//获取当前的时间
			time_t now = time(0);
			//转换为时分秒，string类型
			std::string dt = ctime(&now);

			spdlog::error("[{0}]:Could not initialize the frameBuffer", dt);
			return;
		}
		unbind();
	}

	Framebuffer& Framebuffer::addColorTexture(ColorAttachmentFormat textureFormat) {
#if DEBUG_ENABLED
		if (m_ColorTexture.isGenerated()) {
			spdlog::error("Framebuffer already has a color attachment");
			return *this;
		}
#endif

		bind();

		TextureSettings colorTextureSettings;
		colorTextureSettings.TextureFormat = textureFormat;
		colorTextureSettings.TextureWrapSMode = GL_CLAMP_TO_EDGE;
		colorTextureSettings.TextureWrapTMode = GL_CLAMP_TO_EDGE;
		colorTextureSettings.TextureMinificationFilterMode = GL_LINEAR;
		colorTextureSettings.TextureMagnificationFilterMode = GL_LINEAR;
		colorTextureSettings.TextureAnisotropyLevel = 1.0f;
		colorTextureSettings.HasMips = false;
		m_ColorTexture.setTextureSettings(colorTextureSettings);


		// 生成颜色纹理附件
		if (m_IsMultisampled) {
			m_ColorTexture.generate2DMultisampleTexture(m_Width, m_Height);
			setColorAttachment(m_ColorTexture.getTextureId(), GL_TEXTURE_2D_MULTISAMPLE);
		}
		else {
			m_ColorTexture.generate2DTexture(m_Width, m_Height, GL_RGB);
			setColorAttachment(m_ColorTexture.getTextureId(), GL_TEXTURE_2D);
		}

		unbind();
		return *this;
	}

	Framebuffer& Framebuffer::addDepthStencilTexture(DepthStencilAttachmentFormat textureFormat) {
#if DEBUG_ENABLED
		if (m_DepthStencilTexture.isGenerated()) {
			spdlog::error("Framebuffer already has a depth attachment");

			return *this;
		}
#endif

		GLenum attachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
		if (textureFormat == NormalizedDepthOnly) {
			attachmentType = GL_DEPTH_ATTACHMENT;
		}

		bind();

		TextureSettings depthStencilSettings;
		depthStencilSettings.TextureFormat = textureFormat;
		depthStencilSettings.TextureWrapSMode = GL_CLAMP_TO_BORDER;
		depthStencilSettings.TextureWrapTMode = GL_CLAMP_TO_BORDER;
		depthStencilSettings.TextureMinificationFilterMode = GL_NEAREST;
		depthStencilSettings.TextureMagnificationFilterMode = GL_NEAREST;
		depthStencilSettings.TextureAnisotropyLevel = 1.0f;
		depthStencilSettings.HasBorder = true;
		depthStencilSettings.BorderColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		depthStencilSettings.HasMips = false;
		m_DepthStencilTexture.setTextureSettings(depthStencilSettings);

		// 生成深度附件
		if (m_IsMultisampled) {
			m_DepthStencilTexture.generate2DMultisampleTexture(m_Width, m_Height);
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D_MULTISAMPLE, m_DepthStencilTexture.getTextureId(), 0);
		}
		else {
			m_DepthStencilTexture.generate2DTexture(m_Width, m_Height, GL_DEPTH_COMPONENT);
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, m_DepthStencilTexture.getTextureId(), 0);
		}

		unbind();
		return *this;
	}

	Framebuffer& Framebuffer::addDepthStencilRBO(DepthStencilAttachmentFormat textureFormat) {

#if DEBUG_ENABLED
		if (m_DepthStencilRBO != 0) {
			spdlog::error("Framebuffer already has a depth+stencil RBO attachment");
			return *this;
		}
#endif

		bind();

		GLenum attachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
		if (textureFormat == NormalizedDepthOnly) {
			attachmentType = GL_DEPTH_ATTACHMENT;
		}

		// 生成深度+模板 RBO 附件
		glGenRenderbuffers(1, &m_DepthStencilRBO);
		glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilRBO);

		if (m_IsMultisampled) {
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, MSAA_SAMPLE_AMOUNT, textureFormat, m_Width, m_Height);
		}
		else {
			glRenderbufferStorage(GL_RENDERBUFFER, textureFormat, m_Width, m_Height);
		}

		// 附加深度+模板附件
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachmentType, GL_RENDERBUFFER, m_DepthStencilRBO);

		unbind();
		return *this;
	}

	void Framebuffer::setColorAttachment(unsigned int target, unsigned int targetType, int mipToWriteTo) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, targetType, target, mipToWriteTo);
	}

	void Framebuffer::bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	}

	void Framebuffer::unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Framebuffer::clear() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}


}