#pragma once

namespace engine {
	struct TextureSettings {
		// Texture wrapping options
		GLenum TextureWrapSMode = GL_REPEAT;
		GLenum TextureWrapTMode = GL_REPEAT;

		// Texture filtering options
		GLenum TextureMinificationFilterMode = GL_LINEAR_MIPMAP_LINEAR; // Filtering mode when the texture moves further away and multiple texels map to one pixel (trilinear for best quality)
		GLenum TextureMagnificationFilterMode = GL_LINEAR; // Filtering mode when the texture gets closer and multiple pixels map to a single texel (Never needs to be more than bilinear because that is as accurate as it gets in this sitation)
		float TextureAnisotropyLevel = ANISOTROPIC_FILTERING_LEVEL; // Specified independent of texture min and mag filtering, should be a power of 2 (1.0 means the usual isotropic texture filtering is used which means anisotropic filtering isn't used)

		// Mip options
		bool HasMips = true;
		int MipBias = 0; // positive means blurrier texture selected, negative means sharper texture which can show texture aliasing
	};
	class Texture {
	public:
		Texture();
		Texture(TextureSettings settings);
		~Texture();

		// ��������
		void generate2DTexture(unsigned int width, unsigned int height, GLenum textureFormat, GLenum dataFormat, const void* data);

		void generate2DMultisampleTexture(unsigned int width, unsigned int height, GLenum textureFormat, int numSamples);

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
		// Don't use this to bind the texture and use it. Call the Bind() function instead
		inline unsigned int getTextureId() { return m_TextureId; }
	private:
		unsigned int m_TextureId;
		GLenum m_TextureTarget;

		unsigned int m_Width, m_Height;
		GLenum m_TextureFormat;

		TextureSettings m_TextureSettings;
	};

}