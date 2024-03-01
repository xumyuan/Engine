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

		// ����Ӧ�÷��ն�ͼ�ľ������������䣩
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

			// �Գ����Ĳ�׽���о��������洢�ڹ�̽�����������ͼ��
			m_LightProbeConvolutionFramebuffer.setColorAttachment(lightProbe->getIrradianceMap()->getCubemapID(), GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
			m_ActiveScene->getModelRenderer()->NDC_Cube.Draw(); // �����������ڶ���������ͼ���в��������ֻ��ʹ�� NDC �ռ��е�������
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

		// ��ʼ�� ������Ⱦ��̽����������ͼ
		m_CubemapCamera.setCenterPosition(probePosition);
		ShadowmapPass shadowPass(m_ActiveScene, &m_SceneCaptureShadowFramebuffer);
		LightingPass lightingPass(m_ActiveScene, &m_SceneCaptureLightingFramebuffer, false);

		// ��������Ⱦ��̽�����������ͼ
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