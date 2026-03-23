#include "pch.h"
#include "ForwardProbePass.h"

#include <graphics/mesh/common/Cube.h>
#include <graphics/ibl/ProbeManager.h>
#include <graphics/renderer/renderpass/forward/ForwardLightingPass.h>
#include <graphics/renderer/renderpass/ShadowmapPass.h>
#include <graphics/UniformBufferManager.h>
#include <utils/loaders/ShaderLoader.h>

namespace engine {

	ForwardProbePass::ForwardProbePass(const RenderScene& renderScene) : RenderPass(renderScene, RenderPassType::ProbePassType),
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

		cmd().pushDebugGroup("Generate Cubemap Face");
		for (int i = 0; i < 6; i++) {
			m_SceneCaptureCubemap.generateCubemapFace(
				static_cast<uint8_t>(i), 
				IBL_CAPTURE_RESOLUTION, 
				IBL_CAPTURE_RESOLUTION, 
				ChannelLayout::RGBA, nullptr);
		}
		cmd().popDebugGroup();

		m_ConvolutionShader = ShaderLoader::loadShader("Shaders/lightprobe_convolution.glsl");

		m_ImportanceSamplingShader = ShaderLoader::loadShader("Shaders/reflectionprobe_importance_sampling.glsl");
	}

	ForwardProbePass::~ForwardProbePass() {}

	void ForwardProbePass::pregenerateProbes() {
		cmd().pushDebugGroup("GenerateBRDFLUT");
		generateBRDFLUT();
		cmd().popDebugGroup();
		glm::vec3 probePosition = glm::vec3(112.3f, 139.4f, 97.2f);
		cmd().pushDebugGroup("GenerateLightProbe");
		generateLightProbe(probePosition);
		cmd().popDebugGroup();
		cmd().pushDebugGroup("GenerateReflectionProbe");
		generateReflectionProbe(probePosition);
		cmd().popDebugGroup();
	}

	void ForwardProbePass::generateBRDFLUT() {
		Shader* brdfIntegrationShader = ShaderLoader::loadShader("Shaders/prebrdf.glsl");

		// brdf 的纹理设置
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

		// 设置 LUT 的渲染目标
		RenderTarget brdfRT(BRDF_LUT_RESOLUTION, BRDF_LUT_RESOLUTION);
		brdfRT.addColorTexture(rhi::TextureFormat::RG16F).build();

		// 通过命令缓冲录制 beginRenderPass
		rhi::RenderPassParams params;
		params.viewport = { 0, 0, BRDF_LUT_RESOLUTION, BRDF_LUT_RESOLUTION };
		params.clearColorFlag = true;
		params.clearDepthFlag = false;
		params.clearStencilFlag = false;
		cmd().beginRenderPass(brdfRT.getHandle(), params);

		// 动态附件管理
		cmd().setRenderTargetColorAttachment(brdfRT.getHandle(), 0, brdfLUT->getRHIHandle());

		rhi::PipelineState pipeline;
		pipeline.program = brdfIntegrationShader->getProgramHandle();
		pipeline.depthTest = false;
		pipeline.cullMode = rhi::CullMode::Back;
		cmd().bindPipeline(pipeline);

		ModelRenderer::drawNdcPlane();

		// 恢复原始附件
		cmd().setRenderTargetColorAttachment(brdfRT.getHandle(), 0, rhi::TextureHandle());
		cmd().endRenderPass();

		// 恢复深度测试
		rhi::PipelineState restorePipeline;
		restorePipeline.depthTest = true;
		restorePipeline.cullMode = rhi::CullMode::Back;
		cmd().bindPipeline(restorePipeline);

		ReflectionProbe::setBRDFLUT(brdfLUT);
	}

	void ForwardProbePass::generateLightProbe(glm::vec3& probePosition) {
		glm::vec2 probeResolution(LIGHT_PROBE_RESOLUTION, LIGHT_PROBE_RESOLUTION);
		LightProbe* lightProbe = new LightProbe(probePosition, probeResolution);
		lightProbe->generate();

		// Initialize step before rendering to the probe's cubemap
		m_CubemapCamera.setCenterPosition(probePosition);
		ShadowmapPass shadowPass(m_RenderScene, &m_SceneCaptureShadowRT);
		ForwardLightingPass lightingPass(m_RenderScene, &m_SceneCaptureLightingRT);

		// Render the scene to the probe's cubemap
		for (int i = 0; i < 6; i++) {
			cmd().pushDebugGroup(("Lighting Probe Cubemap[" + std::to_string(i) + "]").c_str());
			m_CubemapCamera.switchCameraToFace(i);

			cmd().pushDebugGroup("ShadowmapPass");
			ShadowmapPassOutput shadowpassOutput = shadowPass.generateShadowmaps(&m_CubemapCamera);
			cmd().popDebugGroup();

			// 先将 cubemap 面挂到 lighting RT 的颜色附件
			cmd().setRenderTargetColorAttachment(m_SceneCaptureLightingRT.getHandle(), 0,
				m_SceneCaptureCubemap.getRHIHandle(), 0, static_cast<uint8_t>(i));

			cmd().pushDebugGroup("LightingPass");
			lightingPass.executeRenderPass(shadowpassOutput, &m_CubemapCamera, false);
			cmd().popDebugGroup();

			// 恢复原始颜色附件
			cmd().setRenderTargetColorAttachment(m_SceneCaptureLightingRT.getHandle(), 0,
				m_SceneCaptureLightingRT.getColorTexture()->getRHIHandle());
			cmd().popDebugGroup();
		}

		// 捕获并应用辐照度图的卷积（间接漫反射）
		rhi::PipelineState convPipeline;
		convPipeline.program = m_ConvolutionShader->getProgramHandle();
		convPipeline.cullMode = rhi::CullMode::None;
		convPipeline.depthTest = false;
		cmd().bindPipeline(convPipeline);

		m_SceneCaptureCubemap.bind(0);
		m_ConvolutionShader->setUniform("sceneCaptureCubemap", 0);

		// PerFrame UBO 更新 projection
		if (auto* uboMgr = getUBOManager()) {
			uboMgr->updatePerFrame(glm::mat4(1.0f), m_CubemapCamera.getProjectionMatrix(), glm::vec3(0.0f));
			uboMgr->bindPerFrame();
		}

		rhi::RenderPassParams convParams;
		convParams.viewport = { 0, 0, m_LightProbeConvolutionRT.getWidth(), m_LightProbeConvolutionRT.getHeight() };
		convParams.clearColorFlag = false;
		convParams.clearDepthFlag = false;
		convParams.clearStencilFlag = false;
		cmd().beginRenderPass(m_LightProbeConvolutionRT.getHandle(), convParams);

		{
			cmd().pushDebugGroup("Convolution");
			for (int i = 0; i < 6; i++) {
				m_CubemapCamera.switchCameraToFace(i);
				// 通过 PerFrame UBO 更新 view
				if (auto* uboMgr = getUBOManager()) {
					uboMgr->updatePerFrame(m_CubemapCamera.getViewMatrix(), m_CubemapCamera.getProjectionMatrix(), glm::vec3(0.0f));
					uboMgr->bindPerFrame();
				}

				cmd().setRenderTargetColorAttachment(m_LightProbeConvolutionRT.getHandle(), 0,
					lightProbe->getIrradianceMap()->getRHIHandle(), 0, static_cast<uint8_t>(i));
				ModelRenderer::drawNdcCube();
			}
			// 恢复原始附件
			cmd().setRenderTargetColorAttachment(m_LightProbeConvolutionRT.getHandle(), 0,
				m_LightProbeConvolutionRT.getColorTexture()->getRHIHandle());
			cmd().popDebugGroup();
		}
		cmd().endRenderPass();

		// 恢复状态
		rhi::PipelineState restorePipeline;
		restorePipeline.cullMode = rhi::CullMode::Back;
		restorePipeline.depthTest = true;
		cmd().bindPipeline(restorePipeline);

		ProbeManager* probeManager = m_RenderScene.probeManager;
		probeManager->addProbe(lightProbe);
	}

	void ForwardProbePass::generateReflectionProbe(glm::vec3& probePosition) {
		glm::vec2 probeResolution(REFLECTION_PROBE_RESOLUTION, REFLECTION_PROBE_RESOLUTION);
		ReflectionProbe* reflectionProbe = new ReflectionProbe(probePosition, probeResolution, true);
		reflectionProbe->generate();

		m_CubemapCamera.setCenterPosition(probePosition);
		ShadowmapPass shadowPass(m_RenderScene, &m_SceneCaptureShadowRT);
		ForwardLightingPass lightingPass(m_RenderScene, &m_SceneCaptureLightingRT);

		// 将场景渲染到探针的立方体贴图
		for (int i = 0; i < 6; ++i) {
			cmd().pushDebugGroup(("Reflection Probe[" + std::to_string(i) + "]").c_str());
			m_CubemapCamera.switchCameraToFace(i);
			cmd().pushDebugGroup("ShadowmapPass");
			ShadowmapPassOutput shadowpassOutput = shadowPass.generateShadowmaps(&m_CubemapCamera);
			cmd().popDebugGroup();

			// 先将 cubemap 面挂到 lighting RT 的颜色附件
			cmd().setRenderTargetColorAttachment(m_SceneCaptureLightingRT.getHandle(), 0,
				m_SceneCaptureCubemap.getRHIHandle(), 0, static_cast<uint8_t>(i));

			cmd().pushDebugGroup("LightingPass");
			lightingPass.executeRenderPass(shadowpassOutput, &m_CubemapCamera, false);
			cmd().popDebugGroup();

			// 恢复原始颜色附件
			cmd().setRenderTargetColorAttachment(m_SceneCaptureLightingRT.getHandle(), 0,
				m_SceneCaptureLightingRT.getColorTexture()->getRHIHandle());
			cmd().popDebugGroup();
		}

		// 对代表增加的粗糙度级别的立方体贴图 mip 进行重要性采样
		rhi::PipelineState samplePipeline;
		samplePipeline.program = m_ImportanceSamplingShader->getProgramHandle();
		samplePipeline.cullMode = rhi::CullMode::None;
		samplePipeline.depthTest = false;
		cmd().bindPipeline(samplePipeline);

		m_SceneCaptureCubemap.bind(0);
		m_ImportanceSamplingShader->setUniform("sceneCaptureCubemap", 0);

		// PerFrame UBO 更新
		if (auto* uboMgr = getUBOManager()) {
			uboMgr->updatePerFrame(glm::mat4(1.0f), m_CubemapCamera.getProjectionMatrix(), glm::vec3(0.0f));
			uboMgr->bindPerFrame();
		}

		rhi::RenderPassParams sampleParams;
		sampleParams.clearColorFlag = false;
		sampleParams.clearDepthFlag = false;
		sampleParams.clearStencilFlag = false;
		cmd().beginRenderPass(m_ReflectionProbeSamplingRT.getHandle(), sampleParams);

		cmd().pushDebugGroup("Generate mip");
		{
			for (int mip = 0; mip < REFLECTION_PROBE_MIP_COUNT; mip++) {
				unsigned int mipWidth = m_ReflectionProbeSamplingRT.getWidth() >> mip;
				unsigned int mipHeight = m_ReflectionProbeSamplingRT.getHeight() >> mip;

				cmd().setViewport(0, 0, mipWidth, mipHeight);

				float mipRoughnessLevel = (float)mip / (float)(REFLECTION_PROBE_MIP_COUNT - 1);
				// ProbeParams UBO
				if (auto* uboMgr = getUBOManager()) {
					UBOProbeParams probeParams{};
					probeParams.roughness = mipRoughnessLevel;
					uboMgr->updateProbeParams(probeParams);
					uboMgr->bindCustom(sizeof(UBOProbeParams));
				}
				for (int i = 0; i < 6; i++) {
					m_CubemapCamera.switchCameraToFace(i);
					// 通过 PerFrame UBO 更新 view
					if (auto* uboMgr = getUBOManager()) {
						uboMgr->updatePerFrame(m_CubemapCamera.getViewMatrix(), m_CubemapCamera.getProjectionMatrix(), glm::vec3(0.0f));
						uboMgr->bindPerFrame();
					}
					cmd().setRenderTargetColorAttachment(m_ReflectionProbeSamplingRT.getHandle(), 0,
						reflectionProbe->getPrefilterMap()->getRHIHandle(),
						static_cast<uint8_t>(mip), static_cast<uint8_t>(i));
					ModelRenderer::drawNdcCube();
				}
			}
			// 恢复原始附件
			cmd().setRenderTargetColorAttachment(m_ReflectionProbeSamplingRT.getHandle(), 0,
				m_ReflectionProbeSamplingRT.getColorTexture()->getRHIHandle());
		}
		cmd().popDebugGroup();
		cmd().endRenderPass();

		// 恢复状态
		rhi::PipelineState restorePipeline;
		restorePipeline.cullMode = rhi::CullMode::Back;
		restorePipeline.depthTest = true;
		cmd().bindPipeline(restorePipeline);

		ProbeManager* probeManager = m_RenderScene.probeManager;
		probeManager->addProbe(reflectionProbe);
	}

}
