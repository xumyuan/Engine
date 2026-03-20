#pragma once

#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "scene/Component.h"

namespace engine {

	/// @brief 场景节点 —— 统一的场景实体表示
	/// 每个节点拥有 Transform（位置/缩放/旋转），支持父子层级关系，
	/// 通过挂载 Component 组件获得不同能力（渲染、灯光、天空盒等）
	class SceneNode {
	public:
		/// @brief 创建场景节点
		/// @param name 节点名称（用于调试和查找）
		explicit SceneNode(const std::string& name = "Node");
		~SceneNode();

		// ── Transform ──
		void setPosition(const glm::vec3& position) { m_Position = position; }
		void setScale(const glm::vec3& scale) { m_Scale = scale; }
		void setOrientation(const glm::quat& orientation) { m_Orientation = orientation; }
		void setOrientation(float radianRotation, const glm::vec3& rotationAxis) {
			m_Orientation = glm::angleAxis(radianRotation, rotationAxis);
		}

		const glm::vec3& getPosition() const { return m_Position; }
		const glm::vec3& getScale() const { return m_Scale; }
		const glm::quat& getOrientation() const { return m_Orientation; }
		const std::string& getName() const { return m_Name; }

		/// @brief 获取世界变换矩阵（考虑父节点）
		glm::mat4 getWorldTransform() const;

		// ── 父子关系 ──
		void addChild(SceneNode* child);
		void removeChild(SceneNode* child);
		SceneNode* getParent() const { return m_Parent; }
		const std::vector<SceneNode*>& getChildren() const { return m_Children; }

		// ── 组件管理 ──

		/// @brief 添加组件（转移所有权给节点）
		void addComponent(Component* component);

		/// @brief 获取指定类型的第一个组件
		template<typename T>
		T* getComponent() const {
			for (auto& comp : m_Components) {
				T* casted = dynamic_cast<T*>(comp);
				if (casted) return casted;
			}
			return nullptr;
		}

		/// @brief 获取指定 ComponentType 的所有组件
		std::vector<Component*> getComponents(ComponentType type) const;

		/// @brief 获取所有组件
		const std::vector<Component*>& getAllComponents() const { return m_Components; }

		/// @brief 按类型查询是否拥有某种组件
		bool hasComponent(ComponentType type) const;

		/// @brief 递归遍历：收集本节点及所有子节点中指定类型的组件
		template<typename T>
		void collectComponents(std::vector<T*>& outList) const {
			for (auto* comp : m_Components) {
				T* casted = dynamic_cast<T*>(comp);
				if (casted) outList.push_back(casted);
			}
			for (auto* child : m_Children) {
				child->collectComponents<T>(outList);
			}
		}

	private:
		std::string m_Name;

		// Transform
		glm::vec3 m_Position = glm::vec3(0.0f);
		glm::vec3 m_Scale = glm::vec3(1.0f);
		glm::quat m_Orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // 单位四元数

		// 层级关系
		SceneNode* m_Parent = nullptr;
		std::vector<SceneNode*> m_Children;

		// 组件列表（节点拥有所有权）
		std::vector<Component*> m_Components;
	};

}
