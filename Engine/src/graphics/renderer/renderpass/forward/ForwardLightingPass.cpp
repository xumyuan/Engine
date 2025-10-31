#include "pch.h"
#include "ForwardLightingPass.h"

#include "physics/fluid/FluidSim.h"

#include <utils/loaders/ShaderLoader.h>

namespace engine
{

	ForwardLightingPass::ForwardLightingPass(Scene3D* scene) : RenderPass(scene, RenderPassType::LightingPassType)
	{
		m_ModelShader = ShaderLoader::loadShader("src/shaders/forward/pbr_model.glsl");
		m_TerrainShader = ShaderLoader::loadShader("src/shaders/forward/pbr_terrain.glsl");
		bool shouldMultisample = MSAA_SAMPLE_AMOUNT > 1.0 ? true : false;
		m_Framebuffer = new Framebuffer(Window::getWidth(), Window::getHeight(), shouldMultisample);

		m_Framebuffer->addColorTexture(FloatingPoint16).addDepthStencilRBO(NormalizedDepthStencil).createFramebuffer();
	}

	ForwardLightingPass::ForwardLightingPass(Scene3D* scene, Framebuffer* customFramebuffer) : RenderPass(scene, RenderPassType::LightingPassType), m_Framebuffer(customFramebuffer)
	{
		m_ModelShader = ShaderLoader::loadShader("src/shaders/forward/pbr_model.glsl");
		m_TerrainShader = ShaderLoader::loadShader("src/shaders/forward/pbr_terrain.glsl");
	}

	ForwardLightingPass::~ForwardLightingPass() {}

	LightingPassOutput ForwardLightingPass::executeRenderPass(ShadowmapPassOutput& shadowmapData, ICamera* camera, bool useIBL) {
		glViewport(0, 0, m_Framebuffer->getWidth(), m_Framebuffer->getHeight());
		m_Framebuffer->bind();
		m_Framebuffer->clear();

		if (m_Framebuffer->isMultisampled()) {
			m_GLCache->setMultisample(true);
		}
		else {
			m_GLCache->setMultisample(false);
		}

		// Setup
		ModelRenderer* modelRenderer = m_ActiveScene->getModelRenderer();
		Terrain* terrain = m_ActiveScene->getTerrain();
		FluidSim* fluid = m_ActiveScene->getFluid();
		DynamicLightManager* lightManager = m_ActiveScene->getDynamicLightManager();
		Skybox* skybox = m_ActiveScene->getSkybox();
		ProbeManager* probeManager = m_ActiveScene->getProbeManager();
		// View setup + lighting setup
		m_GLCache->switchShader(m_ModelShader);
		lightManager->setupLightingUniforms(m_ModelShader);
		m_ModelShader->setUniform("viewPos", camera->getPosition());
		m_ModelShader->setUniform("view", camera->getViewMatrix());
		m_ModelShader->setUniform("projection", camera->getProjectionMatrix());
		// Shadowmap code
		bindShadowmap(m_ModelShader, shadowmapData);
		// IBL code
		if (useIBL) {
			m_ModelShader->setUniform("computeIBL", 1);
			glm::vec3 renderPos(0.0f, 0.0f, 0.0f);
			probeManager->bindProbe(renderPos, m_ModelShader);
		}
		else {
			m_ModelShader->setUniform("computeIBL", 0);

		}
		// Render the scene
		m_ActiveScene->addModelsToRenderer();
		modelRenderer->flushOpaque(m_ModelShader, m_RenderPassType);

		m_GLCache->switchShader(m_TerrainShader);
		lightManager->setupLightingUniforms(m_TerrainShader);
		m_TerrainShader->setUniform("viewPos", camera->getPosition());
		glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), terrain->getPosition());
		glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
		m_TerrainShader->setUniform("normalMatrix", normalMatrix);
		m_TerrainShader->setUniform("model", modelMatrix);
		m_TerrainShader->setUniform("view", camera->getViewMatrix());
		m_TerrainShader->setUniform("projection", camera->getProjectionMatrix());
		bindShadowmap(m_TerrainShader, shadowmapData);
		terrain->Draw(m_TerrainShader, m_RenderPassType);

		FPSCamera* fpscamera = dynamic_cast<FPSCamera*>(camera);
		if (fpscamera && fluid) {
			fluid->drawParticle(dynamic_cast<FPSCamera*>(camera));
		}
		skybox->Draw(camera);

		m_GLCache->switchShader(m_ModelShader);
		modelRenderer->flushTransparent(m_ModelShader, m_RenderPassType);

		// Render pass output
		LightingPassOutput passOutput;
		passOutput.outputFramebuffer = m_Framebuffer;
		return passOutput;
	}

	void ForwardLightingPass::bindShadowmap(Shader* shader, ShadowmapPassOutput& shadowmapData) {
		shadowmapData.shadowmapFramebuffer->getDepthStencilTexture()->bind(0);
		shader->setUniform("dirLightShadowmap", 0);
		/*shader->setUniform("shadowmap", 0);
		shader->setUniform("lightSpaceViewProjectionMatrix", shadowmapData.directionalLightViewProjMatrix);*/

		shader->setUniform("dirLightShadowData.shadowBias", 0.01f);
		shader->setUniform("dirLightShadowData.lightSpaceViewProjectionMatrix", shadowmapData.directionalLightViewProjMatrix);
		shader->setUniform("dirLightShadowData.lightShadowIndex", 1);
	}

}
