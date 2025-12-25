#include "pch.h"
#include "SSAOPass.h"

#include <graphics/Window.h>
#include <graphics/renderer/GLCache.h>
#include <graphics/renderer/ModelRenderer.h>
#include <utils/loaders/ShaderLoader.h>

namespace engine
{

	SSAOPass::SSAOPass(Scene3D* scene)
		: RenderPass(scene, RenderPassType::SSAOPassType),
		m_SSAOFramebuffer(Window::getWidth(), Window::getHeight(), false),
		m_SSAOBlurFramebuffer(Window::getWidth(), Window::getHeight(), false)
	{
		// 加载着色器
		m_SSAOShader = ShaderLoader::loadShader("Shaders/post_process/ssao.glsl");
		m_SSAOBlurShader = ShaderLoader::loadShader("Shaders/post_process/ssao_blur.glsl");

		// 创建 SSAO Framebuffer（单通道，存储 AO 值）
		m_SSAOFramebuffer.addColorTexture(NormalizedSingleChannel8).createFramebuffer();

		// 创建模糊后的 Framebuffer
		m_SSAOBlurFramebuffer.addColorTexture(NormalizedSingleChannel8).createFramebuffer();

		// 生成采样核和噪声纹理
		generateSampleKernel();
		generateNoiseTexture();
	}

	SSAOPass::~SSAOPass()
	{
		glDeleteTextures(1, &m_NoiseTexture);
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

		// 生成 4x4 的随机旋转向量
		std::vector<glm::vec3> ssaoNoise;
		for (int i = 0; i < NOISE_SIZE * NOISE_SIZE; ++i)
		{
			// 绕 z 轴的随机旋转向量
			glm::vec3 noise(
				randomFloats(generator) * 2.0f - 1.0f,
				randomFloats(generator) * 2.0f - 1.0f,
				0.0f
			);
			ssaoNoise.push_back(noise);
		}

		// 创建噪声纹理
		glGenTextures(1, &m_NoiseTexture);
		glBindTexture(GL_TEXTURE_2D, m_NoiseTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, NOISE_SIZE, NOISE_SIZE, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	PreLightingPassOutput SSAOPass::executeSSAOPass(ICamera* camera, GBuffer* gbuffer)
	{
		BEGIN_EVENT("SSAO");
		// ========== SSAO Pass ==========
		glViewport(0, 0, m_SSAOFramebuffer.getWidth(), m_SSAOFramebuffer.getHeight());
		m_SSAOFramebuffer.bind();
		m_SSAOFramebuffer.clear();

		m_GLCache->setDepthTest(false);
		m_GLCache->setBlend(false);
		m_GLCache->setStencilTest(false);
		m_GLCache->switchShader(m_SSAOShader);

		// 传递采样核
		for (int i = 0; i < KERNEL_SIZE; ++i)
		{
			m_SSAOShader->setUniform("samples[" + std::to_string(i) + "]", m_SSAOKernel[i]);
		}

		// 传递矩阵
		m_SSAOShader->setUniform("projection", camera->getProjectionMatrix());
		m_SSAOShader->setUniform("projectionInverse", glm::inverse(camera->getProjectionMatrix()));
		m_SSAOShader->setUniform("view", camera->getViewMatrix());

		// 传递参数
		m_SSAOShader->setUniform("screenSize", glm::vec2(Window::getWidth(), Window::getHeight()));
		m_SSAOShader->setUniform("radius", m_Radius);
		m_SSAOShader->setUniform("bias", m_Bias);
		m_SSAOShader->setUniform("power", m_Power);
		m_SSAOShader->setUniform("kernelSize", KERNEL_SIZE);

		// 绑定 GBuffer 纹理
		gbuffer->GetNormal()->bind(0);
		m_SSAOShader->setUniform("gNormal", 0);

		gbuffer->getDepthStencilTexture()->bind(1);
		m_SSAOShader->setUniform("gDepth", 1);

		// 绑定噪声纹理
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_NoiseTexture);
		m_SSAOShader->setUniform("texNoise", 2);

		// 绘制全屏四边形
		ModelRenderer::drawNdcPlane();
		END_EVENT();

		BEGIN_EVENT("SSAO Blur");
		// ========== Blur Pass ==========
		glViewport(0, 0, m_SSAOBlurFramebuffer.getWidth(), m_SSAOBlurFramebuffer.getHeight());
		m_SSAOBlurFramebuffer.bind();
		m_SSAOBlurFramebuffer.clear();

		m_GLCache->switchShader(m_SSAOBlurShader);

		m_SSAOFramebuffer.getColorBufferTexture()->bind(0);
		m_SSAOBlurShader->setUniform("ssaoInput", 0);

		ModelRenderer::drawNdcPlane();
		END_EVENT();

		// 返回结果
		PreLightingPassOutput output;
		output.ssaoTexture = m_SSAOBlurFramebuffer.getColorBufferTexture();
		return output;
	}

}
