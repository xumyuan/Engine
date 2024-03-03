#pragma once

namespace engine {
	struct TextureSettings {
		// Texture wrapping options
		GLenum TextureWrapSMode = GL_REPEAT;
		GLenum TextureWrapTMode = GL_REPEAT;

		// Texture filtering options
		GLenum TextureMinificationFilterMode = GL_LINEAR_MIPMAP_LINEAR; 
		GLenum TextureMagnificationFilterMode = GL_LINEAR; 
		float TextureAnisotropyLevel = ANISOTROPIC_FILTERING_LEVEL; // ָ������������ min �� mag ���ˣ�ӦΪ 2 ���ݣ�1.0 ��ʾʹ��ͨ���ĸ���ͬ��������ˣ�����ζ�Ų�ʹ�ø������Թ��ˣ�

		// Mip options
		bool HasMips = true;
		int MipBias = 0; // ��ֵ��ʾѡ���ģ����������ֵ��ʾ������������������ʾ������
	};
	class Texture {
	public:
		Texture();
		Texture(TextureSettings &settings);
		~Texture();

		// ��������
		void generate2DTexture(unsigned int width, unsigned int height, GLenum textureFormat, GLenum dataFormat, const void* data);

		void generate2DMultisampleTexture(unsigned int width, unsigned int height);

		void bind(int unit = -1);
		void unbind();

		// Texture Tuning Functions
		void setTextureWrapS(GLenum textureWrapMode, bool shouldBind = false);
		void setTextureWrapT(GLenum textureWrapMode, bool shouldBind = false);
		void setTextureMinFilter(GLenum textureFilterMode, bool shouldBind = false);
		void setTextureMagFilter(GLenum textureFilterMode, bool shouldBind = false);
		void setAnisotropicFilteringMode(float textureAnisotropyLevel, bool shouldBind = false);

		// Pre-generation controls only
		void setMipMode(bool shouldGenMips, int mipBias);
		inline void setTextureSettings(TextureSettings settings) { m_TextureSettings = settings; }
		// ��Ҫ������������ʹ������ ���ǵ��� Bind() ����
		inline unsigned int getTextureId() { return m_TextureId; }
		inline unsigned int getWidth() { return m_Width; }
		inline unsigned int getHeight() { return m_Height; }
	private:
		unsigned int m_TextureId;
		GLenum m_TextureTarget;

		unsigned int m_Width, m_Height;
		GLenum m_TextureFormat;

		TextureSettings m_TextureSettings;
	};

}