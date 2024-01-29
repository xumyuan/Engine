#include "Renderer.h"
namespace engine {
	namespace graphics {

		Renderer::Renderer(Camera* camera) : m_Camera(camera)
		{
			m_GLCache = GLCache::getInstance();
			m_GLCache->setDepthTest(true);
			m_GLCache->setBlend(false);
			m_GLCache->setFaceCull(true);
		}

		//不透明渲染队列
		void Renderer::submitOpaque(Renderable3D* renderable) {
			m_OpaqueRenderQueue.push_back(renderable);
		}

		//透明物体渲染队列
		void Renderer::submitTransparent(Renderable3D* renderable) {
			m_TransparentRenderQueue.push_back(renderable);
		}

		// 不透明物渲染
		void Renderer::flushOpaque(Shader& shader, RenderPass pass) {
			m_GLCache->switchShader(shader.getShaderID());


			m_GLCache->setDepthTest(true);
			m_GLCache->setBlend(false);
			m_GLCache->setStencilTest(false);
			m_GLCache->setFaceCull(true);


			// 只绘制背面防止阴影失真
			if (pass == RenderPass::ShadowmapPass)
				m_GLCache->setCullFace(GL_FRONT);
			else
				m_GLCache->setCullFace(GL_BACK);

			//不透明物体渲染队列
			while (!m_OpaqueRenderQueue.empty()) {
				Renderable3D* current = m_OpaqueRenderQueue.front();

				setupModelMatrix(current, shader, pass);
				current->draw(shader, pass);

				m_OpaqueRenderQueue.pop_front();
			}
		}

		// 透明物体渲染
		void Renderer::flushTransparent(Shader& shader, RenderPass pass) {

			m_GLCache->switchShader(shader.getShaderID());
			m_GLCache->setDepthTest(true);
			m_GLCache->setBlend(true);
			m_GLCache->setStencilTest(false);

			// 只绘制背面防止阴影失真
			if (pass == RenderPass::ShadowmapPass)
				m_GLCache->setCullFace(GL_FRONT);
			else
				m_GLCache->setFaceCull(false);



			//排序后从后往前渲染，没有考虑缩放和旋转
			std::sort(m_TransparentRenderQueue.begin(), m_TransparentRenderQueue.end(),
				[this](Renderable3D* a, Renderable3D* b)->bool {
					return glm::length2(m_Camera->getPosition() - a->getPosition()) > glm::length2(m_Camera->getPosition() - b->getPosition());
				});

			//透明物体渲染
			while (!m_TransparentRenderQueue.empty()) {
				Renderable3D* current = m_TransparentRenderQueue.front();

				//开启混合
				m_GLCache->setBlend(true);
				m_GLCache->setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				setupModelMatrix(current, shader, pass);
				current->draw(shader, RenderPass::LightingPass);

				m_TransparentRenderQueue.pop_front();

			}
		}

		// TODO: Currently only support two levels in a hierarchical scene graph
		void Renderer::setupModelMatrix(Renderable3D* renderable, Shader& shader, RenderPass pass) {
			glm::mat4 model(1);
			glm::mat4 translate = glm::translate(glm::mat4(1.0f), renderable->getPosition());
			glm::mat4 rotate = glm::toMat4(renderable->getOrientation());
			glm::mat4 scale = glm::scale(glm::mat4(1.0f), renderable->getScale());

			if (renderable->getParent()) {
				// Only apply scale locally
				model = glm::translate(glm::mat4(1.0f), renderable->getParent()->getPosition()) * glm::toMat4(renderable->getParent()->getOrientation()) * translate * rotate * scale;
			}
			else {
				model = translate * rotate * scale;

			}

			shader.setUniformMat4("model", model);

			if (pass != RenderPass::ShadowmapPass) {
				glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
				shader.setUniformMat3("normalMatrix", normalMatrix);
			}
		}

	}
}