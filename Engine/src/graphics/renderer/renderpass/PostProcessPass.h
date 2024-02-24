#pragma once

#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/Shader.h>
#include <scene/Scene3D.h>

namespace engine
{

	class PostProcessPass : public RenderPass
	{
	public:
		PostProcessPass(Scene3D* scene);
		virtual ~PostProcessPass() override;

		void executeRenderPass(Framebuffer* framebufferToProcess);

		inline void EnableBlur(bool choice) { m_Blur = choice; }
	private:
		Shader m_PostProcessShader;
		Quad m_NDC_Plane;
		Framebuffer m_ScreenRenderTarget; // 仅在启用多重采样时使​​用，以便它可以位块传输到非多重采样缓冲区

		// Post Processing Tweaks
		float m_GammaCorrection = 2.2f;
		bool m_Blur = false;
	};

}