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

		// 构建基础管线状态
		rhi::PipelineState pipeline;
		pipeline.program = m_ModelShader->getProgramHandle();
		pipeline.depthTest = true;
		pipeline.depthWrite = true;
		pipeline.blendEnable = false;
		pipeline.multisample = false;
		pipeline.cullMode = rhi::CullMode::Back;

		// 初始 stencil 设置：开启测试，设置 op 为 Keep/Keep/Replace，写 mask 0x00（暂不写入 stencil）
		rhi::StencilState baseStencil;
		baseStencil.stencilFail = rhi::StencilOp::Keep;
		baseStencil.depthFail = rhi::StencilOp::Keep;
		baseStencil.depthPass = rhi::StencilOp::Replace;
		baseStencil.writeMask = 0x00;
		baseStencil.readMask = 0xFF;
		baseStencil.func = rhi::CompareOp::Always;
		baseStencil.ref = 0;

		pipeline.stencilEnable = true;
		pipeline.stencilFront = baseStencil;
		pipeline.stencilBack = baseStencil;
		bindPipelineState(pipeline);

		m_ModelShader->setUniform("viewPos", camera->getPosition());
		m_ModelShader->setUniform("view", camera->getViewMatrix());
		m_ModelShader->setUniform("projection", camera->getProjectionMatrix());
	
		// Render opaque objects: 开启 stencil 写入，model stencil value
		pipeline.stencilFront.writeMask = 0xFF;
		pipeline.stencilFront.func = rhi::CompareOp::Always;
		pipeline.stencilFront.ref = StencilValue::ModelStencilValue;
		pipeline.stencilBack = pipeline.stencilFront;
		bindPipelineState(pipeline);

		ModelRenderer* modelRenderer = m_ActiveScene->getModelRenderer();
		
		m_ActiveScene->addModelsToRenderer();

		modelRenderer->flushOpaque(m_ModelShader, m_RenderPassType);

		// 关闭 stencil 写入
		pipeline.stencilFront.writeMask = 0x00;
		pipeline.stencilBack.writeMask = 0x00;
		bindPipelineState(pipeline);

		Terrain* terrain = m_ActiveScene->getTerrain();
		if (terrain)
		{
			BEGIN_EVENT("Render Terrain");
			// 切换 terrain shader
			pipeline.program = m_TerrainShader->getProgramHandle();
			bindPipelineState(pipeline);

			m_TerrainShader->setUniform("view", camera->getViewMatrix());
			m_TerrainShader->setUniform("projection", camera->getProjectionMatrix());

			// 开启 stencil 写入，terrain stencil value
			pipeline.stencilFront.writeMask = 0xFF;
			pipeline.stencilFront.func = rhi::CompareOp::Always;
			pipeline.stencilFront.ref = StencilValue::TerrainStencilValue;
			pipeline.stencilBack = pipeline.stencilFront;
			bindPipelineState(pipeline);

			terrain->Draw(m_TerrainShader, m_RenderPassType);

			// 关闭 stencil 写入
			pipeline.stencilFront.writeMask = 0x00;
			pipeline.stencilBack.writeMask = 0x00;
			bindPipelineState(pipeline);
			END_EVENT();
		}

		// 关闭 stencil 测试
		pipeline.stencilEnable = false;
		bindPipelineState(pipeline);

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
