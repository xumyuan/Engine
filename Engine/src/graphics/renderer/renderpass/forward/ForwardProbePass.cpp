#include "pch.h"
#include "ForwardProbePass.h"

#include <graphics/mesh/common/Cube.h>
#include <graphics/ibl/ProbeManager.h>
#include <graphics/renderer/renderpass/forward/ForwardLightingPass.h>
#include <graphics/renderer/renderpass/ShadowmapPass.h>
#include <utils/loaders/ShaderLoader.h>

namespace engine {

	ForwardProbePass::ForwardProbePass(Scene3D* scene) : RenderPass(scene, RenderPassType::ProbePassType),
		m_SceneCaptureShadowFramebuffer(IBL_CAPTURE_RESOLUTION, IBL_CAPTURE_RESOLUTION, false),
		m_SceneCaptureLightingFramebuffer(IBL_CAPTURE_RESOLUTION, IBL_CAPTURE_RESOLUTION, false),
		m_LightProbeConvolutionFramebuffer(LIGHT_PROBE_RESOLUTION, LIGHT_PROBE_RESOLUTION, false), m_ReflectionProbeSamplingFramebuffer(REFLECTION_PROBE_RESOLUTION, REFLECTION_PROBE_RESOLUTION, false),
		m_SceneCaptureCubemap(m_SceneCaptureSettings)
	{
		m_SceneCaptureSettings.TextureFormat = GL_RGBA16F;
		m_SceneCaptureCubemap.setCubemapSettings(m_SceneCaptureSettings);

		m_SceneCaptureShadowFramebuffer.addDepthStencilTexture(NormalizedDepthOnly).createFramebuffer();
		m_SceneCaptureLightingFramebuffer.addColorTexture(FloatingPoint16)
			.addDepthStencilRBO(NormalizedDepthOnly).createFramebuffer();

		m_LightProbeConvolutionFramebuffer.addColorTexture(FloatingPoint16)
			.createFramebuffer();
		m_ReflectionProbeSamplingFramebuffer.addColorTexture(FloatingPoint16).
			createFramebuffer();


		for (int i = 0; i < 6; i++) {
			m_SceneCaptureCubemap.generateCubemapFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, IBL_CAPTURE_RESOLUTION, IBL_CAPTURE_RESOLUTION, GL_RGB, nullptr);
		}

		m_ConvolutionShader = ShaderLoader::loadShader("src/shaders/lightprobe_convolution.glsl");

		m_ImportanceSamplingShader = ShaderLoader::loadShader("src/shaders/reflectionprobe_importance_sampling.glsl");
	}

	ForwardProbePass::~ForwardProbePass() {}

	void ForwardProbePass::pregenerateProbes() {

		generateBRDFLUT();

		glm::vec3 probePosition = glm::vec3(67.0f, 92.0f, 133.0f);
		generateLightProbe(probePosition);
		generateReflectionProbe(probePosition);
	}

	void ForwardProbePass::generateBRDFLUT() {
		Shader* brdfIntegrationShader = ShaderLoader::loadShader("src/shaders/prebrdf.glsl");
		ModelRenderer* modelRenderer = m_ActiveScene->getModelRenderer();

		// brdf的纹理设置
		TextureSettings textureSettings;
		textureSettings.TextureWrapSMode = GL_CLAMP_TO_EDGE;
		textureSettings.TextureWrapTMode = GL_CLAMP_TO_EDGE;
		textureSettings.TextureMinificationFilterMode = GL_LINEAR;
		textureSettings.TextureMagnificationFilterMode = GL_LINEAR;
		textureSettings.TextureAnisotropyLevel = 1.0f;
		textureSettings.HasMips = false;

		Texture* brdfLUT = new Texture(textureSettings);
		brdfLUT->generate2DTexture(BRDF_LUT_RESOLUTION, BRDF_LUT_RESOLUTION, GL_RG);

		// 设置lut的帧缓冲区
		Framebuffer brdfBuffer(BRDF_LUT_RESOLUTION, BRDF_LUT_RESOLUTION, false);

		brdfBuffer.addColorTexture(Normalized8).createFramebuffer();
		brdfBuffer.bind();

		m_GLCache->switchShader(brdfIntegrationShader);
		m_GLCache->setDepthTest(false);

		glViewport(0, 0, BRDF_LUT_RESOLUTION, BRDF_LUT_RESOLUTION);
		brdfBuffer.setColorAttachment(brdfLUT->getTextureId(), GL_TEXTURE_2D);
		ModelRenderer::drawNdcPlane();
		brdfBuffer.setColorAttachment(0, GL_TEXTURE_2D);

		m_GLCache->setDepthTest(true);

		ReflectionProbe::setBRDFLUT(brdfLUT);
	}

	void ForwardProbePass::generateLightProbe(glm::vec3& probePosition) {
		glm::vec2 probeResolution(LIGHT_PROBE_RESOLUTION, LIGHT_PROBE_RESOLUTION);
		LightProbe* lightProbe = new LightProbe(probePosition, probeResolution);
		lightProbe->generate();

		// Initialize step before rendering to the probe's cubemap
		m_CubemapCamera.setCenterPosition(probePosition);
		ShadowmapPass shadowPass(m_ActiveScene, &m_SceneCaptureShadowFramebuffer);
		ForwardLightingPass lightingPass(m_ActiveScene, &m_SceneCaptureLightingFramebuffer);

		// Render the scene to the probe's cubemap
		for (int i = 0; i < 6; i++) {
			// Setup the camera's view
			m_CubemapCamera.switchCameraToFace(i);

			// Shadow pass
			ShadowmapPassOutput shadowpassOutput = shadowPass.generateShadowmaps(&m_CubemapCamera);

			// Light pass
			m_SceneCaptureLightingFramebuffer.bind();
			m_SceneCaptureLightingFramebuffer.setColorAttachment(m_SceneCaptureCubemap.getCubemapID(), GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
			lightingPass.executeRenderPass(shadowpassOutput, &m_CubemapCamera, false);
			m_SceneCaptureLightingFramebuffer.setColorAttachment(0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
		}

		// 捕获并应用辐照度图的卷积（间接漫反射）
		m_GLCache->switchShader(m_ConvolutionShader);
		m_GLCache->setFaceCull(false);
		m_GLCache->setDepthTest(false); // Important cause the depth buffer isn't cleared so it zero depth

		m_ConvolutionShader->setUniform("projection", m_CubemapCamera.getProjectionMatrix());
		m_SceneCaptureCubemap.bind(0);
		m_ConvolutionShader->setUniform("sceneCaptureCubemap", 0);

		m_LightProbeConvolutionFramebuffer.bind();

		auto* skybox = m_ActiveScene->getSkybox()->getSkyboxCubemap();
		skybox->bind(0);
		glViewport(0, 0, m_LightProbeConvolutionFramebuffer.getWidth(), m_LightProbeConvolutionFramebuffer.getHeight());
		for (int i = 0; i < 6; i++) {
			// Setup the camera's view
			m_CubemapCamera.switchCameraToFace(i);
			m_ConvolutionShader->setUniform("view", m_CubemapCamera.getViewMatrix());

			// 对场景的捕捉进行卷积并将其存储在光探针的立方体贴图中
			m_LightProbeConvolutionFramebuffer.setColorAttachment(lightProbe->getIrradianceMap()->getCubemapID(), GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
			// 由于我们正在对立方体贴图进行采样，因此只需使用 NDC 空间中的立方体
			ModelRenderer::drawNdcCube();
			m_LightProbeConvolutionFramebuffer.setColorAttachment(0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
		}
		m_GLCache->setFaceCull(true);
		m_GLCache->setDepthTest(true);

		ProbeManager* probeManager = m_ActiveScene->getProbeManager();
		probeManager->addProbe(lightProbe);
	}

	void ForwardProbePass::generateReflectionProbe(glm::vec3& probePosition) {
		glm::vec2 probeResolution(REFLECTION_PROBE_RESOLUTION, REFLECTION_PROBE_RESOLUTION);
		ReflectionProbe* reflectionProbe = new ReflectionProbe(probePosition, probeResolution, true);
		reflectionProbe->generate();

		// 初始化 用于渲染到探针立方体贴图
		m_CubemapCamera.setCenterPosition(probePosition);
		ShadowmapPass shadowPass(m_ActiveScene, &m_SceneCaptureShadowFramebuffer);
		ForwardLightingPass lightingPass(m_ActiveScene, &m_SceneCaptureLightingFramebuffer);

		// 将场景渲染到探针的立方体贴图
		for (int i = 0; i < 6; ++i) {
			m_CubemapCamera.switchCameraToFace(i);

			ShadowmapPassOutput shadowpassOutput = shadowPass.generateShadowmaps(&m_CubemapCamera);

			m_SceneCaptureLightingFramebuffer.bind();
			m_SceneCaptureLightingFramebuffer.setColorAttachment(m_SceneCaptureCubemap.getCubemapID(), GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
			lightingPass.executeRenderPass(shadowpassOutput, &m_CubemapCamera, false);
			m_SceneCaptureLightingFramebuffer.setColorAttachment(0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
		}

		// 对代表增加的粗糙度级别的立方体贴图 mip 进行捕获并执行重要性采样
		m_GLCache->switchShader(m_ImportanceSamplingShader);
		m_GLCache->setFaceCull(false);
		m_GLCache->setDepthTest(false); // Important cause the depth buffer isn't cleared so it zero depth

		m_ImportanceSamplingShader->setUniform("projection", m_CubemapCamera.getProjectionMatrix());
		m_SceneCaptureCubemap.bind(0);
		m_ImportanceSamplingShader->setUniform("sceneCaptureCubemap", 0);

		m_ReflectionProbeSamplingFramebuffer.bind();
		for (int mip = 0; mip < REFLECTION_PROBE_MIP_COUNT; mip++) {
			// Calculate the size of this mip and resize
			unsigned int mipWidth = m_ReflectionProbeSamplingFramebuffer.getWidth() >> mip;
			unsigned int mipHeight = m_ReflectionProbeSamplingFramebuffer.getHeight() >> mip;

			/*	glBindRenderbuffer(GL_RENDERBUFFER, m_ReflectionProbeSamplingFramebuffer.getDepthStencilRBO());
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);*/
			glViewport(0, 0, mipWidth, mipHeight);

			float mipRoughnessLevel = (float)mip / (float)(REFLECTION_PROBE_MIP_COUNT - 1);
			m_ImportanceSamplingShader->setUniform("roughness", mipRoughnessLevel);
			for (int i = 0; i < 6; i++) {
				// Setup the camera's view
				m_CubemapCamera.switchCameraToFace(i);
				m_ImportanceSamplingShader->setUniform("view", m_CubemapCamera.getViewMatrix());
				// 对场景捕获的重要性进行采样并将其存储在反射探针的立方体贴图中
				m_ReflectionProbeSamplingFramebuffer.setColorAttachment(reflectionProbe->getPrefilterMap()->getCubemapID(), GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip);
				// Since we are sampling a cubemap, just use a cube
				ModelRenderer::drawNdcCube();
				m_ReflectionProbeSamplingFramebuffer.setColorAttachment(0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
			}
		}


		ProbeManager* probeManager = m_ActiveScene->getProbeManager();
		probeManager->addProbe(reflectionProbe);
	}

}
