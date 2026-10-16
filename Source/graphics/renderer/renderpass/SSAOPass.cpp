#include "pch.h"
#include "SSAOPass.h"

#include <graphics/Window.h>
#include <graphics/renderer/ModelRenderer.h>
#include <graphics/UniformBufferManager.h>
#include <graphics/TextureBindings.h>
#include <ui/DebugPane.h>
#include <utils/loaders/ShaderLoader.h>

#include <imgui.h>

namespace engine
{

	namespace {

	constexpr const char* kAlgorithmNames[] = {
		"Crytek",
		"SAO",
	};
	static_assert(sizeof(kAlgorithmNames) / sizeof(kAlgorithmNames[0])
		== static_cast<size_t>(SSAOPass::Algorithm::Count),
		"Algorithm combo labels must match SSAOPass::Algorithm");

	}

	SSAOPass::SSAOPass(const RenderScene& renderScene)
		: RenderPass(renderScene, RenderPassType::SSAOPassType),
		m_SSAORT(Window::getWidth(), Window::getHeight()),
		m_BlurPingRT(Window::getWidth(), Window::getHeight()),
		m_SSAOBlurRT(Window::getWidth(), Window::getHeight()),
		m_DepthPyramidRT(Window::getWidth(), Window::getHeight())
	{
		// 加载着色器
		m_SSAOShader = ShaderLoader::loadShader("Source/shaders/post_process/ssao.glsl");
		m_SAOShader = ShaderLoader::loadShader("Source/shaders/post_process/sao.glsl");
		m_SSAOBlurShader = ShaderLoader::loadShader("Source/shaders/post_process/ssao_blur.glsl");
		m_DepthPyramidShader = ShaderLoader::loadShader("Source/shaders/post_process/depth_pyramid_minify.glsl");

		// 创建 SSAO RenderTarget（RGB8：R=可见度，GB=打包的 bilateral key）与模糊乒乓目标
		m_SSAORT.addColorTexture(rhi::TextureFormat::RGB8).build();
		m_BlurPingRT.addColorTexture(rhi::TextureFormat::RGB8).build();

		// 创建模糊后的 RenderTarget
		m_SSAOBlurRT.addColorTexture(rhi::TextureFormat::R8).build();

		// 深度塔：单张 R32F 多 mip 纹理存线性视图距离（1080p -> 6 级）
		const int width = Window::getWidth();
		const int height = Window::getHeight();
		m_DepthPyramidLevels = std::min(8, std::max(1,
			static_cast<int>(std::floor(std::log2(std::max(width, height)))) - 4));

		TextureSettings pyramidSettings;
		pyramidSettings.format = rhi::TextureFormat::R32F;
		pyramidSettings.formatExplicitlySet = true;
		pyramidSettings.minFilter = rhi::FilterMode::NearestMipmapNearest;
		pyramidSettings.magFilter = rhi::FilterMode::Nearest;
		pyramidSettings.wrapS = rhi::WrapMode::ClampToEdge;
		pyramidSettings.wrapT = rhi::WrapMode::ClampToEdge;
		pyramidSettings.anisotropy = 1.0f;
		pyramidSettings.HasMips = true;
		m_DepthPyramidTexture.setTextureSettings(pyramidSettings);
		m_DepthPyramidTexture.generate2DMipmappedTexture(
			static_cast<unsigned int>(width), static_cast<unsigned int>(height),
			ChannelLayout::R, static_cast<unsigned int>(m_DepthPyramidLevels));
		m_DepthPyramidRT.addExternalColorTexture(&m_DepthPyramidTexture).build();

		// 生成采样核和噪声纹理
		generateSampleKernel();
		generateNoiseTexture();

		m_DebugSection = DebugPane::addSection("SSAO", [this]() {
			ImGui::Checkbox("Enabled", &m_Enabled);

			int algorithm = static_cast<int>(m_Algorithm);
			if (ImGui::Combo("Algorithm", &algorithm, kAlgorithmNames, static_cast<int>(Algorithm::Count)))
				m_Algorithm = static_cast<Algorithm>(algorithm);

			ImGui::SliderFloat("Radius", &m_Radius, 0.05f, 2.0f, "%.3f");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("View-space sample radius");

			ImGui::SliderFloat("Bilateral Threshold", &m_BilateralThreshold, 0.01f, 0.2f, "%.3f m");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Depth difference treated as an edge by the bilateral blur");

			drawAlgorithmControls();
		}, 30, true);
	}

	SSAOPass::~SSAOPass()
	{
		DebugPane::removeSection(m_DebugSection);
	}

	float SSAOPass::lerp(float a, float b, float t)
	{
		return a + t * (b - a);
	}

	void SSAOPass::generateSampleKernel()
	{
		std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
		std::default_random_engine generator;

		m_SSAOKernel.clear();
		for (int i = 0; i < KERNEL_SIZE; ++i)
		{
			// 生成半球内的随机方向
			glm::vec3 sample(
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator)  // z 在 [0, 1]，保证在半球内
			);
			sample = glm::normalize(sample);
			sample *= randomFloats(generator);

			// 使采样点更靠近原点（二次分布）
			float scale = (float)i / (float)KERNEL_SIZE;
			scale = lerp(0.1f, 1.0f, scale * scale);
			sample *= scale;

			m_SSAOKernel.push_back(sample);
		}
	}

	void SSAOPass::generateNoiseTexture()
	{
		std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
		std::default_random_engine generator;

		// 生成 4x4 的随机旋转向量（使用 RGBA float 存储，避免 RGB 对齐问题）
		std::vector<glm::vec4> ssaoNoise;
		for (int i = 0; i < NOISE_SIZE * NOISE_SIZE; ++i)
		{
			// 绕 z 轴的随机旋转向量
			glm::vec4 noise(
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator) * 2.0f - 1.0f,
				0.0f,
				0.0f
			);
			ssaoNoise.push_back(noise);
		}

		// 通过 RHI 创建噪声纹理
		TextureSettings noiseSettings;
		noiseSettings.format = rhi::TextureFormat::RGBA16F;
		noiseSettings.formatExplicitlySet = true;
		noiseSettings.minFilter = rhi::FilterMode::Nearest;
		noiseSettings.magFilter = rhi::FilterMode::Nearest;
		noiseSettings.wrapS = rhi::WrapMode::Repeat;
		noiseSettings.wrapT = rhi::WrapMode::Repeat;
		noiseSettings.anisotropy = 1.0f;
		noiseSettings.HasMips = false;
		m_NoiseTexture.setTextureSettings(noiseSettings);
		m_NoiseTexture.generate2DTexture(NOISE_SIZE, NOISE_SIZE, ChannelLayout::RGBA,
			rhi::TextureFormat::RGBA32F, ssaoNoise.data());
	}

	void SSAOPass::drawAlgorithmControls()
	{
		switch (m_Algorithm) {
		case Algorithm::Crytek:
			ImGui::SliderFloat("Bias", &m_Bias, 0.0f, 0.2f, "%.4f");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Depth bias. Raise it if flat surfaces look speckled");
			ImGui::SliderFloat("Power", &m_Power, 0.5f, 4.0f, "%.2f");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Contrast of the occlusion");
			break;
		case Algorithm::SAO: {
			ImGui::SliderFloat("Bias", &m_SAOBias, 0.0f, 0.05f, "%.5f");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Multiplied by view-space z. Keep this small");
			ImGui::SliderFloat("Intensity", &m_SAOIntensity, 0.0f, 4.0f, "%.2f");
			ImGui::SliderFloat("Power", &m_SAOPower, 0.5f, 4.0f, "%.2f");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Contrast. 2 matches the squared SAO result");

			const char* qualities[] = { "Low", "Medium", "High", "Ultra" };
			int quality = static_cast<int>(m_SAOQuality);
			if (ImGui::Combo("Quality", &quality, qualities, IM_ARRAYSIZE(qualities)))
				m_SAOQuality = static_cast<SAOQuality>(quality);

			ImGui::SliderFloat("Min Horizon Angle", &m_SAOMinHorizonAngleDeg, 0.0f, 30.0f, "%.1f deg");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Reject samples too close to the horizon");
			break;
		}
		default:
			break;
		}
	}

	const char* SSAOPass::algorithmName() const
	{
		const int index = static_cast<int>(m_Algorithm);
		if (index < 0 || index >= static_cast<int>(Algorithm::Count))
			return "AO";
		return kAlgorithmNames[index];
	}

	Shader* SSAOPass::algorithmShader() const
	{
		switch (m_Algorithm) {
		case Algorithm::Crytek:
			return m_SSAOShader;
		case Algorithm::SAO:
			return m_SAOShader;
		default:
			return m_SSAOShader;
		}
	}

	void SSAOPass::uploadAlgorithmParams(const glm::mat4& projection)
	{
		auto* uboMgr = getUBOManager();
		if (!uboMgr)
			return;

		switch (m_Algorithm) {
		case Algorithm::Crytek: {
			UBOSSAOParams ssaoUBOParams{};
			for (int i = 0; i < KERNEL_SIZE; ++i)
				ssaoUBOParams.samples[i] = glm::vec4(m_SSAOKernel[i], 0.0f);
			ssaoUBOParams.kernelSize = KERNEL_SIZE;
			ssaoUBOParams.radius = m_Radius;
			ssaoUBOParams.bias = m_Bias;
			ssaoUBOParams.power = m_Power;
			cmd().updateBuffer(uboMgr->getCustomHandle(), &ssaoUBOParams, sizeof(UBOSSAOParams));
			cmd().bindUBO(UBOBinding::CustomParams,
				uboMgr->getCustomHandle(), sizeof(UBOSSAOParams));
			break;
		}
		case Algorithm::SAO: {
			UBOSAOParams saoParams = makeSAOParams(projection);
			cmd().updateBuffer(uboMgr->getCustomHandle(), &saoParams, sizeof(UBOSAOParams));
			cmd().bindUBO(UBOBinding::CustomParams,
				uboMgr->getCustomHandle(), sizeof(UBOSAOParams));
			break;
		}
		default:
			break;
		}
	}

	void SSAOPass::bindAlgorithmTextures(const GeometryPassOutput& gBufferOutput)
	{
		switch (m_Algorithm) {
		case Algorithm::Crytek:
			cmd().bindTextureUnit(gBufferOutput.depthStencilTexture->getRHIHandle(), TextureUnit::SSAODepth);
			cmd().bindTextureUnit(gBufferOutput.normalTexture->getRHIHandle(), TextureUnit::SSAONormal);
			cmd().bindTextureUnit(m_NoiseTexture.getRHIHandle(), TextureUnit::SSAONoise);
			break;
		case Algorithm::SAO:
			cmd().bindTextureUnit(m_DepthPyramidTexture.getRHIHandle(), TextureUnit::SSAODepth);
			break;
		default:
			break;
		}
	}

	UBOSAOParams SSAOPass::makeSAOParams(const glm::mat4& projection) const
	{
		float sampleCount = 11.0f;
		float spiralTurns = 9.0f;
		switch (m_SAOQuality) {
		case SAOQuality::Low:
			sampleCount = 7.0f;
			spiralTurns = 5.0f;
			break;
		case SAOQuality::Medium:
			sampleCount = 11.0f;
			spiralTurns = 9.0f;
			break;
		case SAOQuality::High:
			sampleCount = 16.0f;
			spiralTurns = 10.0f;
			break;
		case SAOQuality::Ultra:
			sampleCount = 32.0f;
			spiralTurns = 14.0f;
			break;
		}

		constexpr float kTau = 6.28318530718f;
		const float radius = glm::max(m_Radius, 1.0e-4f);
		const float invCount = 1.0f / (sampleCount - 0.5f);
		const float inc = invCount * spiralTurns * kTau;
		const float peak = 0.1f * radius;
		const float width = static_cast<float>(Window::getWidth());
		const float height = static_cast<float>(Window::getHeight());
		const float projectionScale = glm::min(
			0.5f * projection[0][0] * width,
			0.5f * projection[1][1] * height);
		const float horizonRad = glm::radians(m_SAOMinHorizonAngleDeg);
		const float horizonSin = std::sin(horizonRad);

		UBOSAOParams params{};
		params.sampleCount = glm::vec4(sampleCount, invCount, spiralTurns, 1.0f / (radius * radius));
		params.angleIncCosSin = glm::vec2(std::cos(inc), std::sin(inc));
		params.minHorizonAngleSineSquared = horizonSin * horizonSin;
		params.bias = m_SAOBias;
		params.peak2 = peak * peak;
		params.projectionScaleRadius = projectionScale * radius;
		params.intensity = (kTau * peak * m_SAOIntensity) / sampleCount;
		params.power = m_SAOPower;
		params.maxLevel = m_DepthPyramidLevels - 1;
		params.farPlane = FAR_PLANE;
		return params;
	}

	void SSAOPass::buildDepthPyramid(const GeometryPassOutput& gBufferOutput)
	{
		cmd().pushDebugGroup("DepthPyramid");

		const uint32_t width = m_DepthPyramidRT.getWidth();
		const uint32_t height = m_DepthPyramidRT.getHeight();
		const rhi::TextureHandle pyramidHandle = m_DepthPyramidTexture.getRHIHandle();
		auto* uboMgr = getUBOManager();

		auto uploadPyramidMode = [&](int isBaseLevel) {
			if (!uboMgr)
				return;
			UBODepthPyramidParams pyramidParams{ isBaseLevel };
			cmd().updateBuffer(uboMgr->getCustomHandle(), &pyramidParams, sizeof(UBODepthPyramidParams));
			cmd().bindUBO(UBOBinding::CustomParams,
				uboMgr->getCustomHandle(), sizeof(UBODepthPyramidParams));
		};

		cmd().bindTextureUnit(gBufferOutput.depthStencilTexture->getRHIHandle(), TextureUnit::SSAODepth);
		cmd().setRenderTargetColorAttachment(m_DepthPyramidRT.getHandle(), 0, pyramidHandle, 0, 0);

		rhi::RenderPassParams pyramidPassParams;
		pyramidPassParams.viewport = { 0, 0, width, height };
		pyramidPassParams.clearColorFlag = false;
		pyramidPassParams.clearDepthFlag = false;
		pyramidPassParams.clearStencilFlag = false;
		cmd().beginRenderPass(m_DepthPyramidRT.getHandle(), pyramidPassParams);

		rhi::PipelineState pipeline;
		pipeline.program = m_DepthPyramidShader->getProgramHandle();
		pipeline.depthTest = false;
		pipeline.depthWrite = false;
		pipeline.blendEnable = false;
		pipeline.stencilEnable = false;
		pipeline.cullMode = rhi::CullMode::Back;
		cmd().bindPipeline(pipeline);

		// Level 0：硬件深度 -> 线性视图距离
		uploadPyramidMode(1);
		ModelRenderer::drawNdcPlane(cmd());

		// Level 1..N：逐级锁定上一层 [level-1, level-1] 做 Rotated-Grid 子采样，消除渲染反馈环
		if (m_DepthPyramidLevels > 1) {
			uploadPyramidMode(0);
			cmd().bindTextureUnit(pyramidHandle, TextureUnit::SSAODepth);
			for (int level = 1; level < m_DepthPyramidLevels; ++level) {
				const uint8_t srcLevel = static_cast<uint8_t>(level - 1);
				cmd().setTextureMipRange(pyramidHandle, srcLevel, srcLevel);
				cmd().setRenderTargetColorAttachment(m_DepthPyramidRT.getHandle(), 0,
					pyramidHandle, static_cast<uint8_t>(level), 0);
				cmd().setViewport(0, 0,
					std::max(1u, width >> level), std::max(1u, height >> level));
				ModelRenderer::drawNdcPlane(cmd());
			}
			cmd().setTextureMipRange(pyramidHandle, 0,
				static_cast<uint8_t>(m_DepthPyramidLevels - 1));
		}

		cmd().endRenderPass();
		cmd().popDebugGroup();
	}

	PreLightingPassOutput SSAOPass::executeSSAOPass(ICamera* camera, GeometryPassOutput& gBufferOutput)
	{
		if (!m_Enabled)
			return {};

		cmd().pushDebugGroup("SSAOPass");

		auto* uboMgr = getUBOManager();
		if (uboMgr) {
			// PerFrame UBO
			uboMgr->preparePerFrame(camera->getViewMatrix(), camera->getProjectionMatrix(),
				glm::vec3(0.0f), glm::vec2(Window::getWidth(), Window::getHeight()));
			cmd().updateBuffer(uboMgr->getPerFrameHandle(),
				&uboMgr->getPerFrameData(), sizeof(UBOPerFrame));
			cmd().bindUBO(UBOBinding::PerFrame,
				uboMgr->getPerFrameHandle(), sizeof(UBOPerFrame));
		}

		if (m_Algorithm == Algorithm::SAO)
			buildDepthPyramid(gBufferOutput);

		rhi::PipelineState ssaoPipeline;
		{
			cmd().pushDebugGroup(algorithmName());

			// 通过命令缓冲录制 beginRenderPass
			rhi::RenderPassParams ssaoParams;
			ssaoParams.viewport = { 0, 0, m_SSAORT.getWidth(), m_SSAORT.getHeight() };
			ssaoParams.clearColorFlag = true;
			ssaoParams.clearDepthFlag = true;
			cmd().beginRenderPass(m_SSAORT.getHandle(), ssaoParams);

			// 构建 SSAO pass 的管线状态
			ssaoPipeline.program = algorithmShader()->getProgramHandle();
			ssaoPipeline.depthTest = false;
			ssaoPipeline.blendEnable = false;
			ssaoPipeline.stencilEnable = false;
			ssaoPipeline.cullMode = rhi::CullMode::Back;
			cmd().bindPipeline(ssaoPipeline);

			// 传递采样核 + 参数 通过 UBO
			uploadAlgorithmParams(camera->getProjectionMatrix());

			// 绑定深度纹理及各算法额外需要的纹理
			bindAlgorithmTextures(gBufferOutput);

			// 绘制全屏四边形
			ModelRenderer::drawNdcPlane(cmd());
			cmd().endRenderPass();
			cmd().popDebugGroup();
		}

		{
			// ========== Blur Pass（水平 + 竖直两遍可分离双边模糊） ==========
			cmd().pushDebugGroup("SSAO Blur");

			rhi::PipelineState blurPipeline = ssaoPipeline;
			blurPipeline.program = m_SSAOBlurShader->getProgramHandle();

			const glm::vec2 step(2.0f / static_cast<float>(m_SSAORT.getWidth()),
				2.0f / static_cast<float>(m_SSAORT.getHeight()));
			RenderTarget* targets[2] = { &m_BlurPingRT, &m_SSAOBlurRT };
			Texture* inputs[2] = { m_SSAORT.getColorTexture(), m_BlurPingRT.getColorTexture() };

			for (int pass = 0; pass < 2; ++pass) {
				rhi::RenderPassParams blurParams;
				blurParams.viewport = { 0, 0, targets[pass]->getWidth(), targets[pass]->getHeight() };
				blurParams.clearColorFlag = true;
				blurParams.clearDepthFlag = false;
				cmd().beginRenderPass(targets[pass]->getHandle(), blurParams);
				cmd().bindPipeline(blurPipeline);

				if (uboMgr) {
					UBOBilateralBlurParams blurUBOParams{};
					blurUBOParams.axis = (pass == 0) ? glm::vec2(step.x, 0.0f) : glm::vec2(0.0f, step.y);
					blurUBOParams.sampleCount = 6;
					blurUBOParams.farPlaneOverEdgeDistance = FAR_PLANE / std::max(m_BilateralThreshold, 1.0e-4f);
					blurUBOParams.sigma = 4.0f;
					cmd().updateBuffer(uboMgr->getCustomHandle(), &blurUBOParams, sizeof(UBOBilateralBlurParams));
					cmd().bindUBO(UBOBinding::CustomParams,
						uboMgr->getCustomHandle(), sizeof(UBOBilateralBlurParams));
				}

				cmd().bindTextureUnit(inputs[pass]->getRHIHandle(), TextureUnit::PostProcessInput);
				ModelRenderer::drawNdcPlane(cmd());
				cmd().endRenderPass();
			}

			cmd().popDebugGroup();
		}

		cmd().popDebugGroup();

		// 返回结果
		PreLightingPassOutput output;
		output.ssaoTexture = m_SSAOBlurRT.getColorTexture();
		return output;
	}

}
