#include "pch.h"
#include "ForwardProbePass.h"

#include <graphics/mesh/common/Cube.h>
#include <graphics/ibl/ProbeManager.h>
#include <graphics/renderer/renderpass/forward/ForwardLightingPass.h>
#include <graphics/renderer/renderpass/ShadowmapPass.h>
#include <utils/loaders/ShaderLoader.h>

namespace engine {

	ForwardProbePass::ForwardProbePass(Scene3D* scene) : RenderPass(scene, RenderPassType::ProbePassType),
		m_SceneCaptureShadowRT(IBL_CAPTURE_RESOLUTION, IBL_CAPTURE_RESOLUTION),
		m_SceneCaptureLightingRT(IBL_CAPTURE_RESOLUTION, IBL_CAPTURE_RESOLUTION),
		m_LightProbeConvolutionRT(LIGHT_PROBE_RESOLUTION, LIGHT_PROBE_RESOLUTION),
		m_ReflectionProbeSamplingRT(REFLECTION_PROBE_RESOLUTION, REFLECTION_PROBE_RESOLUTION),
		m_SceneCaptureCubemap(m_SceneCaptureSettings)
	{
		m_SceneCaptureSettings.format = rhi::TextureFormat::RGBA16F;
		m_SceneCaptureSettings.formatExplicitlySet = true;
		m_SceneCaptureCubemap.setCubemapSettings(m_SceneCaptureSettings);

		m_SceneCaptureShadowRT.addDepthStencilTexture(DepthStencilFormat::DepthOnly).build();
		m_SceneCaptureLightingRT.addColorTexture(rhi::TextureFormat::RGBA16F)
			.addDepthStencilTexture(DepthStencilFormat::DepthOnly, false).build();

		m_LightProbeConvolutionRT.addColorTexture(rhi::TextureFormat::RGBA16F)
			.build();
		m_ReflectionProbeSamplingRT.addColorTexture(rhi::TextureFormat::RGBA16F)
			.build();

		BEGIN_EVENT("Generate Cubemap Face");
		for (int i = 0; i < 6; i++) {
			m_SceneCaptureCubemap.generateCubemapFace(
				static_cast<uint8_t>(i), 
				IBL_CAPTURE_RESOLUTION, 
				IBL_CAPTURE_RESOLUTION, 
				ChannelLayout::RGBA, nullptr);
		}
		END_EVENT();

		m_ConvolutionShader = ShaderLoader::loadShader("Shaders/lightprobe_convolution.glsl");

		m_ImportanceSamplingShader = ShaderLoader::loadShader("Shaders/reflectionprobe_importance_sampling.glsl");
	}

	ForwardProbePass::~ForwardProbePass() {}

	void ForwardProbePass::pregenerateProbes() {
		BEGIN_EVENT("GenerateBRDFLUT");
		generateBRDFLUT();
		END_EVENT();
		glm::vec3 probePosition = glm::vec3(112.3f, 139.4f, 97.2f);
		BEGIN_EVENT("GenerateLightProbe");
		generateLightProbe(probePosition);
		END_EVENT();
		BEGIN_EVENT("GenerateReflectionProbe");
		generateReflectionProbe(probePosition);
		END_EVENT();
	}

	void ForwardProbePass::generateBRDFLUT() {
		Shader* brdfIntegrationShader = ShaderLoader::loadShader("Shaders/prebrdf.glsl");

		// brdf 的纹理设置 —— 必须使用浮点格式以保留 BRDF 积分精度
		TextureSettings textureSettings;
		textureSettings.format = rhi::TextureFormat::RG16F;
		textureSettings.formatExplicitlySet = true;
		textureSettings.wrapS = rhi::WrapMode::ClampToEdge;
		textureSettings.wrapT = rhi::WrapMode::ClampToEdge;
		textureSettings.minFilter = rhi::FilterMode::Linear;
		textureSettings.magFilter = rhi::FilterMode::Linear;
		textureSettings.anisotropy = 1.0f;
		textureSettings.HasMips = false;

		Texture* brdfLUT = new Texture(textureSettings);
		brdfLUT->generate2DTexture(BRDF_LUT_RESOLUTION, BRDF_LUT_RESOLUTION, ChannelLayout::RG);

		// 设置 LUT 的渲染目标（颜色格式与 LUT 一致）
		RenderTarget brdfRT(BRDF_LUT_RESOLUTION, BRDF_LUT_RESOLUTION);
		brdfRT.addColorTexture(rhi::TextureFormat::RG16F).build();

		// 临时将 brdfLUT 挂到 RT 的颜色附件
		auto* device = getRHIDevice();
		rhi::RenderPassParams params;
		params.viewport = { 0, 0, BRDF_LUT_RESOLUTION, BRDF_LUT_RESOLUTION };
		params.clearColorFlag = true;
		params.clearDepthFlag = false;
		brdfRT.beginPass(params);
		brdfRT.setColorAttachment(0, brdfLUT->getRHIHandle());

		m_GLCache->switchShader(brdfIntegrationShader);
		m_GLCache->setDepthTest(false);

		ModelRenderer::drawNdcPlane();

		// 恢复原始附件
		brdfRT.setColorAttachment(0, rhi::TextureHandle());
		brdfRT.endPass();

		m_GLCache->setDepthTest(true);

		ReflectionProbe::setBRDFLUT(brdfLUT);
	}

	void ForwardProbePass::generateLightProbe(glm::vec3& probePosition) {
		glm::vec2 probeResolution(LIGHT_PROBE_RESOLUTION, LIGHT_PROBE_RESOLUTION);
		LightProbe* lightProbe = new LightProbe(probePosition, probeResolution);
		lightProbe->generate();

		// Initialize step before rendering to the probe's cubemap
		m_CubemapCamera.setCenterPosition(probePosition);
		ShadowmapPass shadowPass(m_ActiveScene, &m_SceneCaptureShadowRT);
		ForwardLightingPass lightingPass(m_ActiveScene, &m_SceneCaptureLightingRT);

		// Render the scene to the probe's cubemap
		for (int i = 0; i < 6; i++) {
			BEGIN_EVENT("Lighting Probe Cubemap[" + std::to_string(i) + "]");
			m_CubemapCamera.switchCameraToFace(i);

			BEGIN_EVENT("ShadowmapPass");
			ShadowmapPassOutput shadowpassOutput = shadowPass.generateShadowmaps(&m_CubemapCamera);
			END_EVENT();

			// 先将 cubemap 面挂到 lighting RT 的颜色附件（FBO 状态持久化）
			// 然后让 lighting pass 的 beginPass 绑定同一 FBO 并清除
			auto* device = getRHIDevice();
			device->setRenderTargetColorAttachment(m_SceneCaptureLightingRT.getHandle(), 0,
				m_SceneCaptureCubemap.getRHIHandle(), 0, static_cast<uint8_t>(i));

			BEGIN_EVENT("LightingPass");
			lightingPass.executeRenderPass(shadowpassOutput, &m_CubemapCamera, false);
			END_EVENT();

			// 恢复原始颜色附件
			device->setRenderTargetColorAttachment(m_SceneCaptureLightingRT.getHandle(), 0,
				m_SceneCaptureLightingRT.getColorTexture()->getRHIHandle());
			END_EVENT();
		}

		// 捕获并应用辐照度图的卷积（间接漫反射）
		m_GLCache->switchShader(m_ConvolutionShader);
		m_GLCache->setFaceCull(false);
		m_GLCache->setDepthTest(false);

		m_ConvolutionShader->setUniform("projection", m_CubemapCamera.getProjectionMatrix());
		m_SceneCaptureCubemap.bind(0);
		m_ConvolutionShader->setUniform("sceneCaptureCubemap", 0);

		rhi::RenderPassParams convParams;
		convParams.viewport = { 0, 0, m_LightProbeConvolutionRT.getWidth(), m_LightProbeConvolutionRT.getHeight() };
		convParams.clearColorFlag = false;
		convParams.clearDepthFlag = false;
		m_LightProbeConvolutionRT.beginPass(convParams);

		{
			auto* device = getRHIDevice();
			BEGIN_EVENT("Convolution");
			for (int i = 0; i < 6; i++) {
				m_CubemapCamera.switchCameraToFace(i);
				m_ConvolutionShader->setUniform("view", m_CubemapCamera.getViewMatrix());

				device->setRenderTargetColorAttachment(m_LightProbeConvolutionRT.getHandle(), 0,
					lightProbe->getIrradianceMap()->getRHIHandle(), 0, static_cast<uint8_t>(i));
				ModelRenderer::drawNdcCube();
			}
			// 恢复原始附件
			device->setRenderTargetColorAttachment(m_LightProbeConvolutionRT.getHandle(), 0,
				m_LightProbeConvolutionRT.getColorTexture()->getRHIHandle());
			END_EVENT();
		}
		m_LightProbeConvolutionRT.endPass();

		m_GLCache->setFaceCull(true);
		m_GLCache->setDepthTest(true);

		ProbeManager* probeManager = m_ActiveScene->getProbeManager();
		probeManager->addProbe(lightProbe);
	}

	void ForwardProbePass::generateReflectionProbe(glm::vec3& probePosition) {
		glm::vec2 probeResolution(REFLECTION_PROBE_RESOLUTION, REFLECTION_PROBE_RESOLUTION);
		ReflectionProbe* reflectionProbe = new ReflectionProbe(probePosition, probeResolution, true);
		reflectionProbe->generate();

		m_CubemapCamera.setCenterPosition(probePosition);
		ShadowmapPass shadowPass(m_ActiveScene, &m_SceneCaptureShadowRT);
		ForwardLightingPass lightingPass(m_ActiveScene, &m_SceneCaptureLightingRT);

		// 将场景渲染到探针的立方体贴图
		for (int i = 0; i < 6; ++i) {
			BEGIN_EVENT("Reflection Probe[" + std::to_string(i) + "]");
			m_CubemapCamera.switchCameraToFace(i);
			BEGIN_EVENT("ShadowmapPass");
			ShadowmapPassOutput shadowpassOutput = shadowPass.generateShadowmaps(&m_CubemapCamera);
			END_EVENT();

			// 先将 cubemap 面挂到 lighting RT 的颜色附件
			auto* device = getRHIDevice();
			device->setRenderTargetColorAttachment(m_SceneCaptureLightingRT.getHandle(), 0,
				m_SceneCaptureCubemap.getRHIHandle(), 0, static_cast<uint8_t>(i));

			BEGIN_EVENT("LightingPass");
			lightingPass.executeRenderPass(shadowpassOutput, &m_CubemapCamera, false);
			END_EVENT();

			// 恢复原始颜色附件
			device->setRenderTargetColorAttachment(m_SceneCaptureLightingRT.getHandle(), 0,
				m_SceneCaptureLightingRT.getColorTexture()->getRHIHandle());
			END_EVENT();
		}

		// 对代表增加的粗糙度级别的立方体贴图 mip 进行重要性采样
		m_GLCache->switchShader(m_ImportanceSamplingShader);
		m_GLCache->setFaceCull(false);
		m_GLCache->setDepthTest(false);

		m_ImportanceSamplingShader->setUniform("projection", m_CubemapCamera.getProjectionMatrix());
		m_SceneCaptureCubemap.bind(0);
		m_ImportanceSamplingShader->setUniform("sceneCaptureCubemap", 0);

		rhi::RenderPassParams sampleParams;
		sampleParams.clearColorFlag = false;
		sampleParams.clearDepthFlag = false;
		m_ReflectionProbeSamplingRT.beginPass(sampleParams);

		BEGIN_EVENT("Generate mip");
		{
			auto* device = getRHIDevice();
			for (int mip = 0; mip < REFLECTION_PROBE_MIP_COUNT; mip++) {
				unsigned int mipWidth = m_ReflectionProbeSamplingRT.getWidth() >> mip;
				unsigned int mipHeight = m_ReflectionProbeSamplingRT.getHeight() >> mip;

				device->setViewport(0, 0, mipWidth, mipHeight);

				float mipRoughnessLevel = (float)mip / (float)(REFLECTION_PROBE_MIP_COUNT - 1);
				m_ImportanceSamplingShader->setUniform("roughness", mipRoughnessLevel);
				for (int i = 0; i < 6; i++) {
					m_CubemapCamera.switchCameraToFace(i);
					m_ImportanceSamplingShader->setUniform("view", m_CubemapCamera.getViewMatrix());
					device->setRenderTargetColorAttachment(m_ReflectionProbeSamplingRT.getHandle(), 0,
						reflectionProbe->getPrefilterMap()->getRHIHandle(),
						static_cast<uint8_t>(mip), static_cast<uint8_t>(i));
					ModelRenderer::drawNdcCube();
				}
			}
			// 恢复原始附件
			device->setRenderTargetColorAttachment(m_ReflectionProbeSamplingRT.getHandle(), 0,
				m_ReflectionProbeSamplingRT.getColorTexture()->getRHIHandle());
		}
		END_EVENT();
		m_ReflectionProbeSamplingRT.endPass();

		m_GLCache->setFaceCull(true);
		m_GLCache->setDepthTest(true);

		ProbeManager* probeManager = m_ActiveScene->getProbeManager();
		probeManager->addProbe(reflectionProbe);
	}

}
