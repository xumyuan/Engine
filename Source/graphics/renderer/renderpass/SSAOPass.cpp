#include "pch.h"
#include "SSAOPass.h"

#include <graphics/Window.h>
#include <graphics/renderer/ModelRenderer.h>
#include <graphics/UniformBufferManager.h>
#include <utils/loaders/ShaderLoader.h>

namespace engine
{

	SSAOPass::SSAOPass(const RenderScene& renderScene)
		: RenderPass(renderScene, RenderPassType::SSAOPassType),
		m_SSAORT(Window::getWidth(), Window::getHeight()),
		m_SSAOBlurRT(Window::getWidth(), Window::getHeight())
	{
		// 加载着色器
		m_SSAOShader = ShaderLoader::loadShader("Shaders/post_process/ssao.glsl");
		m_SSAOBlurShader = ShaderLoader::loadShader("Shaders/post_process/ssao_blur.glsl");

		// 创建 SSAO RenderTarget（单通道，存储 AO 值）
		m_SSAORT.addColorTexture(rhi::TextureFormat::R8).build();

		// 创建模糊后的 RenderTarget
		m_SSAOBlurRT.addColorTexture(rhi::TextureFormat::R8).build();

		// 生成采样核和噪声纹理
		generateSampleKernel();
		generateNoiseTexture();
	}

	SSAOPass::~SSAOPass()
	{
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
			ssaoNoise.data());
	}

	PreLightingPassOutput SSAOPass::executeSSAOPass(ICamera* camera, GeometryPassOutput& gBufferOutput)
	{
		BEGIN_EVENT("SSAO");
		// ========== SSAO Pass ==========
		m_SSAORT.beginPass();

		// 构建 SSAO pass 的管线状态
		rhi::PipelineState ssaoPipeline;
		ssaoPipeline.program = m_SSAOShader->getProgramHandle();
		ssaoPipeline.depthTest = false;
		ssaoPipeline.blendEnable = false;
		ssaoPipeline.stencilEnable = false;
		ssaoPipeline.cullMode = rhi::CullMode::Back;
		bindPipelineState(ssaoPipeline);

		// 传递采样核 + 参数 通过 UBO
		if (auto* uboMgr = getUBOManager()) {
			// PerFrame UBO
			uboMgr->updatePerFrame(camera->getViewMatrix(), camera->getProjectionMatrix(),
				glm::vec3(0.0f), glm::vec2(Window::getWidth(), Window::getHeight()));
			uboMgr->bindPerFrame();

			// SSAO Params UBO
			UBOSSAOParams ssaoParams{};
			for (int i = 0; i < KERNEL_SIZE; ++i) {
				ssaoParams.samples[i] = glm::vec4(m_SSAOKernel[i], 0.0f);
			}
			ssaoParams.kernelSize = KERNEL_SIZE;
			ssaoParams.radius = m_Radius;
			ssaoParams.bias = m_Bias;
			ssaoParams.power = m_Power;
			uboMgr->updateSSAOParams(ssaoParams);
			uboMgr->bindCustom(sizeof(UBOSSAOParams));
		}

		// 绑定 GBuffer 纹理
		gBufferOutput.normalTexture->bind(0);
		m_SSAOShader->setUniform("gNormal", 0);

		gBufferOutput.depthStencilTexture->bind(1);
		m_SSAOShader->setUniform("gDepth", 1);

		// 绑定噪声纹理
		m_NoiseTexture.bind(2);
		m_SSAOShader->setUniform("texNoise", 2);

		// 绘制全屏四边形
		ModelRenderer::drawNdcPlane();
		m_SSAORT.endPass();
		END_EVENT();

		BEGIN_EVENT("SSAO Blur");
		// ========== Blur Pass ==========
		m_SSAOBlurRT.beginPass();

		// 构建 Blur pass 的管线状态（复用大部分设置，仅切换 shader）
		rhi::PipelineState blurPipeline = ssaoPipeline;
		blurPipeline.program = m_SSAOBlurShader->getProgramHandle();
		bindPipelineState(blurPipeline);

		m_SSAORT.getColorTexture()->bind(0);
		m_SSAOBlurShader->setUniform("ssaoInput", 0);

		ModelRenderer::drawNdcPlane();
		m_SSAOBlurRT.endPass();
		END_EVENT();

		// 返回结果
		PreLightingPassOutput output;
		output.ssaoTexture = m_SSAOBlurRT.getColorTexture();
		return output;
	}

}
