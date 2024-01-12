#include "Renderer.h"
namespace engine {
	namespace graphics {

		Renderer::Renderer()
		{
		}

		//将要渲染的物体加入渲染队列
		void Renderer::submit(Renderable3D* renderable) {
			m_RenderQueue.push_back(renderable);
		}

		//将渲染队列中的物体渲染出来
		void Renderer::flush(Shader& shader, Shader& outlineShader) {
			while (!m_RenderQueue.empty()) {
				Renderable3D* current = m_RenderQueue.front();

				// Drawing prepration
				glEnable(GL_DEPTH_TEST);
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

				glStencilFunc(GL_ALWAYS, 1, 0xFF);
				glStencilMask(0xFF);

				glm::mat4 model(1);
				model = glm::translate(model, current->getPosition());
				if ((current->getRotationAxis().x != 0 || current->getRotationAxis().y != 0 || current->getRotationAxis().z != 0) && current->getRadianRotation() != 0)
					model = glm::rotate(model, current->getRadianRotation(), current->getRotationAxis());
				model = glm::scale(model, current->getScale());
				shader.setUniformMat4("model", model);
				current->draw(shader);

				// 绘制外轮廓
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
				}
				glClear(GL_STENCIL_BUFFER_BIT);
				m_RenderQueue.pop_front();
			}
		}
	}
}