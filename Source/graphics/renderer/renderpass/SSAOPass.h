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

		enum class Quality {
			Low,
			Medium,
			High,
			Ultra
		};

		struct Settings {
			bool enabled = true;
			Algorithm algorithm = Algorithm::Crytek;
			float bilateralThreshold = 0.05f;

			struct {
				float radius = 0.5f;
				float bias = 0.025f;
				float power = 1.0f;
			} crytek;

			struct {
				Quality quality = Quality::Medium;
				float radius = 0.5f;
				float bias = 0.0005f;
				float intensity = 1.0f;
				float power = 2.0f;
				float minHorizonAngleDeg = 0.0f;
			} sao;
		};

		SSAOPass(const RenderScene& renderScene);
		virtual ~SSAOPass() override;

		PreLightingPassOutput executeSSAOPass(ICamera* camera, GeometryPassOutput& gBufferOutput);

		inline Texture* getSSAOTexture() { return m_SSAOBlurRT.getColorTexture(); }
		inline bool isEnabled() const { return m_Settings.enabled; }
		inline Settings& getSettings() { return m_Settings; }

	private:
		void initCrytekResources();
		void drawDebugUI();

		void buildDepthPyramid(const GeometryPassOutput& gBufferOutput);
		void renderCrytek(const GeometryPassOutput& gBufferOutput);
		void renderSAO(const glm::mat4& projection, const GeometryPassOutput& gBufferOutput);
		void renderBilateralBlur();
		UBOSAOParams makeSAOParams(const glm::mat4& projection) const;

	private:
		static constexpr int KERNEL_SIZE = 64;
		static constexpr int NOISE_SIZE = 4;

		Settings m_Settings;
		int m_DebugSection = 0;

		// 渲染目标与纹理资源
		RenderTarget m_SSAORT;
		RenderTarget m_BlurPingRT;
		RenderTarget m_SSAOBlurRT;
		RenderTarget m_DepthPyramidRT;
		Texture m_DepthPyramidTexture;
		int m_DepthPyramidLevels = 1;

		Texture m_NoiseTexture;
		std::vector<glm::vec3> m_SSAOKernel;

		// 着色器
		Shader* m_SSAOShader = nullptr;
		Shader* m_SAOShader = nullptr;
		Shader* m_SSAOBlurShader = nullptr;
		Shader* m_DepthPyramidShader = nullptr;
	};

}
