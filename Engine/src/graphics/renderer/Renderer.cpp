#include "Renderer.h"
namespace engine {
	namespace graphics {

		Renderer::Renderer(Camera* camera) : m_Camera(camera)
		{
			m_GLCache = GLCache::getInstance();
			m_GLCache->setDepthTest(true);
			m_GLCache->setBlend(false);
			m_GLCache->setCull(true);
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
		void Renderer::flushOpaque(Shader& shader, Shader& outlineShader) {
			m_GLCache->switchShader(shader.getShaderID());
			m_GLCache->setCull(true);
			m_GLCache->setDepthTest(true);
			m_GLCache->setBlend(false);
			m_GLCache->setStencilTest(true);
			m_GLCache->setStencilWriteMask(0xFF);

			//不透明物体渲染队列
			while (!m_OpaqueRenderQueue.empty()) {
				Renderable3D* current = m_OpaqueRenderQueue.front();

				m_GLCache->setStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
				m_GLCache->setStencilFunc(GL_ALWAYS, 1, 0xFF);
				if (current->getShouldOutline()) m_GLCache->setStencilWriteMask(0xFF);
				else m_GLCache->setStencilWriteMask(0x00);


				setupModelMatrix(current, shader);
				current->draw(shader);

				// 绘制外轮廓
				if (current->getShouldOutline()) {

					drawOutline(outlineShader, current);
					m_GLCache->switchShader(shader.getShaderID());
				}
				m_OpaqueRenderQueue.pop_front();
			}
		}

		// 透明物体渲染
		void Renderer::flushTransparent(Shader& shader, Shader& outlineShader) {
			// 关闭背面剔除，否则会导致透明物体的背面也被剔除
			m_GLCache->setCull(false);
			m_GLCache->setStencilTest(true);
			//排序后从后往前渲染，没有考虑缩放和旋转
			std::sort(m_TransparentRenderQueue.begin(), m_TransparentRenderQueue.end(), [this](Renderable3D* a, Renderable3D* b)->bool {
				return glm::length2(m_Camera->getPosition() - a->getPosition()) > glm::length2(m_Camera->getPosition() - b->getPosition());
				});

			//透明物体渲染
			while (!m_TransparentRenderQueue.empty()) {
				Renderable3D* current = m_TransparentRenderQueue.front();

				m_GLCache->setStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
				m_GLCache->setStencilFunc(GL_ALWAYS, 1, 0xFF);
				if (current->getShouldOutline()) m_GLCache->setStencilWriteMask(0xFF);
				else m_GLCache->setStencilWriteMask(0x00);

				//开启混合
				m_GLCache->setBlend(true);
				m_GLCache->setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				setupModelMatrix(current, shader);
				current->draw(shader);

				// 绘制外轮廓
				if (current->getShouldOutline()) {
					drawOutline(outlineShader, current);
					m_GLCache->switchShader(shader.getShaderID());
				}


				m_TransparentRenderQueue.pop_front();

			}
		}

		// TODO: Currently only support two levels in a hierarchical scene graph
		void Renderer::setupModelMatrix(Renderable3D* renderable, Shader& shader, float scaleFactor) {
			glm::mat4 model(1);
			glm::mat4 translate = glm::translate(glm::mat4(1.0f), renderable->getPosition());
			glm::mat4 rotate = glm::toMat4(renderable->getOrientation());
			glm::mat4 scale = glm::scale(glm::mat4(1.0f), renderable->getScale() * scaleFactor);

			if (renderable->getParent()) {
				// Only apply scale locally
				model = glm::translate(glm::mat4(1.0f), renderable->getParent()->getPosition()) * glm::toMat4(renderable->getParent()->getOrientation()) * translate * rotate * scale;
			}
			else {
				model = translate * rotate * scale;

			}

			shader.setUniformMat4("model", model);
		}

		// 绘制外轮廓
		void Renderer::drawOutline(Shader& outlineShader, Renderable3D* renderable) {
			m_GLCache->switchShader(outlineShader.getShaderID());
			m_GLCache->setStencilFunc(GL_NOTEQUAL, 1, 0xFF);

			setupModelMatrix(renderable, outlineShader, 1.025f);
			renderable->draw(outlineShader);

			m_GLCache->setDepthTest(true);
			glClear(GL_STENCIL_BUFFER_BIT);
		}
	}
}