#pragma once
#include "utils/loaders/TextureLoader.h"

namespace engine {
	namespace opengl {

		class RenderTarget {
		public:
			RenderTarget(unsigned int width, unsigned int height);
			~RenderTarget();

			void createFramebuffer();
			RenderTarget& addColorAttachment(bool multisampledBuffer);
			RenderTarget& addDepthStencilRBO(bool multisampledBuffer);
			RenderTarget& addDepthAttachment(bool multisampledBuffer);

			void bind();
			void unbind();

			void clear();

			inline unsigned int getWidth() { return m_Width; }
			inline unsigned int getHeight() { return m_Height; }

			inline unsigned int getFramebuffer() { return m_FBO; }
			inline unsigned int getColorBufferTexture() { return m_ColorTexture; }
			inline unsigned int getDepthTexture() { return m_DepthTexture; }

			inline bool isMultisampledColourBuffer() { return m_IsMultisampledColourBuffer; }
		private:
			unsigned int m_FBO;

			bool m_IsMultisampledColourBuffer;
			// Attachments
			unsigned int m_ColorTexture;
			unsigned int m_DepthStencilRBO;
			unsigned int m_DepthTexture;

			unsigned int m_Width, m_Height;
		};

	}
}