#include "pch.h"
#include "DeferredGeometryPass.h"

#include <scene/Scene3D.h>
#include <utils/loaders/ShaderLoader.h>
#include <graphics/renderer/renderpass/RenderPassType.h>
#include <graphics/Window.h>

namespace engine {

	DeferredGeometryPass::DeferredGeometryPass(Scene3D* scene) : RenderPass(scene, RenderPassType::GeometryPassType),
		m_GBufferRT(Window::getWidth(), Window::getHeight())
	{
		m_ModelShader = ShaderLoader::loadShader("Shaders/deferred/PBR_Model_GeometryPass.glsl");
		m_TerrainShader = ShaderLoader::loadShader("Shaders/deferred/PBR_Terrain_GeometryPass.glsl");

		initGBuffer();
	}

	void DeferredGeometryPass::initGBuffer() {
		uint32_t w = Window::getWidth();
		uint32_t h = Window::getHeight();

		auto makeGBufferSettings = [](rhi::TextureFormat fmt) {
			TextureSettings s;
			s.format = fmt;
			s.formatExplicitlySet = true;
			s.wrapS = rhi::WrapMode::ClampToEdge;
			s.wrapT = rhi::WrapMode::ClampToEdge;
			s.minFilter = rhi::FilterMode::Nearest;
			s.magFilter = rhi::FilterMode::Nearest;
			s.anisotropy = 1.0f;
			s.HasMips = false;
			return s;
		};

		// render target 1: RGBA8 (albedo)
		m_AlbedoTexture.setTextureSettings(makeGBufferSettings(rhi::TextureFormat::RGBA8));
		m_AlbedoTexture.generate2DTexture(w, h, ChannelLayout::RGBA);

		// render target 2: RGBA32F (normal)
		m_NormalTexture.setTextureSettings(makeGBufferSettings(rhi::TextureFormat::RGBA32F));
		m_NormalTexture.generate2DTexture(w, h, ChannelLayout::RGBA);

		// render target 3: RGBA8 (material info)
		m_MaterialInfoTexture.setTextureSettings(makeGBufferSettings(rhi::TextureFormat::RGBA8));
		m_MaterialInfoTexture.generate2DTexture(w, h, ChannelLayout::RGBA);

		// 构建 RenderTarget: 3个外部颜色附件 + 深度/模板纹理
		m_GBufferRT.addExternalColorTexture(&m_AlbedoTexture)
			.addExternalColorTexture(&m_NormalTexture)
			.addExternalColorTexture(&m_MaterialInfoTexture)
			.addDepthStencilTexture(DepthStencilFormat::DepthStencil)
			.build();
	}

	DeferredGeometryPass::~DeferredGeometryPass()
	{
	}

	GeometryPassOutput DeferredGeometryPass::ExecuteGeometryPass(ICamera* camera, bool renderOnlyStatic)
	{
		m_GBufferRT.beginPass();

		m_GLCache->setStencilWriteMask(0xFF);
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
	
		// Render opaque objects
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
			m_GLCache->switchShader(m_TerrainShader);
			m_TerrainShader->setUniform("view", camera->getViewMatrix());
			m_TerrainShader->setUniform("projection", camera->getProjectionMatrix());

			m_GLCache->setStencilWriteMask(0xFF);
			m_GLCache->setStencilFunc(GL_ALWAYS, StencilValue::TerrainStencilValue, 0xFF);
			terrain->Draw(m_TerrainShader, m_RenderPassType);
			m_GLCache->setStencilWriteMask(0x00);
			END_EVENT();
		}

		m_GLCache->setStencilTest(false);

		m_GBufferRT.endPass();

		// Render pass output
		GeometryPassOutput passOutput;
		passOutput.renderTarget = m_GBufferRT.getHandle();
		passOutput.albedoTexture = &m_AlbedoTexture;
		passOutput.normalTexture = &m_NormalTexture;
		passOutput.materialInfoTexture = &m_MaterialInfoTexture;
		passOutput.depthStencilTexture = m_GBufferRT.getDepthStencilTexture();
		passOutput.width = m_GBufferRT.getWidth();
		passOutput.height = m_GBufferRT.getHeight();
		return passOutput;
	}

}
