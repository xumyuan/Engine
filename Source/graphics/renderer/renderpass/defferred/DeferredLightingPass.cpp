#include "pch.h"
#include "DeferredLightingPass.h"

#include <graphics/Window.h>
#include <graphics/shader.h>
#include <graphics/texture/Cubemap.h>
#include <graphics/renderer/GLCache.h>
#include <graphics/camera/ICamera.h>
#include <graphics/renderer/renderpass/defferred/DeferredGeometryPass.h>
#include <scene/Scene3D.h>
#include <utils/loaders/ShaderLoader.h>

namespace engine
{
	DeferredLightingPass::DeferredLightingPass(Scene3D* scene) : RenderPass(scene,RenderPassType::LightingPassType), m_AllocatedFramebuffer(true)
	{
		m_LightingShader = ShaderLoader::loadShader("Shaders/deferred/PBR_LightingPass.glsl");

		m_Framebuffer = new Framebuffer(Window::getWidth(), Window::getHeight(), false);
		m_Framebuffer->addColorTexture(FloatingPoint16).addDepthStencilTexture(NormalizedDepthStencil).createFramebuffer();
	}

	DeferredLightingPass::DeferredLightingPass(Scene3D* scene, Framebuffer* customFramebuffer) : RenderPass(scene, RenderPassType::LightingPassType), m_AllocatedFramebuffer(false), m_Framebuffer(customFramebuffer)
	{
		m_LightingShader = ShaderLoader::loadShader("Shaders/deferred/PBR_LightingPass.glsl");
	}

	DeferredLightingPass::~DeferredLightingPass()
	{
		if (m_AllocatedFramebuffer) {
			delete m_Framebuffer;
		}
	}

	LightingPassOutput DeferredLightingPass::ExecuteLightingPass(ShadowmapPassOutput& inputShadowmapData, GBuffer* inputGbuffer, ICamera* camera, bool useIBL)
	{
		// Framebuffer setup
		glViewport(0, 0, m_Framebuffer->getWidth(), m_Framebuffer->getHeight());
		glViewport(0, 0, m_Framebuffer->getWidth(), m_Framebuffer->getHeight());
		m_Framebuffer->bind();
		m_Framebuffer->clear();
		m_GLCache->setDepthTest(false);
		m_GLCache->setMultisample(false);

		// Move the depth + stencil of the GBuffer to our framebuffer
		// NOTE: Framebuffers have to have identical depth + stencil formats for this to work
		glBindFramebuffer(GL_READ_FRAMEBUFFER, inputGbuffer->getFramebuffer());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_Framebuffer->getFramebuffer());
		glBlitFramebuffer(0, 0, inputGbuffer->getWidth(), inputGbuffer->getHeight(), 0, 0, m_Framebuffer->getWidth(), m_Framebuffer->getHeight(), GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

		// Setup initial stencil state
		m_GLCache->setStencilTest(true);
		m_GLCache->setStencilWriteMask(0x00); // Do not update stencil values

		DynamicLightManager* lightManager = m_ActiveScene->getDynamicLightManager();
		ProbeManager* probeManager = m_ActiveScene->getProbeManager();

		m_GLCache->switchShader(m_LightingShader);
		lightManager->setupLightingUniforms(m_LightingShader);
		m_LightingShader->setUniform("viewPos", camera->getPosition());
		m_LightingShader->setUniform("viewInverse", glm::inverse(camera->getViewMatrix()));
		m_LightingShader->setUniform("projectionInverse", glm::inverse(camera->getProjectionMatrix()));

		// Bind GBuffer data
		inputGbuffer->GetAlbedo()->bind(6);
		m_LightingShader->setUniform("albedoTexture", 6);

		inputGbuffer->GetNormal()->bind(7);
		m_LightingShader->setUniform("normalTexture", 7);

		inputGbuffer->GetMaterialInfo()->bind(8);
		m_LightingShader->setUniform("materialInfoTexture", 8);

		/*preLightingOutput.ssaoTexture->bind(9);
		m_LightingShader->setUniform("ssaoTexture", 9);*/

		inputGbuffer->getDepthStencilTexture()->bind(10);
		m_LightingShader->setUniform("depthTexture", 10);

		// Shadowmap code
		BindShadowmap(m_LightingShader, inputShadowmapData);

		// Finally perform the lighting using the GBuffer

		// IBL Bindings
		glm::vec3 cameraPosition = camera->getPosition();
		probeManager->bindProbe(cameraPosition, m_LightingShader); 
		// TODO: Should use camera component

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

		// Render pass output
		LightingPassOutput passOutput;
		passOutput.outputFramebuffer = m_Framebuffer;
		return passOutput;
	}

	void DeferredLightingPass::BindShadowmap(Shader* shader, ShadowmapPassOutput& shadowmapData)
	{
		shadowmapData.shadowmapFramebuffer->getDepthStencilTexture()->bind(0);
		shader->setUniform("dirLightShadowmap", 0);
		shader->setUniform("dirLightShadowData.shadowBias", 0.01f);
		shader->setUniform("dirLightShadowData.lightSpaceViewProjectionMatrix", shadowmapData.directionalLightViewProjMatrix);
		shader->setUniform("dirLightShadowData.lightShadowIndex", 1);
	}
}
