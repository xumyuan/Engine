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
		m_SSAOBlurRT(Window::getWidth(), Window::getHeight())
	{
		// 加载着色器
		m_SSAOShader = ShaderLoader::loadShader("Source/shaders/post_process/ssao.glsl");
		m_SAOShader = ShaderLoader::loadShader("Source/shaders/post_process/sao.glsl");
		m_SSAOBlurShader = ShaderLoader::loadShader("Source/shaders/post_process/ssao_blur.glsl");

		// 创建 SSAO RenderTarget（单通道，存储 AO 值）
		m_SSAORT.addColorTexture(rhi::TextureFormat::R8).build();

		// 创建模糊后的 RenderTarget
		m_SSAOBlurRT.addColorTexture(rhi::TextureFormat::R8).build();

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
			cmd().bindTextureUnit(gBufferOutput.normalTexture->getRHIHandle(), TextureUnit::SSAONormal);
			cmd().bindTextureUnit(m_NoiseTexture.getRHIHandle(), TextureUnit::SSAONoise);
			break;
		case Algorithm::SAO:
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
		return params;
	}

	PreLightingPassOutput SSAOPass::executeSSAOPass(ICamera* camera, GeometryPassOutput& gBufferOutput)
	{
		if (!m_Enabled)
			return {};

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
		if (auto* uboMgr = getUBOManager()) {
			// PerFrame UBO
			uboMgr->preparePerFrame(camera->getViewMatrix(), camera->getProjectionMatrix(),
				glm::vec3(0.0f), glm::vec2(Window::getWidth(), Window::getHeight()));
			cmd().updateBuffer(uboMgr->getPerFrameHandle(),
				&uboMgr->getPerFrameData(), sizeof(UBOPerFrame));
			cmd().bindUBO(UBOBinding::PerFrame,
				uboMgr->getPerFrameHandle(), sizeof(UBOPerFrame));

			uploadAlgorithmParams(camera->getProjectionMatrix());
		}

		// 绑定深度纹理；各算法额外需要的纹理（如法线、噪声）在 bindAlgorithmTextures 里绑定。
		cmd().bindTextureUnit(gBufferOutput.depthStencilTexture->getRHIHandle(), TextureUnit::SSAODepth);
		bindAlgorithmTextures(gBufferOutput);

			// 绘制全屏四边形
			ModelRenderer::drawNdcPlane(cmd());
			cmd().endRenderPass();
			cmd().popDebugGroup();
		}

		{
			// ========== Blur Pass ==========
			cmd().pushDebugGroup("SSAO Blur");

		rhi::RenderPassParams blurParams;
		blurParams.viewport = { 0, 0, m_SSAOBlurRT.getWidth(), m_SSAOBlurRT.getHeight() };
		blurParams.clearColorFlag = true;
		blurParams.clearDepthFlag = true;
		cmd().beginRenderPass(m_SSAOBlurRT.getHandle(), blurParams);

		// 构建 Blur pass 的管线状态（复用大部分设置，仅切换 shader）
		rhi::PipelineState blurPipeline = ssaoPipeline;
		blurPipeline.program = m_SSAOBlurShader->getProgramHandle();
		cmd().bindPipeline(blurPipeline);

		cmd().bindTextureUnit(m_SSAORT.getColorTexture()->getRHIHandle(), TextureUnit::PostProcessInput);

			ModelRenderer::drawNdcPlane(cmd());
			cmd().endRenderPass();
			cmd().popDebugGroup();
		}

		// 返回结果
		PreLightingPassOutput output;
		output.ssaoTexture = m_SSAOBlurRT.getColorTexture();
		return output;
	}

}
