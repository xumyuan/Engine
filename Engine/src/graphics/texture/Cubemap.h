#pragma once

#include <gl/glew.h>
#include "../../Defs.h"

namespace engine {
	namespace graphics {

		class Cubemap {
		public:
			Cubemap();
			~Cubemap();

			void generateCubemapFace(GLenum face, unsigned int faceWidth, unsigned int faceHeight, GLenum textureFormat, GLenum dataFormat, const unsigned char* data);

			void bind(int unit = -1);
			void unbind();
		private:

			// TODO:研究更好的过滤，例如各向异性支持，并研究立方体贴图的适当 mips
			unsigned int m_CubemapId;

			unsigned int m_FaceWidth, m_FaceHeight;
			GLenum m_TextureFormat;

			// Texture wrapping options
			GLenum m_TextureWrapSMode = GL_CLAMP_TO_EDGE;
			GLenum m_TextureWrapTMode = GL_CLAMP_TO_EDGE;
			GLenum m_TextureWrapRMode = GL_CLAMP_TO_EDGE;

			// Texture filtering options
			GLenum m_TextureMinificationFilterMode = GL_LINEAR; // 当纹理移得更远并且多个纹素映射到一个像素时的过滤模式（三线性以获得最佳质量）
			GLenum m_TextureMagnificationFilterMode = GL_LINEAR; // 当纹理变得更接近并且多个像素映射到单个纹素时的过滤模式（永远不需要超过双线性，因为这与在这种情况下一样准确）

		};
	}
}