#pragma once

namespace engine {

	struct CubemapSettings {
		// Texture wrapping options
		GLenum TextureWrapSMode = GL_CLAMP_TO_EDGE;
		GLenum TextureWrapTMode = GL_CLAMP_TO_EDGE;
		GLenum TextureWrapRMode = GL_CLAMP_TO_EDGE;

		// Texture filtering options
		GLenum TextureMinificationFilterMode = GL_LINEAR; // 当纹理移得更远并且多个纹素映射到一个像素时的过滤模式（三线性以获得最佳质量）
		GLenum TextureMagnificationFilterMode = GL_LINEAR; // 当纹理变得更接近并且多个像素映射到单个纹素时的过滤模式（永远不需要超过双线性，因为这与在这种情况下一样准确）

		// Mip Settings
		bool HasMips = false;
	};

	class Cubemap {
	public:
		Cubemap();
		Cubemap(CubemapSettings &settings);
		~Cubemap();

		void generateCubemapFace(GLenum face, unsigned int faceWidth, unsigned int faceHeight, GLenum textureFormat, GLenum dataFormat, const unsigned char* data);

		void bind(int unit = -1);
		void unbind();

		// Pre-generation controls only
		inline void setCubemapSettings(CubemapSettings settings) {
			m_CubemapSettings = settings;
		}

		// Getters
		unsigned int getCubemapID() { return m_CubemapID; }
	private:
		// TODO:研究更好的过滤，例如各向异性支持，并研究立方体贴图的适当 mips
		unsigned int m_CubemapID;

		unsigned int m_FaceWidth, m_FaceHeight;
		unsigned int m_FacesGenerated;
		GLenum m_TextureFormat;

		CubemapSettings m_CubemapSettings;
	};

}