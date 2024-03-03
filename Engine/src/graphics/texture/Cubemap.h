#pragma once

namespace engine {

	struct CubemapSettings {
		// Texture wrapping options
		GLenum TextureWrapSMode = GL_CLAMP_TO_EDGE;
		GLenum TextureWrapTMode = GL_CLAMP_TO_EDGE;
		GLenum TextureWrapRMode = GL_CLAMP_TO_EDGE;

		// Texture filtering options
		GLenum TextureMinificationFilterMode = GL_LINEAR; // �������Ƶø�Զ���Ҷ������ӳ�䵽һ������ʱ�Ĺ���ģʽ���������Ի�����������
		GLenum TextureMagnificationFilterMode = GL_LINEAR; // �������ø��ӽ����Ҷ������ӳ�䵽��������ʱ�Ĺ���ģʽ����Զ����Ҫ����˫���ԣ���Ϊ���������������һ��׼ȷ��

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
		// TODO:�о����õĹ��ˣ������������֧�֣����о���������ͼ���ʵ� mips
		unsigned int m_CubemapID;

		unsigned int m_FaceWidth, m_FaceHeight;
		unsigned int m_FacesGenerated;
		GLenum m_TextureFormat;

		CubemapSettings m_CubemapSettings;
	};

}