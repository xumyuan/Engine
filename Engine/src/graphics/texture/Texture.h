#pragma once

namespace engine {
	struct TextureSettings {
		// �������ݸ�ʽ
		GLenum TextureFormat = GL_NONE;

		/*��־�����Ƿ�ΪsRGB��������ǣ����ڲ���ǰҪ���Ի�
		* ������ɫ�Ķ�Ӧ�������Ի��ģ����ǰ������ݵ� ����߶�ͼ�ͷ���ͼ��Ӧ�������Ի���
		* �Լ����ɵ�Ĭ�Ͼ��������Կռ��
		*/
		bool IsSRGB = false;

		// ������ѡ��
		GLenum TextureWrapSMode = GL_REPEAT;
		GLenum TextureWrapTMode = GL_REPEAT;
		bool HasBorder = false;
		glm::vec4 BorderColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

		// �������ѡ��
		GLenum TextureMinificationFilterMode = GL_LINEAR_MIPMAP_LINEAR;//�������ӳ�䵽һ������ʱ�Ĺ��˷�ʽ
		GLenum TextureMagnificationFilterMode = GL_LINEAR;// �������ӳ�䵽һ������ʱ�Ĺ��˷�ʽ
		float TextureAnisotropyLevel = ANISOTROPIC_FILTERING_LEVEL; // ָ������������ min �� mag ���ˣ�ӦΪ 2 ���ݣ�1.0 ��ʾʹ��ͨ���ĸ���ͬ��������ˣ�����ζ�Ų�ʹ�ø������Թ��ˣ�

		// Mip options
		bool HasMips = true;
		int MipBias = 0; // ��ֵ��ʾѡ���ģ����������ֵ��ʾ������������������ʾ������
	};
	class Texture {
	public:
		Texture(const TextureSettings& settings = TextureSettings());
		Texture(const Texture& teture);
		~Texture();

		// ��������
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
		// ��Ҫ������������ʹ������ ���ǵ��� Bind() ����
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