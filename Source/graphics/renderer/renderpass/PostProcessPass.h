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

		void gammaCorrect(Framebuffer* target, Texture* hdrTexture);

		void fxaa(Framebuffer* target, Texture* texture);

	private:

		Quad m_NDC_Plane;
		Framebuffer m_ScreenRenderTarget; // 仅在启用多重采样时使​​用，以便它可以位块传输到非多重采样缓冲区

		Framebuffer m_FullRenderTarget;

		// 伽马矫正
		Shader* m_GammaCorrectShader;
		float m_GammaCorrection = 2.2f;
		float m_Exposure = 1.0f;
		Framebuffer m_GammaCorrectTarget;

		// fxaa
		bool m_FxaaEnabled = true;
		Shader* m_FxaaShader;

		// 处理结果
		Shader* m_PassthroughShader;
	};

}