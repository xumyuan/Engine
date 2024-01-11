#include "Renderer.h"
namespace engine {
	namespace graphics {

		//将要渲染的物体加入渲染队列
		void Renderer::submit(Renderable3D* renderable) {
			m_RenderQueue.push_back(renderable);
		}

		//将渲染队列中的物体渲染出来
		void Renderer::flush(Shader& shader) {
			while (!m_RenderQueue.empty()) {
				Renderable3D* current = m_RenderQueue.front();

				glm::mat4 model(1);
				model = glm::translate(model, current->getPosition());
				if ((current->getRotationAxis().x != 0 || current->getRotationAxis().y != 0 || current->getRotationAxis().z != 0) && current->getRadianRotation() != 0)
					model = glm::rotate(model, current->getRadianRotation(), current->getRotationAxis());
				model = glm::scale(model, current->getScale());
				shader.setUniformMat4("model", model);
				m_RenderQueue.front()->draw(shader);
				m_RenderQueue.pop_front();
			}
		}
	}
}