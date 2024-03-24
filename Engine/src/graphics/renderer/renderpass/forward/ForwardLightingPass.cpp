#include "pch.h"
#include "ForwardLightingPass.h"

#include <utils/loaders/ShaderLoader.h>

namespace engine
{

	ForwardLightingPass::ForwardLightingPass(Scene3D* scene) : RenderPass(scene, RenderPassType::LightingPassType)
	{
		m_ModelShader = ShaderLoader::loadShader("src/shaders/pbr_model.vert", "src/shaders/pbr_model.frag");
		m_TerrainShader = ShaderLoader::loadShader("src/shaders/terrain.vert", "src/shaders/terrain.frag");

		m_Framebuffer = new Framebuffer(Window::getWidth(), Window::getHeight());
		bool shouldMultisample = MSAA_SAMPLE_AMOUNT > 1.0 ? true : false;
		m_Framebuffer->addTexture2DColorAttachment(shouldMultisample).addDepthStencilRBO(shouldMultisample).createFramebuffer();
	}

	ForwardLightingPass::ForwardLightingPass(Scene3D* scene, Framebuffer* customFramebuffer) : RenderPass(scene, RenderPassType::LightingPassType), m_Framebuffer(customFramebuffer)
	{
		m_ModelShader = ShaderLoader::loadShader("src/shaders/pbr_model.vert", "src/shaders/pbr_model.frag");
		m_TerrainShader = ShaderLoader::loadShader("src/shaders/terrain.vert", "src/shaders/terrain.frag");
	}

	ForwardLightingPass::~ForwardLightingPass() {}

	LightingPassOutput ForwardLightingPass::executeRenderPass(ShadowmapPassOutput& shadowmapData, ICamera* camera, bool useIBL) {
		glViewport(0, 0, m_Framebuffer->getWidth(), m_Framebuffer->getHeight());
		m_Framebuffer->bind();
		m_Framebuffer->clear();

		// Setup
		ModelRenderer* modelRenderer = m_ActiveScene->getModelRenderer();
		Terrain* terrain = m_ActiveScene->getTerrain();
		DynamicLightManager* lightManager = m_ActiveScene->getDynamicLightManager();
		Skybox* skybox = m_ActiveScene->getSkybox();
		ProbeManager* probeManager = m_ActiveScene->getProbeManager();

		// View setup + lighting setup
		m_GLCache->switchShader(m_ModelShader);
		lightManager->setupLightingUniforms(m_ModelShader);
		m_ModelShader->setUniform3f("viewPos", camera->getPosition());
		m_ModelShader->setUniformMat4("view", camera->getViewMatrix());
		m_ModelShader->setUniformMat4("projection", camera->getProjectionMatrix());

		// Shadowmap code
		bindShadowmap(m_ModelShader, shadowmapData);

		// IBL code
		if (useIBL) {
			m_ModelShader->setUniform1i("computeIBL", 1);
			glm::vec3 renderPos(0.0f, 0.0f, 0.0f);
			probeManager->bindProbe(renderPos, m_ModelShader);
		}
		else {
			m_ModelShader->setUniform1i("computeIBL", 0);

		}

		// Render the scene
		m_ActiveScene->addModelsToRenderer();
		modelRenderer->flushOpaque(m_ModelShader, m_RenderPassType);


		m_GLCache->switchShader(m_TerrainShader);
		lightManager->setupLightingUniforms(m_TerrainShader);
		m_TerrainShader->setUniform3f("viewPos", camera->getPosition());
		glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), terrain->getPosition());
		glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
		m_TerrainShader->setUniformMat3("normalMatrix", normalMatrix);
		m_TerrainShader->setUniformMat4("model", modelMatrix);
		m_TerrainShader->setUniformMat4("view", camera->getViewMatrix());
		m_TerrainShader->setUniformMat4("projection", camera->getProjectionMatrix());
		bindShadowmap(m_TerrainShader, shadowmapData);

		terrain->Draw(m_TerrainShader, m_RenderPassType);

		skybox->Draw(camera);

		m_GLCache->switchShader(m_ModelShader);
		modelRenderer->flushTransparent(m_ModelShader, m_RenderPassType);

		// Render pass output
		LightingPassOutput passOutput;
		passOutput.outputFramebuffer = m_Framebuffer;
		return passOutput;
	}

	void ForwardLightingPass::bindShadowmap(Shader* shader, ShadowmapPassOutput& shadowmapData) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowmapData.shadowmapFramebuffer->getDepthTexture());
		shader->setUniform1i("shadowmap", 0);
		shader->setUniformMat4("lightSpaceViewProjectionMatrix", shadowmapData.directionalLightViewProjMatrix);
	}

}