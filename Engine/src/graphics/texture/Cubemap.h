#pragma once

namespace engine {

	struct CubemapSettings {
		// Texture wrapping options
		GLenum TextureWrapSMode = GL_CLAMP_TO_EDGE;
		GLenum TextureWrapTMode = GL_CLAMP_TO_EDGE;
		GLenum TextureWrapRMode = GL_CLAMP_TO_EDGE;

		// Texture filtering options
		GLenum TextureMinificationFilterMode = GL_LINEAR; // Filtering mode when the texture moves further away and multiple texels map to one pixel (trilinear for best quality)
		GLenum TextureMagnificationFilterMode = GL_LINEAR; // Filtering mode when the texture gets closer and multiple pixels map to a single texel (Never needs to be more than bilinear because that is as accurate as it gets in this sitation)
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
		// TODO:�о����õĹ��ˣ������������֧�֣����о���������ͼ���ʵ� mips
		unsigned int m_CubemapID;

		unsigned int m_FaceWidth, m_FaceHeight;
		GLenum m_TextureFormat;

		CubemapSettings m_CubemapSettings;
	};

}