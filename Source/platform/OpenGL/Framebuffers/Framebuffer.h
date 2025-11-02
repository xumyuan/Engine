#pragma once
#include "utils/loaders/TextureLoader.h"

namespace engine {

	enum ColorAttachmentFormat {
		NormalizedSingleChannel8 = GL_RED,
		Normalized8 = GL_RGBA8,
		Normalized16 = GL_RGBA16,
		FloatingPoint16 = GL_RGBA16F,
		FloatingPoint32 = GL_RGBA32F
	};

	enum DepthStencilAttachmentFormat {
		NormalizedDepthOnly = GL_DEPTH_COMPONENT,
		NormalizedDepthStencil = GL_DEPTH24_STENCIL8,
		FloatingPointDepthStencil = GL_DEPTH32F_STENCIL8
	};

	enum StencilValue : int
	{
		ModelStencilValue = 0x01,
		TerrainStencilValue = 0x02
	};

	class Framebuffer {
	public:
		Framebuffer(unsigned int width, unsigned int height, bool isMultisampled);
		virtual ~Framebuffer();

		void createFramebuffer();
		Framebuffer& addColorTexture(ColorAttachmentFormat textureFormat);
		Framebuffer& addDepthStencilTexture(DepthStencilAttachmentFormat textureFormat);
		Framebuffer& addDepthStencilRBO(DepthStencilAttachmentFormat rboFormat);
		//Framebuffer& addDepthAttachment();

		void bind();
		void unbind();

		// Assumes framebuffer is bound
		void setColorAttachment(unsigned int target, unsigned int targetType, int mipToWriteT0 = 0);
		void clear();

		inline unsigned int getWidth() { return m_Width; }
		inline unsigned int getHeight() { return m_Height; }

		inline unsigned int getFramebuffer() { return m_FBO; }
		inline bool isMultisampled() { return m_IsMultisampled; }

		inline Texture* getColorBufferTexture() { return &m_ColorTexture; }
		inline Texture* getDepthStencilTexture() { return &m_DepthStencilTexture; }

		inline unsigned int getDepthStencilRBO() { return m_DepthStencilRBO; }


	protected:
		unsigned int m_FBO;
		unsigned int m_Width, m_Height;

		bool m_IsMultisampled;

		// Attachments
		Texture m_ColorTexture;
		Texture m_DepthStencilTexture;
		unsigned int m_DepthStencilRBO;

	};


}
