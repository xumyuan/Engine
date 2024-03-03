#pragma once
#include "utils/loaders/TextureLoader.h"

namespace engine {

	class Framebuffer {
	public:
		Framebuffer(unsigned int width, unsigned int height);
		~Framebuffer();

		void createFramebuffer();
		Framebuffer& addTexture2DColorAttachment(bool multisampledBuffer);
		Framebuffer& addDepthRBO(bool multisampledBuffer);
		Framebuffer& addDepthStencilRBO(bool multisampledBuffer);
		Framebuffer& addDepthAttachment(bool multisampledBuffer);

		void bind();
		void unbind();

		// Assumes framebuffer is bound
		void setColorAttachment(unsigned int target, unsigned int targetType, int mipToWriteT0 = 0);
		void clear();

		inline unsigned int getWidth() { return m_Width; }
		inline unsigned int getHeight() { return m_Height; }

		inline unsigned int getFramebuffer() { return m_FBO; }
		inline unsigned int getColorBufferTexture() { return m_ColorTexture; }
		inline unsigned int getDepthRBO() { return m_DepthRBO; }
		inline unsigned int getDepthStencilRBO() { return m_DepthStencilRBO; }
		inline unsigned int getDepthTexture() { return m_DepthTexture; }

		inline bool isMultisampledColourBuffer() { return m_IsMultisampledColourBuffer; }
	private:
		unsigned int m_FBO;

		bool m_IsMultisampledColourBuffer;
		// Attachments
		unsigned int m_ColorTexture;
		unsigned int m_DepthRBO;
		unsigned int m_DepthStencilRBO;
		unsigned int m_DepthTexture;

		unsigned int m_Width, m_Height;
	};


}