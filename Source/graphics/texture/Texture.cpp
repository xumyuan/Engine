#include "pch.h"
#include "Texture.h"
#include "utils/DebugEvent.h"

namespace engine {

	// ============================================================================
	// 格式解析
	// ============================================================================

	rhi::TextureFormat Texture::resolveFormat(rhi::TextureFormat explicitFormat,
		ChannelLayout channels, bool isSRGB, bool formatExplicitlySet) {

		// 如果格式是由用户显式设置的（不是默认值），直接使用
		if (formatExplicitlySet) {
			// 但如果要求 sRGB，尝试升级
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

		// 未显式指定格式：根据通道数推导
		if (isSRGB) {
			return rhi::TextureFormat::SRGB8_A8;
		}

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

	rhi::TextureDesc Texture::buildTextureDesc(unsigned int width, unsigned int height) const {
		rhi::TextureDesc desc;
		desc.type = rhi::TextureType::Texture2D;
		desc.width = width;
		desc.height = height;
		desc.levels = m_TextureSettings.HasMips ? 0 : 1; // 0 = 自动计算 mip 层级
		desc.samples = 1;

		desc.format = m_TextureSettings.format;

		// 采样器参数
		desc.minFilter = m_TextureSettings.minFilter;
		desc.magFilter = m_TextureSettings.magFilter;
		desc.wrapS = m_TextureSettings.wrapS;
		desc.wrapT = m_TextureSettings.wrapT;
		desc.anisotropy = m_TextureSettings.anisotropy;
		desc.hasBorder = m_TextureSettings.HasBorder;
		desc.borderColor[0] = m_TextureSettings.BorderColor[0];
		desc.borderColor[1] = m_TextureSettings.BorderColor[1];
		desc.borderColor[2] = m_TextureSettings.BorderColor[2];
		desc.borderColor[3] = m_TextureSettings.BorderColor[3];
		desc.lodBias = m_TextureSettings.MipBias;

		return desc;
	}

	// ============================================================================
	// 构造/析构
	// ============================================================================

	Texture::Texture() : m_RHIHandle(), m_Device(nullptr), m_Width(0), m_Height(0), m_IsMultisample(false), m_TextureSettings() {
		m_Device = getRHIDevice();
	}

	Texture::Texture(const TextureSettings& settings) : m_RHIHandle(), m_Device(nullptr), m_Width(0), m_Height(0), m_IsMultisample(false), m_TextureSettings(settings) {
		m_Device = getRHIDevice();
	}

	Texture::Texture(const Texture& texture) : m_RHIHandle(), m_Device(nullptr), m_Width(texture.getWidth()), m_Height(texture.getHeight()), m_IsMultisample(texture.isMultisample()), m_TextureSettings(texture.getTextureSettings()) {
		m_Device = getRHIDevice();
		if (m_Device && texture.isGenerated()) {
			rhi::TextureDesc desc = buildTextureDesc(m_Width, m_Height);
			desc.levels = 1;
			m_RHIHandle = m_Device->createTexture(desc);

			// 使用 RHI 接口进行纹理拷贝
			if (static_cast<bool>(m_RHIHandle) && static_cast<bool>(texture.getRHIHandle())) {
				m_Device->copyTexture(texture.getRHIHandle(), m_RHIHandle, m_Width, m_Height);
			}
		}
	}

	Texture::~Texture() {
		if (m_Device && static_cast<bool>(m_RHIHandle)) {
			m_Device->destroyTexture(m_RHIHandle);
		}
	}

	// ============================================================================
	// 纹理生成
	// ============================================================================

	void Texture::generate2DTexture(unsigned int width, unsigned int height,
		ChannelLayout channels, const void* data) {
		m_Width = width;
		m_Height = height;
		m_IsMultisample = false;

		// 根据通道数和 sRGB 标志解析最终格式
		m_TextureSettings.format = resolveFormat(m_TextureSettings.format, channels,
			m_TextureSettings.IsSRGB, m_TextureSettings.formatExplicitlySet);

		if (!m_Device) {
			m_Device = getRHIDevice();
		}

		if (m_Device) {
			rhi::TextureDesc desc = buildTextureDesc(width, height);
			desc.levels = 1; // 先不生成 mip
			m_RHIHandle = m_Device->createTexture(desc);

			if (data) {
				// 使用源数据的实际通道格式（而非纹理内部格式）上传数据
				rhi::TextureFormat srcFormat = channelLayoutToUploadFormat(channels);
				m_Device->updateTexture(m_RHIHandle, 0, 0, 0, width, height, srcFormat, data, 0);
			}

			applyTextureSettings();

			if (m_TextureSettings.HasMips) {
				m_Device->generateMipmaps(m_RHIHandle);
			}
		}
	}

	void Texture::generate2DMultisampleTexture(unsigned int width, unsigned int height) {
		m_Width = width;
		m_Height = height;
		m_IsMultisample = true;

		if (!m_Device) {
			m_Device = getRHIDevice();
		}

		if (m_Device) {
			rhi::TextureDesc desc;
			desc.type = rhi::TextureType::Texture2D;
			desc.format = m_TextureSettings.format;
			desc.width = width;
			desc.height = height;
			desc.levels = 1;
			desc.samples = MSAA_SAMPLE_AMOUNT;
			m_RHIHandle = m_Device->createTexture(desc);
		}
	}

	void Texture::generateMips() {
		m_TextureSettings.HasMips = true;
		if (m_Device && isGenerated()) {
			m_Device->generateMipmaps(m_RHIHandle);
		}
	}

	// ============================================================================
	// 绑定/解绑
	// ============================================================================

	void Texture::bind(int unit) {
		if (m_Device && isGenerated()) {
			m_Device->bindTexture(0, unit >= 0 ? unit : 0, m_RHIHandle);
		}
	}

	void Texture::unbind() {
		// 现代 RHI 不需要显式解绑纹理，绑定新纹理会自动覆盖
	}

	// ============================================================================
	// 采样器参数调整
	// ============================================================================

	void Texture::applyTextureSettings() {
		if (!m_Device || !isGenerated()) return;

		rhi::TextureDesc desc = buildTextureDesc(m_Width, m_Height);
		desc.samples = m_IsMultisample ? MSAA_SAMPLE_AMOUNT : 1;
		m_Device->updateTextureSampler(m_RHIHandle, desc);
	}

	void Texture::setTextureWrapS(rhi::WrapMode wrapMode) {
		if (m_TextureSettings.wrapS == wrapMode) return;
		m_TextureSettings.wrapS = wrapMode;
		if (isGenerated()) applyTextureSettings();
	}

	void Texture::setTextureWrapT(rhi::WrapMode wrapMode) {
		if (m_TextureSettings.wrapT == wrapMode) return;
		m_TextureSettings.wrapT = wrapMode;
		if (isGenerated()) applyTextureSettings();
	}

	void Texture::setHasBorder(bool hasBorder) {
		if (m_TextureSettings.HasBorder == hasBorder) return;
		m_TextureSettings.HasBorder = hasBorder;
		if (isGenerated()) applyTextureSettings();
	}

	void Texture::setBorderColor(float r, float g, float b, float a) {
		m_TextureSettings.BorderColor[0] = r;
		m_TextureSettings.BorderColor[1] = g;
		m_TextureSettings.BorderColor[2] = b;
		m_TextureSettings.BorderColor[3] = a;
		if (isGenerated()) applyTextureSettings();
	}

	void Texture::setTextureMinFilter(rhi::FilterMode filterMode) {
		if (m_TextureSettings.minFilter == filterMode) return;
		m_TextureSettings.minFilter = filterMode;
		if (isGenerated()) applyTextureSettings();
	}

	void Texture::setTextureMagFilter(rhi::FilterMode filterMode) {
		if (m_TextureSettings.magFilter == filterMode) return;
		m_TextureSettings.magFilter = filterMode;
		if (isGenerated()) applyTextureSettings();
	}

	void Texture::setAnisotropicFilteringMode(float level) {
		if (m_TextureSettings.anisotropy == level) return;
		m_TextureSettings.anisotropy = level;
		if (isGenerated()) applyTextureSettings();
	}

	void Texture::setMipBias(int mipBias) {
		if (m_TextureSettings.MipBias == mipBias) return;
		m_TextureSettings.MipBias = mipBias;
		if (isGenerated()) applyTextureSettings();
	}

	void Texture::setHasMips(bool hasMips) {
		if (m_TextureSettings.HasMips == hasMips) return;
		m_TextureSettings.HasMips = hasMips;
		if (isGenerated() && hasMips) {
			m_Device->generateMipmaps(m_RHIHandle);
		}
	}

	// ============================================================================
	// 兼容旧代码
	// ============================================================================

	unsigned int Texture::getTextureId() const {
		// 已弃用：新代码应使用 getRHIHandle()
		return 0;
	}

}
