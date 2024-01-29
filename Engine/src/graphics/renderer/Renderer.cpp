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

		//��͸����Ⱦ����
		void Renderer::submitOpaque(Renderable3D* renderable) {
			m_OpaqueRenderQueue.push_back(renderable);
		}

		//͸��������Ⱦ����
		void Renderer::submitTransparent(Renderable3D* renderable) {
			m_TransparentRenderQueue.push_back(renderable);
		}

		// ��͸������Ⱦ
		void Renderer::flushOpaque(Shader& shader, Shader& outlineShader) {
			m_GLCache->switchShader(shader.getShaderID());
			m_GLCache->setCull(true);
			m_GLCache->setDepthTest(true);
			m_GLCache->setBlend(false);
			m_GLCache->setStencilTest(true);
			m_GLCache->setStencilWriteMask(0xFF);

			//��͸��������Ⱦ����
			while (!m_OpaqueRenderQueue.empty()) {
				Renderable3D* current = m_OpaqueRenderQueue.front();

				m_GLCache->setStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
				m_GLCache->setStencilFunc(GL_ALWAYS, 1, 0xFF);
				if (current->getShouldOutline()) m_GLCache->setStencilWriteMask(0xFF);
				else m_GLCache->setStencilWriteMask(0x00);


				setupModelMatrix(current, shader);
				current->draw(shader);

				// ����������
				if (current->getShouldOutline()) {

					drawOutline(outlineShader, current);
					m_GLCache->switchShader(shader.getShaderID());
				}
				m_OpaqueRenderQueue.pop_front();
			}
		}

		// ͸��������Ⱦ
		void Renderer::flushTransparent(Shader& shader, Shader& outlineShader) {
			// �رձ����޳�������ᵼ��͸������ı���Ҳ���޳�
			m_GLCache->setCull(false);
			m_GLCache->setStencilTest(true);
			//�����Ӻ���ǰ��Ⱦ��û�п������ź���ת
			std::sort(m_TransparentRenderQueue.begin(), m_TransparentRenderQueue.end(), [this](Renderable3D* a, Renderable3D* b)->bool {
				return glm::length2(m_Camera->getPosition() - a->getPosition()) > glm::length2(m_Camera->getPosition() - b->getPosition());
				});

			//͸��������Ⱦ
			while (!m_TransparentRenderQueue.empty()) {
				Renderable3D* current = m_TransparentRenderQueue.front();

				m_GLCache->setStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
				m_GLCache->setStencilFunc(GL_ALWAYS, 1, 0xFF);
				if (current->getShouldOutline()) m_GLCache->setStencilWriteMask(0xFF);
				else m_GLCache->setStencilWriteMask(0x00);

				//�������
				m_GLCache->setBlend(true);
				m_GLCache->setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				setupModelMatrix(current, shader);
				current->draw(shader);

				// ����������
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

		// ����������
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