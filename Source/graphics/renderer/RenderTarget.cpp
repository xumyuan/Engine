#include "pch.h"
#include "RenderTarget.h"

namespace engine {

	rhi::TextureFormat RenderTarget::toRHIFormat(DepthStencilFormat fmt) {
		switch (fmt) {
		case DepthStencilFormat::DepthOnly:         return rhi::TextureFormat::Depth24;
		case DepthStencilFormat::DepthStencil:       return rhi::TextureFormat::Depth24Stencil8;
		case DepthStencilFormat::DepthStencilFloat:  return rhi::TextureFormat::Depth32FStencil8;
		default:                                     return rhi::TextureFormat::Depth24Stencil8;
		}
	}

	RenderTarget::RenderTarget(uint32_t width, uint32_t height, uint8_t samples)
		: m_Device(getRHIDevice()), m_RTHandle(), m_Width(width), m_Height(height), m_Samples(samples)
	{
	}

	RenderTarget::~RenderTarget() {
		if (m_Device) {
			if (static_cast<bool>(m_RTHandle)) {
				m_Device->destroyRenderTarget(m_RTHandle);
			}
			// 如果深度 handle 由本类创建（sampleable=false），则由本类销毁
			if (m_OwnsDepthHandle && static_cast<bool>(m_DepthHandle)) {
				m_Device->destroyTexture(m_DepthHandle);
			}
		}
	}

	RenderTarget& RenderTarget::addColorTexture(rhi::TextureFormat format) {
		if (m_ColorTextureCount >= MAX_COLORS) return *this;

		TextureSettings settings;
		settings.format = format;
		settings.formatExplicitlySet = true;
		settings.wrapS = rhi::WrapMode::ClampToEdge;
		settings.wrapT = rhi::WrapMode::ClampToEdge;
		settings.minFilter = rhi::FilterMode::Linear;
		settings.magFilter = rhi::FilterMode::Linear;
		settings.anisotropy = 1.0f;
		settings.HasMips = false;
		m_ColorTextures[m_ColorTextureCount].setTextureSettings(settings);

		if (m_Samples > 1) {
			m_ColorTextures[m_ColorTextureCount].generate2DMultisampleTexture(m_Width, m_Height);
		} else {
			m_ColorTextures[m_ColorTextureCount].generate2DTexture(m_Width, m_Height, ChannelLayout::RGBA);
		}

		m_ColorTextureCount++;
		return *this;
	}

	RenderTarget& RenderTarget::addExternalColorTexture(Texture* texture) {
		if (m_ColorTextureCount >= MAX_COLORS || !texture) return *this;
		m_ExternalColorHandles[m_ColorTextureCount] = texture->getRHIHandle();
		m_ColorIsExternal[m_ColorTextureCount] = true;
		m_ColorTextureCount++;
		return *this;
	}

	RenderTarget& RenderTarget::addDepthStencilTexture(DepthStencilFormat format, bool sampleable) {
		m_DepthFormat = toRHIFormat(format);
		m_DepthSampleable = sampleable;

		if (sampleable) {
			// 需要后续采样 → 创建真正的深度纹理
			TextureSettings settings;
			settings.format = m_DepthFormat;
			settings.formatExplicitlySet = true;
			settings.wrapS = rhi::WrapMode::ClampToBorder;
			settings.wrapT = rhi::WrapMode::ClampToBorder;
			settings.minFilter = rhi::FilterMode::Nearest;
			settings.magFilter = rhi::FilterMode::Nearest;
			settings.anisotropy = 1.0f;
			settings.HasBorder = true;
			settings.BorderColor[0] = 1.0f;
			settings.BorderColor[1] = 1.0f;
			settings.BorderColor[2] = 1.0f;
			settings.BorderColor[3] = 1.0f;
			settings.HasMips = false;
			m_DepthStencilTexture.setTextureSettings(settings);

			ChannelLayout depthChannels = (format == DepthStencilFormat::DepthOnly)
				? ChannelLayout::Depth : ChannelLayout::DepthStencil;

			if (m_Samples > 1) {
				m_DepthStencilTexture.generate2DMultisampleTexture(m_Width, m_Height);
			} else {
				m_DepthStencilTexture.generate2DTexture(m_Width, m_Height, depthChannels);
			}

			m_HasDepthTexture = true;
		} else {
			// 不需要后续采样 → 只声明意图，让 RHI 后端自动选最优实现
			// 创建 usage = DepthAttachment (无 Sampled) 的纹理
			// OpenGL 后端会自动选 renderbuffer
			rhi::TextureDesc desc;
			desc.type = rhi::TextureType::Texture2D;
			desc.format = m_DepthFormat;
			desc.usage = rhi::TextureUsage::DepthAttachment;  // ← 关键：无 Sampled
			desc.width = m_Width;
			desc.height = m_Height;
			desc.levels = 1;
			desc.samples = m_Samples;

			m_DepthHandle = m_Device->createTexture(desc);
			m_OwnsDepthHandle = true;
			m_HasDepthTexture = false; // 无可采样的 Texture 对象
		}

		return *this;
	}

	void RenderTarget::build() {
		if (!m_Device) return;

		rhi::RenderTargetDesc desc;
		desc.width = m_Width;
		desc.height = m_Height;
		desc.samples = m_Samples;

		// 颜色附件
		desc.colorCount = m_ColorTextureCount;
		for (uint8_t i = 0; i < m_ColorTextureCount; ++i) {
			if (m_ColorIsExternal[i]) {
				desc.colorAttachments[i] = m_ExternalColorHandles[i];
			} else {
				desc.colorAttachments[i] = m_ColorTextures[i].getRHIHandle();
			}
		}

		// 深度附件 —— 统一通过 depthAttachment handle
		// 后端自动判断是 texture 还是 renderbuffer
		if (m_HasDepthTexture) {
			desc.depthAttachment = m_DepthStencilTexture.getRHIHandle();
		} else if (m_OwnsDepthHandle) {
			desc.depthAttachment = m_DepthHandle;
		}

		m_RTHandle = m_Device->createRenderTarget(desc);
	}

	void RenderTarget::beginPass(const rhi::RenderPassParams& params) {
		if (m_Device && static_cast<bool>(m_RTHandle)) {
			m_Device->beginRenderPass(m_RTHandle, params);
		}
	}

	void RenderTarget::beginPass() {
		rhi::RenderPassParams params;
		params.viewport = { 0, 0, m_Width, m_Height };
		params.clearColorFlag = true;
		params.clearDepthFlag = true;
		beginPass(params);
	}

	void RenderTarget::endPass() {
		if (m_Device) {
			m_Device->endRenderPass();
		}
	}

	void RenderTarget::setColorAttachment(uint8_t index, rhi::TextureHandle texture,
		uint8_t level, uint8_t layer) {
		if (m_Device && static_cast<bool>(m_RTHandle)) {
			m_Device->setRenderTargetColorAttachment(m_RTHandle, index, texture, level, layer);
		}
	}

}
