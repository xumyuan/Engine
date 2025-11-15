#include "pch.h"
#include "DeferredGeometryPass.h"

#include <scene/Scene3D.h>
#include <utils/loaders/ShaderLoader.h>
#include <platform/OpenGL/Framebuffers/GBuffer.h>
#include <graphics/renderer/renderpass/RenderPassType.h>
#include <graphics/Window.h>

namespace engine {

	DeferredGeometryPass::DeferredGeometryPass(Scene3D* scene) : RenderPass(scene, RenderPassType::GeometryPassType),
		m_AllocatedGBuffer(true),m_GBuffer(nullptr)
	{
		m_ModelShader = ShaderLoader::loadShader("Shaders/deferred/PBR_Model_GeometryPass.glsl");
		//m_SkinnedModelShader = ShaderLoader::loadShader("Shaders/deferred/PBR_Skinned_Model_GeometryPass.glsl");
		m_TerrainShader = ShaderLoader::loadShader("Shaders/deferred/PBR_Terrain_GeometryPass.glsl");

		m_GBuffer = new GBuffer(Window::getWidth(), Window::getHeight());
	}

	DeferredGeometryPass::DeferredGeometryPass(Scene3D* scene, GBuffer* customGBuffer) : RenderPass(scene, RenderPassType::GeometryPassType), m_AllocatedGBuffer(false), m_GBuffer(customGBuffer)
	{
		m_ModelShader = ShaderLoader::loadShader("Shaders/deferred/PBR_Model_GeometryPass.glsl");
		m_TerrainShader = ShaderLoader::loadShader("Shaders/deferred/PBR_Terrain_GeometryPass.glsl");
	}

	DeferredGeometryPass::~DeferredGeometryPass()
	{
		if (m_AllocatedGBuffer) {
			delete m_GBuffer;
		}
	}

	GeometryPassOutput DeferredGeometryPass::ExecuteGeometryPass(ICamera* camera, bool renderOnlyStatic)
	{
		glViewport(0, 0, m_GBuffer->getWidth(), m_GBuffer->getHeight());
		m_GLCache->setStencilWriteMask(0xFF);
		m_GBuffer->bind();
		m_GBuffer->clear();
		m_GLCache->setBlend(false);
		m_GLCache->setMultisample(false);
		
		// Setup initial stencil state
		m_GLCache->setStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		m_GLCache->setStencilWriteMask(0x00);
		m_GLCache->setStencilTest(true);

		m_GLCache->switchShader(m_ModelShader);
		m_ModelShader->setUniform("viewPos", camera->getPosition());
		m_ModelShader->setUniform("view", camera->getViewMatrix());
		m_ModelShader->setUniform("projection", camera->getProjectionMatrix());
	
		// Render opaque objects (use stencil to denote models for the deferred lighting pass)
		m_GLCache->setStencilWriteMask(0xFF);
		m_GLCache->setStencilFunc(GL_ALWAYS, StencilValue::ModelStencilValue, 0xFF);

		ModelRenderer* modelRenderer = m_ActiveScene->getModelRenderer();
		
		m_ActiveScene->addModelsToRenderer();

		modelRenderer->flushOpaque(m_ModelShader, m_RenderPassType);
		m_GLCache->setStencilWriteMask(0x00);

		Terrain* terrain = m_ActiveScene->getTerrain();
		if (terrain)
		{
			BEGIN_EVENT("Render Terrain");
			// Setup terrain information
			m_GLCache->switchShader(m_TerrainShader);
			m_TerrainShader->setUniform("view", camera->getViewMatrix());
			m_TerrainShader->setUniform("projection", camera->getProjectionMatrix());

			// Render the terrain (use stencil to denote the terrain for the deferred lighting pass)
			m_GLCache->setStencilWriteMask(0xFF);
			m_GLCache->setStencilFunc(GL_ALWAYS, StencilValue::TerrainStencilValue, 0xFF);
			terrain->Draw(m_TerrainShader, m_RenderPassType);
			m_GLCache->setStencilWriteMask(0x00);
			END_EVENT();
			
		}

		// Reset state
		m_GLCache->setStencilTest(false);

		// Render pass output
		GeometryPassOutput passOutput;
		passOutput.outputGBuffer = m_GBuffer;
		return passOutput;
	}

}
