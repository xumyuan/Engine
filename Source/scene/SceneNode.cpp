#include "pch.h"
#include "SceneNode.h"

#include <glm/gtc/matrix_transform.hpp>

namespace engine {

	SceneNode::SceneNode(const std::string& name)
		: m_Name(name), m_Parent(nullptr)
	{
	}

	SceneNode::~SceneNode() {
		// 释放所有组件
		for (auto* comp : m_Components) {
			delete comp;
		}
		m_Components.clear();

		// 释放所有子节点（递归）
		for (auto* child : m_Children) {
			delete child;
		}
		m_Children.clear();
	}

	glm::mat4 SceneNode::getWorldTransform() const {
		glm::mat4 local = glm::mat4(1.0f);
		local = glm::translate(local, m_Position);
		local *= glm::mat4_cast(m_Orientation);
		local = glm::scale(local, m_Scale);

		if (m_Parent) {
			return m_Parent->getWorldTransform() * local;
		}
		return local;
	}

	void SceneNode::addChild(SceneNode* child) {
		if (!child || child == this) return;

		// 如果 child 已有父节点，先从旧父节点移除
		if (child->m_Parent) {
			child->m_Parent->removeChild(child);
		}

		child->m_Parent = this;
		m_Children.push_back(child);
	}

	void SceneNode::removeChild(SceneNode* child) {
		auto it = std::find(m_Children.begin(), m_Children.end(), child);
		if (it != m_Children.end()) {
			(*it)->m_Parent = nullptr;
			m_Children.erase(it);
		}
	}

	void SceneNode::addComponent(Component* component) {
		if (!component) return;
		component->setOwner(this);
		m_Components.push_back(component);
	}

	std::vector<Component*> SceneNode::getComponents(ComponentType type) const {
		std::vector<Component*> result;
		for (auto* comp : m_Components) {
			if (comp->getType() == type) {
				result.push_back(comp);
			}
		}
		return result;
	}

	bool SceneNode::hasComponent(ComponentType type) const {
		for (auto* comp : m_Components) {
			if (comp->getType() == type) return true;
		}
		return false;
	}

}
