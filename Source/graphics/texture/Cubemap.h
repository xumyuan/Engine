#pragma once

#include "Texture.h"  // for ChannelLayout

namespace engine {

	struct CubemapSettings {
		rhi::TextureFormat format = rhi::TextureFormat::RGBA8;

		// 标记格式是否被显式设置
		bool formatExplicitlySet = false;

		bool IsSRGB = false;

		// Texture wrapping options
		rhi::WrapMode wrapS = rhi::WrapMode::ClampToEdge;
		rhi::WrapMode wrapT = rhi::WrapMode::ClampToEdge;
		rhi::WrapMode wrapR = rhi::WrapMode::ClampToEdge;

		// Texture filtering options
		rhi::FilterMode minFilter = rhi::FilterMode::Linear;
		rhi::FilterMode magFilter = rhi::FilterMode::Linear;
		float anisotropy = 8.0f; // ANISOTROPIC_FILTERING_LEVEL

		// Mip Settings
		bool HasMips = false;
		int MipBias = 0;
	};

	class Cubemap {
	public:
		Cubemap(const CubemapSettings& settings = CubemapSettings());
		~Cubemap();

		// face: 0~5 对应 +X,-X,+Y,-Y,+Z,-Z
		void generateCubemapFace(uint8_t face, unsigned int faceWidth, unsigned int faceHeight,
			ChannelLayout channels, const unsigned char* data);

		void bind(int unit = 0);
		void unbind();

		// Pre-generation controls only
		inline void setCubemapSettings(const CubemapSettings& settings) { m_CubemapSettings = settings; }

		// Getters
		inline rhi::TextureHandle getRHIHandle() const { return m_RHIHandle; }

		// 兼容旧代码
		unsigned int getCubemapID();

		inline unsigned int getFaceWidth() { return m_FaceWidth; }
		inline unsigned int getFaceHeight() { return m_FaceHeight; }

	private:
		void applyCubemapSettings();
		rhi::TextureDesc buildCubemapDesc() const;

		static rhi::TextureFormat resolveFormat(rhi::TextureFormat explicitFormat,
			ChannelLayout channels, bool isSRGB, bool formatExplicitlySet);

	private:
		rhi::TextureHandle m_RHIHandle;
		rhi::RHIDevice* m_Device;

		unsigned int m_FaceWidth, m_FaceHeight;
		unsigned int m_FacesGenerated;

		CubemapSettings m_CubemapSettings;
	};

}
