#include "pch.h"
#include "Skybox.h"

#include <utils/loaders/ShaderLoader.h>
#include "rhi/include/RHIContext.h"
#include "graphics/UniformBufferManager.h"
#include "graphics/UniformBufferData.h"

namespace engine {


	Skybox::Skybox(const std::vector<std::string>& filePaths) {
		m_Device = getRHIDevice();

		m_SkyboxShader = ShaderLoader::loadShader("Shaders/skybox.glsl");
		CubemapSettings srgbCubemap;
		srgbCubemap.IsSRGB = true;
		m_SkyboxCubemap = TextureLoader::loadCubemapTexture(filePaths[0], filePaths[1], filePaths[2], filePaths[3], filePaths[4], filePaths[5], &srgbCubemap);

		float skyboxVertices[] = {
			// Front
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			// Back
			-1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
		};
		unsigned int skyboxIndices[] = {
			// front
			2, 1, 0,
			0, 3, 2,
			// top
			6, 5, 1,
			1, 2, 6,
			// back
			5, 6, 7,
			7, 4, 5,
			// bottom
			3, 0, 4,
			4, 7, 3,
			// left
			1, 5, 4,
			4, 0, 1,
			// right
			6, 2, 3,
			3, 7, 6,
		};

		// ── 创建 Vertex Buffer ──
		rhi::BufferDesc vbDesc;
		vbDesc.usage = rhi::BufferUsage::Vertex;
		vbDesc.size = sizeof(skyboxVertices);
		m_VertexBuffer = m_Device->createBuffer(vbDesc);

		rhi::BufferDataDesc vbData;
		vbData.data = skyboxVertices;
		vbData.size = sizeof(skyboxVertices);
		vbData.offset = 0;
		m_Device->updateBuffer(m_VertexBuffer, vbData);

		// ── 创建 Index Buffer ──
		rhi::BufferDesc ibDesc;
		ibDesc.usage = rhi::BufferUsage::Index;
		ibDesc.size = sizeof(skyboxIndices);
		m_IndexBuffer = m_Device->createBuffer(ibDesc);

		rhi::BufferDataDesc ibData;
		ibData.data = skyboxIndices;
		ibData.size = sizeof(skyboxIndices);
		ibData.offset = 0;
		m_Device->updateBuffer(m_IndexBuffer, ibData);

		// ── 构建 VertexLayout ──
		rhi::VertexLayout layout = {};
		layout.attributes[0].bufferIndex = 0;
		layout.attributes[0].offset = 0;
		layout.attributes[0].type = rhi::VertexAttribType::Float3;
		layout.strides[0] = 3 * sizeof(float);
		layout.attributeCount = 1;
		layout.bufferCount = 1;

		// ── 创建 RenderPrimitive ──
		rhi::BufferHandle vbs[] = { m_VertexBuffer };
		m_RenderPrimitive = m_Device->createRenderPrimitive(
			layout, vbs, 1, m_IndexBuffer, rhi::IndexType::UInt32);
	}

	Skybox::~Skybox() {
		if (m_Device) {
			if (static_cast<bool>(m_RenderPrimitive))
				m_Device->destroyRenderPrimitive(m_RenderPrimitive);
			if (static_cast<bool>(m_VertexBuffer))
				m_Device->destroyBuffer(m_VertexBuffer);
			if (static_cast<bool>(m_IndexBuffer))
				m_Device->destroyBuffer(m_IndexBuffer);
		}
	}

	void Skybox::Draw(ICamera* camera) {
		// 通过 PipelineState 设置 skybox 渲染状态
		rhi::PipelineState pipeline;
		pipeline.program = m_SkyboxShader->getProgramHandle();
		pipeline.depthTest = true;
		pipeline.depthFunc = rhi::CompareOp::LessEqual;
		pipeline.depthWrite = false;  // skybox 不写深度
		pipeline.cullMode = rhi::CullMode::Back;
		m_Device->bindPipeline(pipeline);

		// Pass the texture to the shader
		m_SkyboxCubemap->bind(0);
		m_SkyboxShader->setUniform("skyboxCubemap", 0);

		// PerFrame UBO 更新（skybox 只需 view + projection）
		if (auto* uboMgr = getUBOManager()) {
			uboMgr->updatePerFrame(camera->getViewMatrix(), camera->getProjectionMatrix(),
				glm::vec3(0.0f));
			uboMgr->bindPerFrame();
		}

		m_Device->bindRenderPrimitive(m_RenderPrimitive);
		m_Device->draw(36, 0);

		// 恢复深度函数和深度写入
		rhi::PipelineState restorePipeline;
		restorePipeline.depthTest = true;
		restorePipeline.depthFunc = rhi::CompareOp::Less;
		restorePipeline.depthWrite = true;
		restorePipeline.cullMode = rhi::CullMode::Back;
		m_Device->bindPipeline(restorePipeline);

		m_SkyboxCubemap->unbind();
	}

	void Skybox::Draw(rhi::CommandBuffer& cmd, ICamera* camera) {
		rhi::PipelineState pipeline;
		pipeline.program = m_SkyboxShader->getProgramHandle();
		pipeline.depthTest = true;
		pipeline.depthFunc = rhi::CompareOp::LessEqual;
		pipeline.depthWrite = false;
		pipeline.cullMode = rhi::CullMode::Back;
		cmd.bindPipeline(pipeline);

		cmd.bindTextureUnit(m_SkyboxCubemap->getRHIHandle(), 0);
		cmd.setUniformInt(m_SkyboxShader->getProgramHandle(), "skyboxCubemap", 0);

		if (auto* uboMgr = getUBOManager()) {
			uboMgr->preparePerFrame(camera->getViewMatrix(), camera->getProjectionMatrix(),
				glm::vec3(0.0f));
			cmd.updateBuffer(uboMgr->getPerFrameHandle(),
				&uboMgr->getPerFrameData(), sizeof(UBOPerFrame));
			cmd.bindUBO(UBOBinding::PerFrame,
				uboMgr->getPerFrameHandle(), sizeof(UBOPerFrame));
		}

		cmd.bindRenderPrimitive(m_RenderPrimitive);
		cmd.draw(36, 0);

		rhi::PipelineState restorePipeline;
		restorePipeline.depthTest = true;
		restorePipeline.depthFunc = rhi::CompareOp::Less;
		restorePipeline.depthWrite = true;
		restorePipeline.cullMode = rhi::CullMode::Back;
		cmd.bindPipeline(restorePipeline);
	}

}
