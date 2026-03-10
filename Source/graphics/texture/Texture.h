#pragma once

#include "rhi/include/RHIDevice.h"
#include "rhi/include/RHIResources.h"

namespace engine {

	struct TextureSettings {
		// 纹理格式（RHI 枚举）
		rhi::TextureFormat format = rhi::TextureFormat::RGBA8;

		// 标记格式是否被显式设置（用于 resolveFormat 判断是否应根据通道数推导格式）
		bool formatExplicitlySet = false;

		// 是否为 sRGB 纹理（如果是，采样前自动线性化）
		// 用于颜色的纹理应为 sRGB，数据纹理（法线、高度等）不应
		bool IsSRGB = false;

		// 纹理环绕选项
		rhi::WrapMode wrapS = rhi::WrapMode::Repeat;
		rhi::WrapMode wrapT = rhi::WrapMode::Repeat;
		bool HasBorder = false;
		float BorderColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};

		// 纹理过滤选项
		rhi::FilterMode minFilter = rhi::FilterMode::LinearMipmapLinear;
		rhi::FilterMode magFilter = rhi::FilterMode::Linear;
		float anisotropy = 8.0f; // ANISOTROPIC_FILTERING_LEVEL

		// Mip options
		bool HasMips = true;
		int MipBias = 0;
	};

	// 通道数枚举（替代 GL_RED, GL_RGB, GL_RGBA 等）
	enum class ChannelLayout : uint8_t {
		R = 1,
		RG = 2,
		RGB = 3,
		RGBA = 4,
		Depth,
		DepthStencil,
	};

	// 将 ChannelLayout 转为 TextureFormat（用作上传数据的源格式描述）
	inline rhi::TextureFormat channelLayoutToUploadFormat(ChannelLayout channels) {
		switch (channels) {
		case ChannelLayout::R:            return rhi::TextureFormat::R8;
		case ChannelLayout::RG:           return rhi::TextureFormat::RG8;
		case ChannelLayout::RGB:          return rhi::TextureFormat::RGB8;
		case ChannelLayout::RGBA:         return rhi::TextureFormat::RGBA8;
		case ChannelLayout::Depth:        return rhi::TextureFormat::Depth24;
		case ChannelLayout::DepthStencil: return rhi::TextureFormat::Depth24Stencil8;
		default:                          return rhi::TextureFormat::RGBA8;
		}
	}

	class Texture {
	public:
		Texture();
		Texture(const TextureSettings& settings);
		Texture(const Texture& texture);
		~Texture();

		// 创建纹理（通过 RHI Device）
		void generate2DTexture(unsigned int width, unsigned int height,
			ChannelLayout channels = ChannelLayout::RGBA,
			const void* data = nullptr);

		void generate2DMultisampleTexture(unsigned int width, unsigned int height);
		void generateMips();

		void bind(int unit = 0);
		void unbind();

		// Texture Tuning Functions（使用 RHI 枚举）
		void setTextureWrapS(rhi::WrapMode wrapMode);
		void setTextureWrapT(rhi::WrapMode wrapMode);
		void setHasBorder(bool hasBorder);
		void setBorderColor(float r, float g, float b, float a);
		void setTextureMinFilter(rhi::FilterMode filterMode);
		void setTextureMagFilter(rhi::FilterMode filterMode);
		void setAnisotropicFilteringMode(float level);
		void setMipBias(int mipBias);
		void setHasMips(bool hasMips);

		// Pre-generation controls only
		inline void setTextureSettings(const TextureSettings& settings) { m_TextureSettings = settings; }
		inline void setTextureFormat(rhi::TextureFormat format) { m_TextureSettings.format = format; m_TextureSettings.formatExplicitlySet = true; }

		// 获取 RHI 句柄
		inline rhi::TextureHandle getRHIHandle() const { return m_RHIHandle; }

		// 兼容旧代码：获取底层 GL 纹理 ID（过渡期使用）
		unsigned int getTextureId() const;
		inline bool isGenerated() const { return static_cast<bool>(m_RHIHandle); }
		inline unsigned int getWidth() const { return m_Width; }
		inline unsigned int getHeight() const { return m_Height; }
		inline bool isMultisample() const { return m_IsMultisample; }
		inline const TextureSettings& getTextureSettings() const { return m_TextureSettings; }

	private:
		void applyTextureSettings();
		rhi::TextureDesc buildTextureDesc(unsigned int width, unsigned int height) const;

		// 根据通道数和 sRGB 标志确定纹理格式
		static rhi::TextureFormat resolveFormat(rhi::TextureFormat explicitFormat,
			ChannelLayout channels, bool isSRGB, bool formatExplicitlySet);

	private:
		rhi::TextureHandle m_RHIHandle;
		rhi::RHIDevice* m_Device;

		unsigned int m_Width, m_Height;
		bool m_IsMultisample;

		TextureSettings m_TextureSettings;
	};

}
