#pragma once

namespace engine {
	struct TextureSettings {
		// 纹理数据格式
		GLenum TextureFormat = GL_NONE;

		/*标志纹理是否为sRGB纹理，如果是，则在采样前要线性化
		* 用于颜色的都应该是线性化的，但是包含数据的 比如高度图和法线图不应该是线性化的
		* 自己生成的默认就是在线性空间的
		*/
		bool IsSRGB = false;

		// 纹理环绕选项
		GLenum TextureWrapSMode = GL_REPEAT;
		GLenum TextureWrapTMode = GL_REPEAT;
		bool HasBorder = false;
		glm::vec4 BorderColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

		// 纹理过滤选项
		GLenum TextureMinificationFilterMode = GL_LINEAR_MIPMAP_LINEAR;//多个纹素映射到一个像素时的过滤方式
		GLenum TextureMagnificationFilterMode = GL_LINEAR;// 多个像素映射到一个纹素时的过滤方式
		float TextureAnisotropyLevel = ANISOTROPIC_FILTERING_LEVEL; // 指定独立于纹理 min 和 mag 过滤，应为 2 的幂（1.0 表示使用通常的各向同性纹理过滤，这意味着不使用各向异性过滤）

		// Mip options
		bool HasMips = true;
		int MipBias = 0; // 正值表示选择较模糊的纹理，负值表示较清晰的纹理，可以显示纹理锯齿
	};
	class Texture {
	public:
		Texture(const TextureSettings& settings = TextureSettings());
		Texture(const Texture& teture);
		~Texture();

		// 创建纹理
		void generate2DTexture(unsigned int width, unsigned int height, GLenum dataFormat, GLenum pixelDataType = GL_UNSIGNED_BYTE, const void* data = nullptr);

		void generate2DMultisampleTexture(unsigned int width, unsigned int height);
		void generateMips();

		void bind(int unit = 0);
		void unbind();

		// Texture Tuning Functions
		void setTextureWrapS(GLenum textureWrapMode);
		void setTextureWrapT(GLenum textureWrapMode);
		void setHasBorder(bool hasBorder);
		void setBorderColor(glm::vec4& borderColor);
		void setTextureMinFilter(GLenum textureFilterMode);
		void setTextureMagFilter(GLenum textureFilterMode);
		void setAnisotropicFilteringMode(float textureAnisotropyLevel);
		void setMipBias(int mipBias);
		void setHasMips(bool hasMips);

		// Pre-generation controls only
		inline void setTextureSettings(TextureSettings settings) { m_TextureSettings = settings; }
		inline void setTextureFormat(GLenum format) { m_TextureSettings.TextureFormat = format; }
		// 不要用它来绑定纹理并使用它。 而是调用 Bind() 函数
		inline unsigned int getTextureId()const { return m_TextureId; }
		inline bool isGenerated() const { return m_TextureId != 0; }
		inline unsigned int getTextureTarget() const { return m_TextureTarget; }
		inline unsigned int getWidth() const { return m_Width; }
		inline unsigned int getHeight()const { return m_Height; }
		inline const TextureSettings& getTextureSettings() const { return m_TextureSettings; }
	private:
		void applyTextureSettings();
	private:
		unsigned int m_TextureId;
		GLenum m_TextureTarget;

		unsigned int m_Width, m_Height;
		GLenum m_TextureFormat;

		TextureSettings m_TextureSettings;
	};

}