#include "pch.h"
#include "Cubemap.h"

namespace engine {

	Cubemap::Cubemap(const CubemapSettings& settings) : m_CubemapID(0), m_FaceWidth(0), m_FaceHeight(0), m_FacesGenerated(0), m_CubemapSettings(settings) {}

	Cubemap::~Cubemap() {
		glDeleteTextures(1, &m_CubemapID);
	}

	void Cubemap::applyCubemapSettings() {
		// Texture wrapping
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, m_CubemapSettings.TextureWrapSMode);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, m_CubemapSettings.TextureWrapTMode);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, m_CubemapSettings.TextureWrapRMode);

		// Texture filtering
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, m_CubemapSettings.TextureMagnificationFilterMode);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, m_CubemapSettings.TextureMinificationFilterMode);

		// Mipmapping
		if (m_CubemapSettings.HasMips) {
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_LOD_BIAS, m_CubemapSettings.MipBias);
		}

		// Anisotropic filtering (TODO: Move the anistropyAmount calculation to Defs.h to avoid querying the OpenGL driver everytime)
		float maxAnisotropy;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
		float anistropyAmount = glm::min(maxAnisotropy, m_CubemapSettings.TextureAnisotropyLevel);
		glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, anistropyAmount);
	}

	void Cubemap::generateCubemapFace(GLenum face, unsigned int faceWidth, unsigned int faceHeight, GLenum dataFormat, const unsigned char* data)
	{
		// 如果这是生成的第一个面，则生成立方体贴图
		if (m_CubemapID == 0) {
			glGenTextures(1, &m_CubemapID);

			m_FaceWidth = faceWidth;
			m_FaceHeight = faceHeight;
			if (m_CubemapSettings.TextureFormat == GL_NONE) {
				m_CubemapSettings.TextureFormat = dataFormat;
			}

			if (m_CubemapSettings.IsSRGB) {
				switch (dataFormat) {
				case GL_RGB: m_CubemapSettings.TextureFormat = GL_SRGB; break;
				case GL_RGBA: m_CubemapSettings.TextureFormat = GL_SRGB_ALPHA; break;
				}
			}
		}

		bind();

		glTexImage2D(face, 0, m_CubemapSettings.TextureFormat, m_FaceWidth, m_FaceHeight, 0, dataFormat, GL_UNSIGNED_BYTE, data);

		++m_FacesGenerated;

		if (m_FacesGenerated >= 6) {
			applyCubemapSettings();
		}

		unbind();
	}

	void Cubemap::bind(int unit) {
		/*if (unit >= 0)*/
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_CubemapID);
	}

	void Cubemap::unbind() {
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

}
