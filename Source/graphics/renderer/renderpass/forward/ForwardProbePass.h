#pragma once

#include <graphics/camera/CubemapCamera.h>
#include <graphics/texture/Cubemap.h>
#include <graphics/renderer/renderpass/RenderPass.h>
#include <graphics/renderer/renderpass/ShadowmapPass.h>
#include <graphics/renderer/RenderTarget.h>
#include <graphics/Shader.h>

namespace engine {

	class ForwardProbePass : public RenderPass {
	public:
		ForwardProbePass(const RenderScene& renderScene);
		virtual ~ForwardProbePass() override;

		void pregenerateProbes();
		void generateBRDFLUT();
		void generateLightProbe(glm::vec3& probePosition);
		void generateReflectionProbe(glm::vec3& probePosition);
	private:
		RenderTarget m_SceneCaptureShadowRT,
			m_SceneCaptureLightingRT,
			m_LightProbeConvolutionRT,
			m_ReflectionProbeSamplingRT;
		CubemapCamera m_CubemapCamera;
		CubemapSettings m_SceneCaptureSettings;
		Cubemap m_SceneCaptureCubemap;

		Shader* m_ConvolutionShader, * m_ImportanceSamplingShader;
	};

}
