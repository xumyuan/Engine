#pragma once

#include <graphics/texture/Texture.h>
#include <graphics/renderer/renderpass/RenderPassType.h>

namespace engine {

	// 现代 RenderTarget 包装类，完全基于 RHI 接口
	// 替代旧的 Framebuffer 类
	class RenderTarget {
	public:
		RenderTarget(uint32_t width, uint32_t height, uint8_t samples = 1);
		~RenderTarget();

		// Builder 模式
		RenderTarget& addColorTexture(rhi::TextureFormat format);
		RenderTarget& addExternalColorTexture(Texture* texture);

		// 深度/模板附件
		// sampleable = true:  后续 pass 需要采样深度 (如 shadowmap、deferred GBuffer)
		// sampleable = false: 仅用于深度测试，不需要后续采样
		//                     后端自动选最优实现 (OpenGL→RBO, Vulkan→transient, Metal→memoryless)
		RenderTarget& addDepthStencilTexture(DepthStencilFormat format, bool sampleable = true);

		void build();

		// 渲染操作
		void beginPass(const rhi::RenderPassParams& params);
		void beginPass(); // 使用默认参数（清除所有，viewport 全屏）
		void endPass();

		// 动态附件管理
		void setColorAttachment(uint8_t index, rhi::TextureHandle texture,
			uint8_t level = 0, uint8_t layer = 0);

		// Getters
		inline uint32_t getWidth() const { return m_Width; }
		inline uint32_t getHeight() const { return m_Height; }
		inline bool isMultisampled() const { return m_Samples > 1; }
		inline rhi::RenderTargetHandle getHandle() const { return m_RTHandle; }

		// 获取附件纹理
		inline Texture* getColorTexture(uint8_t index = 0) {
			return (index < m_ColorTextureCount) ? &m_ColorTextures[index] : nullptr;
		}
		inline Texture* getDepthStencilTexture() {
			return m_HasDepthTexture ? &m_DepthStencilTexture : nullptr;
		}

	private:
		rhi::TextureFormat toRHIFormat(DepthStencilFormat fmt);

	private:
		rhi::RHIDevice* m_Device;
		rhi::RenderTargetHandle m_RTHandle;

		uint32_t m_Width, m_Height;
		uint8_t m_Samples;

		// 颜色附件
		static constexpr uint8_t MAX_COLORS = 8;
		Texture m_ColorTextures[MAX_COLORS];
		rhi::TextureHandle m_ExternalColorHandles[MAX_COLORS]; // 外部纹理 handle（不归本类管理）
		bool m_ColorIsExternal[MAX_COLORS] = {};
		uint8_t m_ColorTextureCount = 0;

		// 深度/模板附件（统一由 TextureHandle 管理，后端自动决定 texture/RBO）
		Texture m_DepthStencilTexture;
		rhi::TextureHandle m_DepthHandle; // 不可采样时由本类直接通过 RHI 创建
		bool m_HasDepthTexture = false;
		bool m_DepthSampleable = true;
		bool m_OwnsDepthHandle = false;   // 当 sampleable=false 时本类管理 handle 生命周期
		rhi::TextureFormat m_DepthFormat = rhi::TextureFormat::Depth24Stencil8;
	};

}
