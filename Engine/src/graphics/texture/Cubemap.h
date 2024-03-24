#pragma once

namespace engine {

	struct CubemapSettings {
		GLenum TextureFormat = GL_NONE;

		bool IsSRGB = false;
		// Texture wrapping options
		GLenum TextureWrapSMode = GL_CLAMP_TO_EDGE;
		GLenum TextureWrapTMode = GL_CLAMP_TO_EDGE;
		GLenum TextureWrapRMode = GL_CLAMP_TO_EDGE;

		// Texture filtering options
		GLenum TextureMinificationFilterMode = GL_LINEAR; // �������Ƶø�Զ���Ҷ������ӳ�䵽һ������ʱ�Ĺ���ģʽ���������Ի�����������
		GLenum TextureMagnificationFilterMode = GL_LINEAR; // �������ø��ӽ����Ҷ������ӳ�䵽��������ʱ�Ĺ���ģʽ����Զ����Ҫ����˫���ԣ���Ϊ���������������һ��׼ȷ��
		float TextureAnisotropyLevel = ANISOTROPIC_FILTERING_LEVEL;

		// Mip Settings
		bool HasMips = false;
		int MipBias = 0;
	};

	class Cubemap {
	public:
		Cubemap(const CubemapSettings& settings = CubemapSettings());
		~Cubemap();

		void generateCubemapFace(GLenum face, unsigned int faceWidth, unsigned int faceHeight, GLenum dataFormat, const unsigned char* data);

		void bind(int unit = 0);
		void unbind();

		// Pre-generation controls only
		inline void setCubemapSettings(CubemapSettings settings) { m_CubemapSettings = settings; }

		// Getters
		unsigned int getCubemapID() { return m_CubemapID; }

		inline unsigned int getFaceWidth() { return m_FaceWidth; }
		inline unsigned int getFaceHeight() { return m_FaceHeight; }

	private:
		void applyCubemapSettings();
	private:
		unsigned int m_CubemapID;

		unsigned int m_FaceWidth, m_FaceHeight;
		unsigned int m_FacesGenerated;

		CubemapSettings m_CubemapSettings;
	};

}