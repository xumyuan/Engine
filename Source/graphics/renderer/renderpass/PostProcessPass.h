#pragma once

#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/renderer/RenderTarget.h>
#include <graphics/Shader.h>
#include <scene/Scene3D.h>

namespace engine
{

	class PostProcessPass : public RenderPass
	{
	public:
		PostProcessPass(Scene3D* scene);
		virtual ~PostProcessPass() override;

		void executeRenderPass(LightingPassOutput& lightingOutput);

		void gammaCorrect(RenderTarget* target, Texture* hdrTexture);

		void fxaa(RenderTarget* target, Texture* texture);

	private:

		Quad m_NDC_Plane;
		RenderTarget m_ResolveRT;   // MSAA resolve 用

		RenderTarget m_FullRenderTarget;

		// 伽马矫正
		Shader* m_GammaCorrectShader;
		float m_GammaCorrection = 2.2f;
		float m_Exposure = 1.0f;
		RenderTarget m_GammaCorrectTarget;

		// fxaa
		bool m_FxaaEnabled = true;
		Shader* m_FxaaShader;

		// 处理结果
		Shader* m_PassthroughShader;
	};

}
