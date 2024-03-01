#include "pch.h"
#include "ProbePass.h"

#include <graphics/mesh/common/Cube.h>
#include <graphics/ibl/ProbeManager.h>
#include <graphics/renderer/renderpass/LightingPass.h>
#include <graphics/renderer/renderpass/ShadowmapPass.h>
#include <utils/loaders/ShaderLoader.h>

namespace engine {

	ProbePass::ProbePass(Scene3D* scene) : RenderPass(scene, RenderPassType::ProbePassType),
		m_SceneCaptureShadowFramebuffer(IBL_CAPTURE_RESOLUTION, IBL_CAPTURE_RESOLUTION), m_SceneCaptureLightingFramebuffer(IBL_CAPTURE_RESOLUTION, IBL_CAPTURE_RESOLUTION),
		m_LightProbeConvolutionFramebuffer(LIGHT_PROBE_RESOLUTION, LIGHT_PROBE_RESOLUTION), m_SceneCaptureCubemap(m_SceneCaptureSettings)
	{
		m_SceneCaptureShadowFramebuffer.addDepthAttachment(false).createFramebuffer();
		m_SceneCaptureLightingFramebuffer.addTexture2DColorAttachment(false).addDepthStencilRBO(false).createFramebuffer();
		m_LightProbeConvolutionFramebuffer.addTexture2DColorAttachment(false).addDepthStencilRBO(false).createFramebuffer();

		for (int i = 0; i < 6; i++) {
			m_SceneCaptureCubemap.generateCubemapFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, IBL_CAPTURE_RESOLUTION, IBL_CAPTURE_RESOLUTION, GL_RGBA16F, GL_RGB, nullptr);
		}

		m_ConvolutionShader = ShaderLoader::loadShader("src/shaders/lightprobe_convolution.vert", "src/shaders/lightprobe_convolution.frag");
	}

	ProbePass::~ProbePass() {}

	void ProbePass::pregenerateProbes() {
		glm::vec3 probePosition = glm::vec3(67.0f, 92.0f, 133.0f);
		generateLightProbe(probePosition);
		generateReflectionProbe(probePosition);
	}

	void ProbePass::generateLightProbe(glm::vec3& probePosition) {
		glm::vec2 probeResolution(LIGHT_PROBE_RESOLUTION, LIGHT_PROBE_RESOLUTION);
		LightProbe* lightProbe = new LightProbe(probePosition, probeResolution);
		lightProbe->generate();

		// Initialize step before rendering to the probe's cubemap
		m_CubemapCamera.setCenterPosition(probePosition);
		ShadowmapPass shadowPass(m_ActiveScene, &m_SceneCaptureShadowFramebuffer);
		LightingPass lightingPass(m_ActiveScene, &m_SceneCaptureLightingFramebuffer, false);

		// Render the scene to the probe's cubemap
		for (int i = 0; i < 6; i++) {
			// Setup the camera's view
			m_CubemapCamera.switchCameraToFace(i);

			// Shadow pass
			ShadowmapPassOutput shadowpassOutput = shadowPass.generateShadowmaps(&m_CubemapCamera);

			// Light pass
			m_SceneCaptureLightingFramebuffer.bind();
			m_SceneCaptureLightingFramebuffer.setColorAttachment(m_SceneCaptureCubemap.getCubemapID(), GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
			lightingPass.executeRenderPass(shadowpassOutput, &m_CubemapCamera);
			m_SceneCaptureLightingFramebuffer.setColorAttachment(0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
		}

		// 捕获并应用辐照度图的卷积（间接漫反射）
		m_GLCache->switchShader(m_ConvolutionShader);
		m_GLCache->setFaceCull(false);
		m_GLCache->setDepthTest(false);

		m_ConvolutionShader->setUniformMat4("projection", m_CubemapCamera.getProjectionMatrix());
		m_SceneCaptureCubemap.bind(0);
		m_ConvolutionShader->setUniform1i("sceneCaptureCubemap", 0);
		m_LightProbeConvolutionFramebuffer.bind();
		for (int i = 0; i < 6; i++) {
			// Setup the camera's view
			m_CubemapCamera.switchCameraToFace(i);
			m_ConvolutionShader->setUniformMat4("view", m_CubemapCamera.getViewMatrix());

			// 对场景的捕捉进行卷积并将其存储在光探针的立方体贴图中
			m_LightProbeConvolutionFramebuffer.setColorAttachment(lightProbe->getIrradianceMap()->getCubemapID(), GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
			m_ActiveScene->getModelRenderer()->NDC_Cube.Draw(); // 由于我们正在对立方体贴图进行采样，因此只需使用 NDC 空间中的立方体
			m_LightProbeConvolutionFramebuffer.setColorAttachment(0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
		}
		m_GLCache->setFaceCull(true);
		m_GLCache->setDepthTest(true);

		ProbeManager* probeManager = m_ActiveScene->getProbeManager();
		probeManager->addProbe(lightProbe);
	}

	void ProbePass::generateReflectionProbe(glm::vec3& probePosition) {
		glm::vec2 probeResolution(IBL_CAPTURE_RESOLUTION, IBL_CAPTURE_RESOLUTION);
		ReflectionProbe* reflectionProbe = new ReflectionProbe(probePosition, probeResolution, true);
		reflectionProbe->generate();

		// 初始化 用于渲染到探针立方体贴图
		m_CubemapCamera.setCenterPosition(probePosition);
		ShadowmapPass shadowPass(m_ActiveScene, &m_SceneCaptureShadowFramebuffer);
		LightingPass lightingPass(m_ActiveScene, &m_SceneCaptureLightingFramebuffer, false);

		// 将场景渲染到探针的立方体贴图
		for (int i = 0; i < 6; ++i) {
			m_CubemapCamera.switchCameraToFace(i);

			ShadowmapPassOutput shadowpassOutput = shadowPass.generateShadowmaps(&m_CubemapCamera);

			m_SceneCaptureLightingFramebuffer.bind();
			m_SceneCaptureLightingFramebuffer.setColorAttachment(m_SceneCaptureCubemap.getCubemapID(), GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
			lightingPass.executeRenderPass(shadowpassOutput, &m_CubemapCamera);
			m_SceneCaptureLightingFramebuffer.setColorAttachment(0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
		}

		ProbeManager* probeManager = m_ActiveScene->getProbeManager();
		probeManager->addProbe(reflectionProbe);
	}

}