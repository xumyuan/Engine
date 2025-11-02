#pragma once

#include <platform/OpenGL/Framebuffers/Framebuffer.h>
#include <graphics/texture/Texture.h>

namespace engine{

	class GBuffer :public Framebuffer {
	public:
		GBuffer(unsigned int width, unsigned int height);
		~GBuffer();

		inline Texture* GetAlbedo() { return &m_GBufferRenderTargets[0]; }
		inline Texture* GetNormal() { return &m_GBufferRenderTargets[1]; }
		inline Texture* GetMaterialInfo() { return &m_GBufferRenderTargets[2]; }
	private:
		void init();
	private:
		// 0 RGBA8  ->       albedo.r     albedo.g        albedo.b             albedo's alpha       (can replaced with emission colour for emissive fragments)
		// 1 RGB32F ->       normal.x     normal.y        normal.z
		// 2 RGBA8  ->       metallic     roughness       ambientOcclusion     emissionIntensity
		std::array<Texture, 3>m_GBufferRenderTargets;
	};
}
