#include "pch.h"
#include "Cubemap.h"
#include "rhi/include/RHIContext.h"

namespace engine {

	// ============================================================================
	// 格式解析
	// ============================================================================

	rhi::TextureFormat Cubemap::resolveFormat(rhi::TextureFormat explicitFormat,
		ChannelLayout channels, bool isSRGB, bool formatExplicitlySet) {

		if (formatExplicitlySet) {
			if (isSRGB) {
				switch (explicitFormat) {
				case rhi::TextureFormat::RGB8:
				case rhi::TextureFormat::RGBA8:
					return rhi::TextureFormat::SRGB8_A8;
				default: break;
				}
			}
			return explicitFormat;
		}

		if (isSRGB) {
			return rhi::TextureFormat::SRGB8_A8;
		}

		switch (channels) {
		case ChannelLayout::R:    return rhi::TextureFormat::R8;
		case ChannelLayout::RG:   return rhi::TextureFormat::RG8;
		case ChannelLayout::RGB:  return rhi::TextureFormat::RGB8;
		case ChannelLayout::RGBA: return rhi::TextureFormat::RGBA8;
		default:                  return rhi::TextureFormat::RGBA8;
		}
	}

	rhi::TextureDesc Cubemap::buildCubemapDesc() const {
		rhi::TextureDesc desc;
		desc.type = rhi::TextureType::TextureCube;
		desc.width = m_FaceWidth;
		desc.height = m_FaceHeight;
		desc.levels = m_CubemapSettings.HasMips ? 0 : 1;
		desc.samples = 1;

		desc.format = m_CubemapSettings.format;

		desc.minFilter = m_CubemapSettings.minFilter;
		desc.magFilter = m_CubemapSettings.magFilter;
		desc.wrapS = m_CubemapSettings.wrapS;
		desc.wrapT = m_CubemapSettings.wrapT;
		desc.wrapR = m_CubemapSettings.wrapR;
		desc.anisotropy = m_CubemapSettings.anisotropy;
		desc.lodBias = m_CubemapSettings.MipBias;

		return desc;
	}

	// ============================================================================
	// 构造/析构
	// ============================================================================

	Cubemap::Cubemap(const CubemapSettings& settings)
		: m_RHIHandle(), m_Device(nullptr), m_FaceWidth(0), m_FaceHeight(0), m_FacesGenerated(0), m_CubemapSettings(settings)
	{
		m_Device = getRHIDevice();
	}

	Cubemap::~Cubemap() {
		if (m_Device && static_cast<bool>(m_RHIHandle)) {
			m_Device->destroyTexture(m_RHIHandle);
		}
	}

	// ============================================================================
	// 面生成
	// ============================================================================

	void Cubemap::generateCubemapFace(uint8_t face, unsigned int faceWidth, unsigned int faceHeight,
		ChannelLayout channels, const unsigned char* data) {
		if (!m_Device) {
			m_Device = getRHIDevice();
		}

		// 如果这是生成的第一个面，则创建 cubemap 纹理
		if (!static_cast<bool>(m_RHIHandle)) {
			m_FaceWidth = faceWidth;
			m_FaceHeight = faceHeight;

			// 解析格式
			m_CubemapSettings.format = resolveFormat(m_CubemapSettings.format, channels,
				m_CubemapSettings.IsSRGB, m_CubemapSettings.formatExplicitlySet);

			if (m_Device) {
				rhi::TextureDesc desc = buildCubemapDesc();
				desc.levels = 1; // 等所有面完成后再生成 mip
				m_RHIHandle = m_Device->createTexture(desc);
			}
		}

		if (!m_Device || !static_cast<bool>(m_RHIHandle)) return;

		m_Device->updateCubemapFace(m_RHIHandle, face, 0, m_FaceWidth, m_FaceHeight,
			channelLayoutToUploadFormat(channels), data, 0);

		++m_FacesGenerated;

		if (m_FacesGenerated >= 6) {
			applyCubemapSettings();
		}
	}

	// ============================================================================
	// 采样器设置
	// ============================================================================

	void Cubemap::applyCubemapSettings() {
		if (!m_Device || !static_cast<bool>(m_RHIHandle)) return;

		rhi::TextureDesc desc = buildCubemapDesc();
		desc.levels = m_CubemapSettings.HasMips ? 0 : 1;
		m_Device->updateTextureSampler(m_RHIHandle, desc);

		if (m_CubemapSettings.HasMips) {
			m_Device->generateMipmaps(m_RHIHandle);
		}
	}

	// ============================================================================
	// 绑定/解绑
	// ============================================================================

	void Cubemap::bind(int unit) {
		if (m_Device && static_cast<bool>(m_RHIHandle)) {
			m_Device->bindTexture(0, unit >= 0 ? unit : 0, m_RHIHandle);
		}
	}

	void Cubemap::unbind() {
		// 现代 RHI 不需要显式解绑纹理
	}

	// ============================================================================
	// 兼容旧代码
	// ============================================================================

	unsigned int Cubemap::getCubemapID() {
		// 已弃用：新代码应使用 getRHIHandle()
		return 0;
	}

}
