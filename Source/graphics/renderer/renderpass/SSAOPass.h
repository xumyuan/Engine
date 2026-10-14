#pragma once
#include <graphics/camera/ICamera.h>
#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/renderer/RenderTarget.h>
#include <graphics/Shader.h>
#include <graphics/UniformBufferData.h>

namespace engine
{

	class SSAOPass : public RenderPass
	{
	public:
		enum class Algorithm {
			Crytek,
			SAO,
			Count
		};

		enum class SAOQuality {
			Low,
			Medium,
			High,
			Ultra
		};

		SSAOPass(const RenderScene& renderScene);
		virtual ~SSAOPass() override;

		PreLightingPassOutput executeSSAOPass(ICamera* camera, GeometryPassOutput& gBufferOutput);

		inline Texture* getSSAOTexture() { return m_SSAOBlurRT.getColorTexture(); }
		inline bool isEnabled() const { return m_Enabled; }

	private:
		void generateSampleKernel();
		void generateNoiseTexture();
		void drawAlgorithmControls();
		const char* algorithmName() const;
		Shader* algorithmShader() const;
		void uploadAlgorithmParams(const glm::mat4& projection);
		void bindAlgorithmTextures();
		UBOSAOParams makeSAOParams(const glm::mat4& projection) const;

		float lerp(float a, float b, float t);

	private:
		// SSAO Pass
		RenderTarget m_SSAORT;
		Shader* m_SSAOShader;
		Shader* m_SAOShader;

		// Blur Pass
		RenderTarget m_SSAOBlurRT;
		Shader* m_SSAOBlurShader;

		// 采样核（半球内的随机采样点）
		std::vector<glm::vec3> m_SSAOKernel;
		static const int KERNEL_SIZE = 64;

		// 噪声纹理
		Texture m_NoiseTexture;
		static const int NOISE_SIZE = 4;

		// 共用参数。Bias / Power 两套分开存，切换算法时不互相覆盖。
		int m_DebugSection = 0;
		bool m_Enabled = true;
		Algorithm m_Algorithm = Algorithm::Crytek;
		float m_Radius = 0.5f;
		float m_Bias = 0.025f;
		float m_Power = 1.0f;

		SAOQuality m_SAOQuality = SAOQuality::Medium;
		float m_SAOBias = 0.0005f;
		float m_SAOIntensity = 1.0f;
		float m_SAOPower = 2.0f;
		float m_SAOMinHorizonAngleDeg = 0.0f;
	};

}
