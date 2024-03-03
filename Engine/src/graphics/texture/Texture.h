#pragma once

namespace engine {
	struct TextureSettings {
		// Texture wrapping options
		GLenum TextureWrapSMode = GL_REPEAT;
		GLenum TextureWrapTMode = GL_REPEAT;

		// Texture filtering options
		GLenum TextureMinificationFilterMode = GL_LINEAR_MIPMAP_LINEAR; 
		GLenum TextureMagnificationFilterMode = GL_LINEAR; 
		float TextureAnisotropyLevel = ANISOTROPIC_FILTERING_LEVEL; // 指定独立于纹理 min 和 mag 过滤，应为 2 的幂（1.0 表示使用通常的各向同性纹理过滤，这意味着不使用各向异性过滤）

		// Mip options
		bool HasMips = true;
		int MipBias = 0; // 正值表示选择较模糊的纹理，负值表示较清晰的纹理，可以显示纹理锯齿
	};
	class Texture {
	public:
		Texture();
		Texture(TextureSettings &settings);
		~Texture();

		// 创建纹理
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
		// 不要用它来绑定纹理并使用它。 而是调用 Bind() 函数
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