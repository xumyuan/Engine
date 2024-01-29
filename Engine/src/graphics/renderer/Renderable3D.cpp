#include "Renderable3D.h"
#include "Renderer.h"

namespace engine {
	namespace graphics {
		Renderable3D::Renderable3D(
			glm::vec3& position,
			glm::vec3& scale,
			glm::vec3& rotationAxis,
			float radianRotation,
			Model* model, Renderable3D* parent, bool transparent)
			: m_Position(position), m_Scale(scale),
			m_Orientation(glm::angleAxis(radianRotation, rotationAxis)),
			m_Model(model), m_Parent(parent),
			m_Transparent(transparent)
		{

		}

		Renderable3D::Renderable3D(
			const glm::vec3& position,
			const glm::vec3& scale,
			const glm::vec3& rotationAxis,
			float radianRotation,
			Model* model, Renderable3D* parent, bool transparent)
			: m_Position(position), m_Scale(scale),
			m_Orientation(glm::angleAxis(radianRotation, rotationAxis)),
			m_Model(model), m_Parent(parent),
			m_Transparent(transparent)
		{

		}

		Renderable3D::~Renderable3D() {}

		void Renderable3D::draw(Shader& shader, RenderPass pass) const {
			m_Model->Draw(shader, pass);
		}
	}
}