#include "Renderer.h"
namespace engine {
	namespace graphics {

		Renderer::Renderer(FPSCamera* camera) : m_Camera(camera)
		{
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
			//��͸��������Ⱦ����
			while (!m_OpaqueRenderQueue.empty()) {

				// Drawing prepration
				glEnable(GL_DEPTH_TEST);
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

				glStencilFunc(GL_ALWAYS, 1, 0xFF);
				glStencilMask(0xFF);

				Renderable3D* current = m_OpaqueRenderQueue.front();



				glm::mat4 model(1);
				model = glm::translate(model, current->getPosition());
				if ((current->getRotationAxis().x != 0 || current->getRotationAxis().y != 0 || current->getRotationAxis().z != 0) && current->getRadianRotation() != 0)
					model = glm::rotate(model, current->getRadianRotation(), current->getRotationAxis());
				model = glm::scale(model, current->getScale());
				shader.setUniformMat4("model", model);
				current->draw(shader);

				// ����������
				if (current->getShouldOutline()) {
					glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

					outlineShader.enable();
					model = glm::mat4(1);
					model = glm::translate(model, current->getPosition());
					if ((current->getRotationAxis().x != 0 || current->getRotationAxis().y != 0 || current->getRotationAxis().z != 0) && current->getRadianRotation() != 0)
						model = glm::rotate(model, current->getRadianRotation(), current->getRotationAxis());
					model = glm::scale(model, current->getScale() + glm::vec3(0.025f, 0.025f, 0.025f));
					outlineShader.setUniformMat4("model", model);
					current->draw(outlineShader);
					outlineShader.disable();

					glEnable(GL_DEPTH_TEST);
					glStencilMask(0xFF);

					shader.enable();

					glClear(GL_STENCIL_BUFFER_BIT);
				}
				m_OpaqueRenderQueue.pop_front();
			}
		}

		// ͸��������Ⱦ
		void Renderer::flushTransparent(Shader& shader, Shader& outlineShader) {

			// �رձ����޳�������ᵼ��͸������ı���Ҳ���޳�
			glDisable(GL_CULL_FACE);
			//�����Ӻ���ǰ��Ⱦ��û�п������ź���ת
			std::sort(m_TransparentRenderQueue.begin(), m_TransparentRenderQueue.end(), [this](Renderable3D* a, Renderable3D* b)->bool {
				return glm::length2(m_Camera->getPosition() - a->getPosition()) > glm::length2(m_Camera->getPosition() - b->getPosition());
				});


			//͸��������Ⱦ
			while (!m_TransparentRenderQueue.empty()) {
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

				glStencilFunc(GL_ALWAYS, 1, 0xFF);
				glStencilMask(0xFF);

				//�������
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


				auto* current = m_TransparentRenderQueue.front();

				glm::mat4 model(1);
				model = glm::translate(model, current->getPosition());
				if ((current->getRotationAxis().x != 0 || current->getRotationAxis().y != 0 || current->getRotationAxis().z != 0) && current->getRadianRotation() != 0)
					model = glm::rotate(model, current->getRadianRotation(), current->getRotationAxis());
				model = glm::scale(model, current->getScale());
				shader.setUniformMat4("model", model);
				current->draw(shader);

				// ����������
				if (current->getShouldOutline()) {
					glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

					outlineShader.enable();
					model = glm::mat4(1);
					model = glm::translate(model, current->getPosition());
					if ((current->getRotationAxis().x != 0 || current->getRotationAxis().y != 0 || current->getRotationAxis().z != 0) && current->getRadianRotation() != 0)
						model = glm::rotate(model, current->getRadianRotation(), current->getRotationAxis());
					model = glm::scale(model, current->getScale() + glm::vec3(0.025f, 0.025f, 0.025f));
					outlineShader.setUniformMat4("model", model);
					current->draw(outlineShader);
					outlineShader.disable();

					glEnable(GL_DEPTH_TEST);
					glStencilMask(0xFF);

					shader.enable();

					glClear(GL_STENCIL_BUFFER_BIT);
				}

				glDisable(GL_BLEND);

				m_TransparentRenderQueue.pop_front();

			}
		}
	}
}