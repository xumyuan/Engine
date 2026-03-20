#pragma once

#include <string>
#include <typeindex>

namespace engine {

	class SceneNode;

	/// @brief 组件类型枚举 —— 用于快速识别组件种类
	enum class ComponentType {
		Mesh,       // 网格渲染组件
		Light,      // 灯光组件
		Skybox,     // 天空盒组件
		Terrain,    // 地形组件
		Custom      // 自定义扩展组件
	};

	/// @brief 组件基类 —— 所有场景组件的抽象接口
	/// 组件附加到 SceneNode 上，通过组合方式赋予节点不同能力
	class Component {
	public:
		Component(ComponentType type) : m_Type(type), m_Owner(nullptr) {}
		virtual ~Component() = default;

		/// @brief 获取组件类型
		ComponentType getType() const { return m_Type; }

		/// @brief 获取所属节点
		SceneNode* getOwner() const { return m_Owner; }

		/// @brief 设置所属节点（由 SceneNode::addComponent 调用）
		void setOwner(SceneNode* owner) { m_Owner = owner; }

	private:
		ComponentType m_Type;
		SceneNode* m_Owner;
	};

}
