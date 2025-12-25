#pragma once
#include <graphics/camera/ICamera.h>
#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/Shader.h>
#include <scene/Scene3D.h>

namespace engine
{

	class SSAOPass : public RenderPass
	{
	public:
		SSAOPass(Scene3D* scene);
		virtual ~SSAOPass() override;

		PreLightingPassOutput executeSSAOPass(ICamera* camera, GBuffer* gbuffer);

		inline Texture* getSSAOTexture() { return m_SSAOBlurFramebuffer.getColorBufferTexture(); }

	private:
		void generateSampleKernel();
		void generateNoiseTexture();

		float lerp(float a, float b, float t);

	private:
		// SSAO Pass
		Framebuffer m_SSAOFramebuffer;
		Shader* m_SSAOShader;

		// Blur Pass
		Framebuffer m_SSAOBlurFramebuffer;
		Shader* m_SSAOBlurShader;

		// 采样核（半球内的随机采样点）
		std::vector<glm::vec3> m_SSAOKernel;
		static const int KERNEL_SIZE = 64;

		// 噪声纹理
		unsigned int m_NoiseTexture;
		static const int NOISE_SIZE = 4;

		// SSAO 参数
		float m_Radius = 0.5f;
		float m_Bias = 0.025f;
		float m_Power = 1.0f;
	};

}
