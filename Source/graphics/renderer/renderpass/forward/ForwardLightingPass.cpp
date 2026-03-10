#include "pch.h"
#include "ForwardLightingPass.h"

#include "physics/fluid/FluidSim.h"

#include <utils/loaders/ShaderLoader.h>

namespace engine
{

	ForwardLightingPass::ForwardLightingPass(Scene3D* scene) : RenderPass(scene, RenderPassType::LightingPassType), m_OwnsRT(true)
	{
		m_ModelShader = ShaderLoader::loadShader("Shaders/forward/pbr_model.glsl");
		m_TerrainShader = ShaderLoader::loadShader("Shaders/forward/pbr_terrain.glsl");
		uint8_t samples = MSAA_SAMPLE_AMOUNT > 1 ? static_cast<uint8_t>(MSAA_SAMPLE_AMOUNT) : 1;
		m_RT = new RenderTarget(Window::getWidth(), Window::getHeight(), samples);

		m_RT->addColorTexture(rhi::TextureFormat::RGBA16F)
			.addDepthStencilTexture(DepthStencilFormat::DepthStencil, false).build();
	}

	ForwardLightingPass::ForwardLightingPass(Scene3D* scene, RenderTarget* customRT) : RenderPass(scene, RenderPassType::LightingPassType), m_RT(customRT), m_OwnsRT(false)
	{
		m_ModelShader = ShaderLoader::loadShader("Shaders/forward/pbr_model.glsl");
		m_TerrainShader = ShaderLoader::loadShader("Shaders/forward/pbr_terrain.glsl");
	}

	ForwardLightingPass::~ForwardLightingPass() {
		if (m_OwnsRT) {
			delete m_RT;
		}
	}

	LightingPassOutput ForwardLightingPass::executeRenderPass(ShadowmapPassOutput& shadowmapData, ICamera* camera, bool useIBL) {
		m_RT->beginPass();

		// Setup
		ModelRenderer* modelRenderer = m_ActiveScene->getModelRenderer();
		Terrain* terrain = m_ActiveScene->getTerrain();
		FluidSim* fluid = m_ActiveScene->getFluid();
		DynamicLightManager* lightManager = m_ActiveScene->getDynamicLightManager();
		Skybox* skybox = m_ActiveScene->getSkybox();
		ProbeManager* probeManager = m_ActiveScene->getProbeManager();

		// 通过 PipelineState 设置 model shader 和渲染状态
		rhi::PipelineState pipeline;
		pipeline.program = m_ModelShader->getProgramHandle();
		pipeline.depthTest = true;
		pipeline.depthWrite = true;
		pipeline.cullMode = rhi::CullMode::Back;
		pipeline.blendEnable = false;
		pipeline.multisample = m_RT->isMultisampled();
		bindPipelineState(pipeline);

		// View setup + lighting setup
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
			// 即使不使用 IBL，也必须绑定正确类型的纹理到 samplerCube uniform 对应的纹理单元，
			// 否则 samplerCube 默认指向 unit 0（绑定的是 GL_TEXTURE_2D shadowmap），
			// 导致 GL_INVALID_OPERATION: program texture usage
			Skybox* skyboxForBind = m_ActiveScene->getSkybox();
			if (skyboxForBind && skyboxForBind->getSkyboxCubemap()) {
				skyboxForBind->getSkyboxCubemap()->bind(1);
				m_ModelShader->setUniform("irradianceMap", 1);
				skyboxForBind->getSkyboxCubemap()->bind(2);
				m_ModelShader->setUniform("prefilterMap", 2);
			}
		}
		// Render the scene
		m_ActiveScene->addModelsToRenderer();
		modelRenderer->flushOpaque(m_ModelShader, m_RenderPassType);

		// 切换 terrain shader 管线
		rhi::PipelineState terrainPipeline = pipeline;
		terrainPipeline.program = m_TerrainShader->getProgramHandle();
		bindPipelineState(terrainPipeline);

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

		// 切换回 model shader 渲染透明物体（需要开启 blend、关闭 face cull）
		rhi::PipelineState transparentPipeline = pipeline;
		transparentPipeline.program = m_ModelShader->getProgramHandle();
		transparentPipeline.blendEnable = true;
		transparentPipeline.srcColorBlend = rhi::BlendFactor::SrcAlpha;
		transparentPipeline.dstColorBlend = rhi::BlendFactor::OneMinusSrcAlpha;
		transparentPipeline.srcAlphaBlend = rhi::BlendFactor::SrcAlpha;
		transparentPipeline.dstAlphaBlend = rhi::BlendFactor::OneMinusSrcAlpha;
		transparentPipeline.stencilEnable = false;
		transparentPipeline.cullMode = rhi::CullMode::None;
		bindPipelineState(transparentPipeline);
		modelRenderer->flushTransparent(m_ModelShader, m_RenderPassType);

		m_RT->endPass();

		// Render pass output
		LightingPassOutput passOutput;
		passOutput.renderTarget = m_RT->getHandle();
		passOutput.colorTexture = m_RT->getColorTexture();
		passOutput.width = m_RT->getWidth();
		passOutput.height = m_RT->getHeight();
		passOutput.isMultisampled = m_RT->isMultisampled();
		return passOutput;
	}

	void ForwardLightingPass::bindShadowmap(Shader* shader, ShadowmapPassOutput& shadowmapData) {
		shadowmapData.depthTexture->bind(0);
		shader->setUniform("dirLightShadowmap", 0);
		shader->setUniform("lightSpaceViewProjectionMatrix", shadowmapData.directionalLightViewProjMatrix);
		shader->setUniform("dirLightShadowData.shadowBias", 0.01f);
		shader->setUniform("dirLightShadowData.lightSpaceViewProjectionMatrix", shadowmapData.directionalLightViewProjMatrix);
		shader->setUniform("dirLightShadowData.lightShadowIndex", 1);
	}

}
