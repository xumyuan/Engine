#include "pch.h"
#include "ModelRenderer.h"
#include "graphics/UniformBufferManager.h"
#include "graphics/UniformBufferData.h"

namespace engine {

	Quad* ModelRenderer::NDC_Plane = nullptr;
	Cube* ModelRenderer::NDC_Cube = nullptr;

	ModelRenderer::ModelRenderer(FPSCamera* camera) :
		m_Camera(camera)
	{
		// 通过 PipelineState 设置初始渲染状态
		auto* device = getRHIDevice();
		if (device) {
			rhi::PipelineState initPipeline;
			initPipeline.depthTest = true;
			initPipeline.blendEnable = false;
			initPipeline.cullMode = rhi::CullMode::Back;
			device->bindPipeline(initPipeline);
		}

		NDC_Cube = new Cube();
		NDC_Plane = new Quad();
	}

	//不透明渲染队列
	void ModelRenderer::submitOpaque(RenderableModel* renderable) {
		m_OpaqueRenderQueue.push_back(renderable);
	}

	//透明物体渲染队列
	void ModelRenderer::submitTransparent(RenderableModel* renderable) {
		m_TransparentRenderQueue.push_back(renderable);
	}

	void ModelRenderer::drawNdcCube() {
		NDC_Cube->Draw();
	}
	void ModelRenderer::drawNdcPlane() {
		NDC_Plane->Draw();
	}

	// ===== 命令缓冲版本 =====

	void ModelRenderer::flushOpaque(rhi::CommandBuffer& cmd, rhi::ProgramHandle program, RenderPassType pass) {
		while (!m_OpaqueRenderQueue.empty()) {
			RenderableModel* current = m_OpaqueRenderQueue.front();
			setupModelMatrix(current, cmd, program, pass);
			current->draw(cmd, program, pass);
			m_OpaqueRenderQueue.pop_front();
		}
	}

	void ModelRenderer::flushTransparent(rhi::CommandBuffer& cmd, rhi::ProgramHandle program, RenderPassType pass) {
		std::sort(m_TransparentRenderQueue.begin(), m_TransparentRenderQueue.end(),
			[this](RenderableModel* a, RenderableModel* b)->bool {
				return glm::length2(m_Camera->getPosition() - a->getPosition()) > glm::length2(m_Camera->getPosition() - b->getPosition());
			});

		while (!m_TransparentRenderQueue.empty()) {
			RenderableModel* current = m_TransparentRenderQueue.front();
			setupModelMatrix(current, cmd, program, pass);
			current->draw(cmd, program, pass);
			m_TransparentRenderQueue.pop_front();
		}
	}

	void ModelRenderer::setupModelMatrix(RenderableModel* renderable, rhi::CommandBuffer& cmd, rhi::ProgramHandle program, RenderPassType pass) {
		glm::mat4 model(1);
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), renderable->getPosition());
		glm::mat4 rotate = glm::toMat4(renderable->getOrientation());
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), renderable->getScale());

		if (renderable->getParent()) {
			model = glm::translate(glm::mat4(1.0f), renderable->getParent()->getPosition()) * glm::toMat4(renderable->getParent()->getOrientation()) * translate * rotate * scale;
		}
		else {
			model = translate * rotate * scale;
		}

		auto* uboMgr = getUBOManager();
		if (uboMgr) {
			if (pass != RenderPassType::ShadowmapPassType) {
				glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
				uboMgr->preparePerObject(model, normalMatrix);
			} else {
				uboMgr->preparePerObject(model);
			}
			cmd.updateBuffer(uboMgr->getPerObjectHandle(),
				&uboMgr->getPerObjectData(), sizeof(UBOPerObject));
			cmd.bindUBO(UBOBinding::PerObject,
				uboMgr->getPerObjectHandle(), sizeof(UBOPerObject));
		}
	}

	void ModelRenderer::drawNdcCube(rhi::CommandBuffer& cmd) {
		NDC_Cube->Draw(cmd);
	}
	void ModelRenderer::drawNdcPlane(rhi::CommandBuffer& cmd) {
		NDC_Plane->Draw(cmd);
	}
}
