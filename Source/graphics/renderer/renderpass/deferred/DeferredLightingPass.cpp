#include "pch.h"
#include "DeferredLightingPass.h"

#include <graphics/Window.h>
#include <graphics/Shader.h>
#include <graphics/texture/Cubemap.h>
#include <graphics/renderer/GLCache.h>
#include <graphics/camera/ICamera.h>
#include <graphics/renderer/renderpass/deferred/DeferredGeometryPass.h>
#include <scene/Scene3D.h>
#include <utils/loaders/ShaderLoader.h>

namespace engine
{
	DeferredLightingPass::DeferredLightingPass(Scene3D* scene) : RenderPass(scene,RenderPassType::LightingPassType)
	{
		m_LightingShader = ShaderLoader::loadShader("Shaders/deferred/PBR_LightingPass.glsl");

		m_RT = new RenderTarget(Window::getWidth(), Window::getHeight());
		m_RT->addColorTexture(rhi::TextureFormat::RGBA16F)
			.addDepthStencilTexture(DepthStencilFormat::DepthStencil).build();
	}

	DeferredLightingPass::~DeferredLightingPass()
	{
		delete m_RT;
	}

	LightingPassOutput DeferredLightingPass::ExecuteLightingPass(ShadowmapPassOutput& inputShadowmapData, GeometryPassOutput& inputGbuffer, PreLightingPassOutput& preLightingOutput, ICamera* camera, bool useIBL)
	{
		m_RT->beginPass();

		m_GLCache->setDepthTest(false);
		m_GLCache->setMultisample(false);

		// Move the depth + stencil of the GBuffer to our render target
		auto* device = getRHIDevice();
		device->blit(inputGbuffer.renderTarget, m_RT->getHandle(),
			0, 0, inputGbuffer.width, inputGbuffer.height,
			0, 0, m_RT->getWidth(), m_RT->getHeight(),
			rhi::RHIDevice::BlitDepth | rhi::RHIDevice::BlitStencil);

		// Re-bind our RT after blit (blit may change FBO binding)
		rhi::RenderPassParams rebindParams;
		rebindParams.viewport = { 0, 0, m_RT->getWidth(), m_RT->getHeight() };
		rebindParams.clearColorFlag = false;
		rebindParams.clearDepthFlag = false;
		m_RT->beginPass(rebindParams);

		// Setup initial stencil state
		m_GLCache->setStencilTest(true);
		m_GLCache->setStencilWriteMask(0x00);

		DynamicLightManager* lightManager = m_ActiveScene->getDynamicLightManager();
		ProbeManager* probeManager = m_ActiveScene->getProbeManager();

		m_GLCache->switchShader(m_LightingShader);
		lightManager->setupLightingUniforms(m_LightingShader);
		m_LightingShader->setUniform("viewPos", camera->getPosition());
		m_LightingShader->setUniform("viewInverse", glm::inverse(camera->getViewMatrix()));
		m_LightingShader->setUniform("projectionInverse", glm::inverse(camera->getProjectionMatrix()));

		// Bind GBuffer data
		inputGbuffer.albedoTexture->bind(6);
		m_LightingShader->setUniform("albedoTexture", 6);

		inputGbuffer.normalTexture->bind(7);
		m_LightingShader->setUniform("normalTexture", 7);

		inputGbuffer.materialInfoTexture->bind(8);
		m_LightingShader->setUniform("materialInfoTexture", 8);

		// Bind SSAO texture
		if (preLightingOutput.ssaoTexture != nullptr) {
			preLightingOutput.ssaoTexture->bind(9);
			m_LightingShader->setUniform("ssaoTexture", 9);
			m_LightingShader->setUniform("useSSAO", 1);
		}
		else {
			m_LightingShader->setUniform("useSSAO", 0);
		}

		inputGbuffer.depthStencilTexture->bind(10);
		m_LightingShader->setUniform("depthTexture", 10);

		// Shadowmap code
		BindShadowmap(m_LightingShader, inputShadowmapData);

		// IBL Bindings
		glm::vec3 cameraPosition = camera->getPosition();
		probeManager->bindProbe(cameraPosition, m_LightingShader);

		// 将未使用的 samplerCube uniform (pointLightShadowCubemap) 指向已绑定 cubemap 的纹理单元，
		// 避免其默认值 0 指向只有 GL_TEXTURE_2D 的 unit 导致 GL_INVALID_OPERATION: program texture usage
		m_LightingShader->setUniform("pointLightShadowCubemap", 1); 

		// Perform lighting on the terrain (turn IBL off)
		m_LightingShader->setUniform("computeIBL", 0);
		m_GLCache->setStencilFunc(GL_EQUAL, StencilValue::TerrainStencilValue, 0xFF);
		ModelRenderer::drawNdcPlane();

		// Perform lighting on the models in the scene
		if (useIBL)
		{
			m_LightingShader->setUniform("computeIBL", 1);
		}
		else
		{
			m_LightingShader->setUniform("computeIBL", 0);
		}
		m_GLCache->setStencilFunc(GL_EQUAL, StencilValue::ModelStencilValue, 0xFF);
		ModelRenderer::drawNdcPlane();

		// Reset state
		m_GLCache->setDepthTest(true);
		
		BEGIN_EVENT("Render Skybox");
		Skybox* skybox = m_ActiveScene->getSkybox();
		m_GLCache->setStencilTest(false);

		skybox->Draw(camera);
		END_EVENT();

		m_RT->endPass();

		// Render pass output
		LightingPassOutput passOutput;
		passOutput.renderTarget = m_RT->getHandle();
		passOutput.colorTexture = m_RT->getColorTexture();
		passOutput.width = m_RT->getWidth();
		passOutput.height = m_RT->getHeight();
		passOutput.isMultisampled = false;
		return passOutput;
	}

	void DeferredLightingPass::BindShadowmap(Shader* shader, ShadowmapPassOutput& shadowmapData)
	{
		shadowmapData.depthTexture->bind(0);
		shader->setUniform("dirLightShadowmap", 0);
		shader->setUniform("dirLightShadowData.shadowBias", 0.01f);
		shader->setUniform("dirLightShadowData.lightSpaceViewProjectionMatrix", shadowmapData.directionalLightViewProjMatrix);
		shader->setUniform("dirLightShadowData.lightShadowIndex", 1);
	}
}
