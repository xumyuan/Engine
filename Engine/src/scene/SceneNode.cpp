#include "pch.h"
#include "SceneNode.h"

namespace engine {
	namespace scene {

		SceneNode::SceneNode(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& rotationAxis, float radianRotation, graphics::Model* model, SceneNode* parent, bool transparent)
			: m_Position(position), m_Scale(scale), m_Orientation(glm::angleAxis(radianRotation, rotationAxis)), m_Model(model), m_Parent(parent), m_Transparent(transparent)
		{
		}

		SceneNode::~SceneNode() {
			// Recursive delete on children
			for (unsigned int i = 0; i < m_Children.size(); i++) {
				delete m_Children[i];
			}
		}

		void SceneNode::draw(graphics::Shader& shader, graphics::RenderPass pass) const {
			if (m_Model)
				m_Model->Draw(shader, pass);
		}

		void SceneNode::addChild(SceneNode* child) {
			if (child == this)
				return;

			m_Children.push_back(child);
			child->setParent(this);
		}

	}
}