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
		m_SSAOShader = ShaderLoader::loadShader("Source/shaders/post_process/ssao.glsl");
		m_SAOShader = ShaderLoader::loadShader("Source/shaders/post_process/sao.glsl");
		m_SSAOBlurShader = ShaderLoader::loadShader("Source/shaders/post_process/ssao_blur.glsl");
		m_DepthPyramidShader = ShaderLoader::loadShader("Source/shaders/post_process/depth_pyramid_minify.glsl");

		// AO 原始输出（RGB8：R=可见度，GB=打包的 bilateral key）、乒乓缓冲与最终 R8 输出
		m_SSAORT.addColorTexture(rhi::TextureFormat::RGB8).build();
		m_BlurPingRT.addColorTexture(rhi::TextureFormat::RGB8).build();
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

		initCrytekResources();

		m_DebugSection = DebugPane::addSection("SSAO", [this]() {
			drawDebugUI();
		}, 30, true);
	}

	SSAOPass::~SSAOPass()
	{
		DebugPane::removeSection(m_DebugSection);
	}

	void SSAOPass::initCrytekResources()
	{
		std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
		std::default_random_engine generator;

		// 生成半球内的随机采样核（二次分布使采样点更靠近原点）
		m_SSAOKernel.clear();
		m_SSAOKernel.reserve(KERNEL_SIZE);
		for (int i = 0; i < KERNEL_SIZE; ++i) {
			glm::vec3 sample(
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator)
			);
			sample = glm::normalize(sample) * randomFloats(generator);
			float scale = static_cast<float>(i) / static_cast<float>(KERNEL_SIZE);
			scale = glm::mix(0.1f, 1.0f, scale * scale);
			m_SSAOKernel.push_back(sample * scale);
		}

		// 生成 4x4 绕 z 轴的随机旋转噪声纹理
		std::vector<glm::vec4> ssaoNoise;
		ssaoNoise.reserve(NOISE_SIZE * NOISE_SIZE);
		for (int i = 0; i < NOISE_SIZE * NOISE_SIZE; ++i) {
			ssaoNoise.emplace_back(
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator) * 2.0f - 1.0f,
				0.0f,
				0.0f
			);
		}

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

	void SSAOPass::drawDebugUI()
	{
		ImGui::Checkbox("Enabled", &m_Settings.enabled);

		int algorithm = static_cast<int>(m_Settings.algorithm);
		if (ImGui::Combo("Algorithm", &algorithm, kAlgorithmNames, static_cast<int>(Algorithm::Count)))
			m_Settings.algorithm = static_cast<Algorithm>(algorithm);

		ImGui::SliderFloat("Bilateral Threshold", &m_Settings.bilateralThreshold, 0.01f, 0.2f, "%.3f m");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Depth difference treated as an edge by the bilateral blur");

		switch (m_Settings.algorithm) {
		case Algorithm::Crytek:
			ImGui::SliderFloat("Radius", &m_Settings.crytek.radius, 0.05f, 2.0f, "%.3f");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("View-space hemisphere sample radius");
			ImGui::SliderFloat("Bias", &m_Settings.crytek.bias, 0.0f, 0.2f, "%.4f");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Depth bias. Raise it if flat surfaces look speckled");
			ImGui::SliderFloat("Power", &m_Settings.crytek.power, 0.5f, 4.0f, "%.2f");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Contrast of the occlusion");
			break;
		case Algorithm::SAO: {
			ImGui::SliderFloat("Radius", &m_Settings.sao.radius, 0.05f, 2.0f, "%.3f");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("World-space obscurance falloff radius");
			ImGui::SliderFloat("Bias", &m_Settings.sao.bias, 0.0f, 0.05f, "%.5f");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Multiplied by view-space z. Keep this small");
			ImGui::SliderFloat("Intensity", &m_Settings.sao.intensity, 0.0f, 4.0f, "%.2f");
			ImGui::SliderFloat("Power", &m_Settings.sao.power, 0.5f, 4.0f, "%.2f");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Contrast. 2 matches the squared SAO result");

			const char* qualities[] = { "Low", "Medium", "High", "Ultra" };
			int quality = static_cast<int>(m_Settings.sao.quality);
			if (ImGui::Combo("Quality", &quality, qualities, IM_ARRAYSIZE(qualities)))
				m_Settings.sao.quality = static_cast<Quality>(quality);

			ImGui::SliderFloat("Min Horizon Angle", &m_Settings.sao.minHorizonAngleDeg, 0.0f, 30.0f, "%.1f deg");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Reject samples too close to the horizon");
			break;
		}
		default:
			break;
		}
	}

	UBOSAOParams SSAOPass::makeSAOParams(const glm::mat4& projection) const
	{
		float sampleCount = 11.0f;
		float spiralTurns = 9.0f;
		switch (m_Settings.sao.quality) {
		case Quality::Low:
			sampleCount = 7.0f;
			spiralTurns = 5.0f;
			break;
		case Quality::Medium:
			sampleCount = 11.0f;
			spiralTurns = 9.0f;
			break;
		case Quality::High:
			sampleCount = 16.0f;
			spiralTurns = 10.0f;
			break;
		case Quality::Ultra:
			sampleCount = 32.0f;
			spiralTurns = 14.0f;
			break;
		}

		constexpr float kTau = 6.28318530718f;
		const float radius = glm::max(m_Settings.sao.radius, 1.0e-4f);
		const float invCount = 1.0f / (sampleCount - 0.5f);
		const float inc = invCount * spiralTurns * kTau;
		const float peak = 0.1f * radius;
		const float width = static_cast<float>(Window::getWidth());
		const float height = static_cast<float>(Window::getHeight());
		const float projectionScale = glm::min(
			0.5f * projection[0][0] * width,
			0.5f * projection[1][1] * height);
		const float horizonRad = glm::radians(m_Settings.sao.minHorizonAngleDeg);
		const float horizonSin = std::sin(horizonRad);

		UBOSAOParams params{};
		params.sampleCount = glm::vec4(sampleCount, invCount, spiralTurns, 1.0f / (radius * radius));
		params.angleIncCosSin = glm::vec2(std::cos(inc), std::sin(inc));
		params.minHorizonAngleSineSquared = horizonSin * horizonSin;
		params.bias = m_Settings.sao.bias;
		params.peak2 = peak * peak;
		params.projectionScaleRadius = projectionScale * radius;
		params.intensity = (kTau * peak * m_Settings.sao.intensity) / sampleCount;
		params.power = m_Settings.sao.power;
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

	void SSAOPass::renderCrytek(const GeometryPassOutput& gBufferOutput)
	{
		cmd().pushDebugGroup("Crytek");

		rhi::RenderPassParams passParams;
		passParams.viewport = { 0, 0, m_SSAORT.getWidth(), m_SSAORT.getHeight() };
		passParams.clearColorFlag = true;
		passParams.clearDepthFlag = false;
		passParams.clearStencilFlag = false;
		cmd().beginRenderPass(m_SSAORT.getHandle(), passParams);

		rhi::PipelineState pipeline;
		pipeline.program = m_SSAOShader->getProgramHandle();
		pipeline.depthTest = false;
		pipeline.depthWrite = false;
		pipeline.blendEnable = false;
		pipeline.stencilEnable = false;
		pipeline.cullMode = rhi::CullMode::Back;
		cmd().bindPipeline(pipeline);

		if (auto* uboMgr = getUBOManager()) {
			UBOSSAOParams params{};
			for (int i = 0; i < KERNEL_SIZE; ++i)
				params.samples[i] = glm::vec4(m_SSAOKernel[i], 0.0f);
			params.kernelSize = KERNEL_SIZE;
			params.radius = m_Settings.crytek.radius;
			params.bias = m_Settings.crytek.bias;
			params.power = m_Settings.crytek.power;
			cmd().updateBuffer(uboMgr->getCustomHandle(), &params, sizeof(UBOSSAOParams));
			cmd().bindUBO(UBOBinding::CustomParams, uboMgr->getCustomHandle(), sizeof(UBOSSAOParams));
		}

		cmd().bindTextureUnit(gBufferOutput.depthStencilTexture->getRHIHandle(), TextureUnit::SSAODepth);
		cmd().bindTextureUnit(gBufferOutput.normalTexture->getRHIHandle(), TextureUnit::SSAONormal);
		cmd().bindTextureUnit(m_NoiseTexture.getRHIHandle(), TextureUnit::SSAONoise);

		ModelRenderer::drawNdcPlane(cmd());
		cmd().endRenderPass();
		cmd().popDebugGroup();
	}

	void SSAOPass::renderSAO(const glm::mat4& projection, const GeometryPassOutput& gBufferOutput)
	{
		buildDepthPyramid(gBufferOutput);

		cmd().pushDebugGroup("SAO");

		rhi::RenderPassParams passParams;
		passParams.viewport = { 0, 0, m_SSAORT.getWidth(), m_SSAORT.getHeight() };
		passParams.clearColorFlag = true;
		passParams.clearDepthFlag = false;
		passParams.clearStencilFlag = false;
		cmd().beginRenderPass(m_SSAORT.getHandle(), passParams);

		rhi::PipelineState pipeline;
		pipeline.program = m_SAOShader->getProgramHandle();
		pipeline.depthTest = false;
		pipeline.depthWrite = false;
		pipeline.blendEnable = false;
		pipeline.stencilEnable = false;
		pipeline.cullMode = rhi::CullMode::Back;
		cmd().bindPipeline(pipeline);

		if (auto* uboMgr = getUBOManager()) {
			UBOSAOParams params = makeSAOParams(projection);
			cmd().updateBuffer(uboMgr->getCustomHandle(), &params, sizeof(UBOSAOParams));
			cmd().bindUBO(UBOBinding::CustomParams, uboMgr->getCustomHandle(), sizeof(UBOSAOParams));
		}

		cmd().bindTextureUnit(m_DepthPyramidTexture.getRHIHandle(), TextureUnit::SSAODepth);

		ModelRenderer::drawNdcPlane(cmd());
		cmd().endRenderPass();
		cmd().popDebugGroup();
	}

	void SSAOPass::renderBilateralBlur()
	{
		cmd().pushDebugGroup("SSAO Blur");

		rhi::PipelineState blurPipeline;
		blurPipeline.program = m_SSAOBlurShader->getProgramHandle();
		blurPipeline.depthTest = false;
		blurPipeline.depthWrite = false;
		blurPipeline.blendEnable = false;
		blurPipeline.stencilEnable = false;
		blurPipeline.cullMode = rhi::CullMode::Back;

		auto* uboMgr = getUBOManager();
		const glm::vec2 step(2.0f / static_cast<float>(m_SSAORT.getWidth()),
			2.0f / static_cast<float>(m_SSAORT.getHeight()));
		RenderTarget* targets[2] = { &m_BlurPingRT, &m_SSAOBlurRT };
		Texture* inputs[2] = { m_SSAORT.getColorTexture(), m_BlurPingRT.getColorTexture() };

		for (int pass = 0; pass < 2; ++pass) {
			rhi::RenderPassParams blurParams;
			blurParams.viewport = { 0, 0, targets[pass]->getWidth(), targets[pass]->getHeight() };
			blurParams.clearColorFlag = true;
			blurParams.clearDepthFlag = false;
			blurParams.clearStencilFlag = false;
			cmd().beginRenderPass(targets[pass]->getHandle(), blurParams);
			cmd().bindPipeline(blurPipeline);

			if (uboMgr) {
				UBOBilateralBlurParams blurUBOParams{};
				blurUBOParams.axis = (pass == 0) ? glm::vec2(step.x, 0.0f) : glm::vec2(0.0f, step.y);
				blurUBOParams.sampleCount = 6;
				blurUBOParams.farPlaneOverEdgeDistance = FAR_PLANE / std::max(m_Settings.bilateralThreshold, 1.0e-4f);
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

	PreLightingPassOutput SSAOPass::executeSSAOPass(ICamera* camera, GeometryPassOutput& gBufferOutput)
	{
		if (!m_Settings.enabled)
			return {};

		cmd().pushDebugGroup("SSAOPass");

		if (auto* uboMgr = getUBOManager()) {
			uboMgr->preparePerFrame(camera->getViewMatrix(), camera->getProjectionMatrix(),
				glm::vec3(0.0f), glm::vec2(Window::getWidth(), Window::getHeight()));
			cmd().updateBuffer(uboMgr->getPerFrameHandle(),
				&uboMgr->getPerFrameData(), sizeof(UBOPerFrame));
			cmd().bindUBO(UBOBinding::PerFrame,
				uboMgr->getPerFrameHandle(), sizeof(UBOPerFrame));
		}

		switch (m_Settings.algorithm) {
		case Algorithm::Crytek:
			renderCrytek(gBufferOutput);
			break;
		case Algorithm::SAO:
			renderSAO(camera->getProjectionMatrix(), gBufferOutput);
			break;
		default:
			break;
		}

		renderBilateralBlur();

		cmd().popDebugGroup();

		PreLightingPassOutput output;
		output.ssaoTexture = m_SSAOBlurRT.getColorTexture();
		return output;
	}

}
