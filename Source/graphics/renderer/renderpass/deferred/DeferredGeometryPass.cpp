#include "pch.h"
#include "DeferredGeometryPass.h"

#include <utils/loaders/ShaderLoader.h>
#include <graphics/renderer/renderpass/RenderPassType.h>
#include <graphics/Window.h>
#include <graphics/UniformBufferManager.h>

namespace engine {

	DeferredGeometryPass::DeferredGeometryPass(const RenderScene& renderScene) : RenderPass(renderScene, RenderPassType::GeometryPassType),
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
		// 通过命令缓冲录制 beginRenderPass
		rhi::RenderPassParams passParams;
		passParams.viewport = { 0, 0, m_GBufferRT.getWidth(), m_GBufferRT.getHeight() };
		passParams.clearColorFlag = true;
		passParams.clearDepthFlag = true;
		cmd().beginRenderPass(m_GBufferRT.getHandle(), passParams);

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
		cmd().bindPipeline(pipeline);

		// 更新并绑定 PerFrame UBO
		if (auto* uboMgr = getUBOManager()) {
			uboMgr->preparePerFrame(camera->getViewMatrix(), camera->getProjectionMatrix(),
				camera->getPosition());
			cmd().updateBuffer(uboMgr->getPerFrameHandle(),
				&uboMgr->getPerFrameData(), sizeof(UBOPerFrame));
			cmd().bindUBO(UBOBinding::PerFrame,
				uboMgr->getPerFrameHandle(), sizeof(UBOPerFrame));
		}
	
		// Render opaque objects: 开启 stencil 写入，model stencil value
		pipeline.stencilFront.writeMask = 0xFF;
		pipeline.stencilFront.func = rhi::CompareOp::Always;
		pipeline.stencilFront.ref = StencilValue::ModelStencilValue;
		pipeline.stencilBack = pipeline.stencilFront;
		cmd().bindPipeline(pipeline);

		rhi::ProgramHandle modelProgram = m_ModelShader->getProgramHandle();
		ModelRenderer* modelRenderer = m_RenderScene.modelRenderer;
		m_RenderScene.submitModelsToRenderer();

		modelRenderer->flushOpaque(cmd(), modelProgram, m_RenderPassType);

		// 关闭 stencil 写入
		pipeline.stencilFront.writeMask = 0x00;
		pipeline.stencilBack.writeMask = 0x00;
		cmd().bindPipeline(pipeline);

		Terrain* terrain = m_RenderScene.terrain;
		if (terrain)
		{
			cmd().pushDebugGroup("Render Terrain");
			// 切换 terrain shader
			pipeline.program = m_TerrainShader->getProgramHandle();
			cmd().bindPipeline(pipeline);

			// PerFrame UBO 已在上面更新绑定，terrain shader 共享同一 binding point

			// 开启 stencil 写入，terrain stencil value
			pipeline.stencilFront.writeMask = 0xFF;
			pipeline.stencilFront.func = rhi::CompareOp::Always;
			pipeline.stencilFront.ref = StencilValue::TerrainStencilValue;
			pipeline.stencilBack = pipeline.stencilFront;
			cmd().bindPipeline(pipeline);

			rhi::ProgramHandle terrainProgram = m_TerrainShader->getProgramHandle();
			terrain->Draw(cmd(), terrainProgram, m_RenderPassType);

			// 关闭 stencil 写入
			pipeline.stencilFront.writeMask = 0x00;
			pipeline.stencilBack.writeMask = 0x00;
			cmd().bindPipeline(pipeline);
			cmd().popDebugGroup();
		}

		// 关闭 stencil 测试
		pipeline.stencilEnable = false;
		cmd().bindPipeline(pipeline);

		// 通过命令缓冲录制 endRenderPass
		cmd().endRenderPass();

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
