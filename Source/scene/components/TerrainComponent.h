#pragma once

#include "scene/Component.h"
#include "terrain/Terrain.h"

namespace engine {

	/// @brief 地形组件 —— 让场景节点拥有地形
	/// 包装现有 Terrain 类，作为组件挂载到 SceneNode
	class TerrainComponent : public Component {
	public:
		/// @brief 构造地形组件
		/// @param terrain 地形指针（不接管所有权，Terrain 生命周期由 Scene3D 管理）
		TerrainComponent(Terrain* terrain)
			: Component(ComponentType::Terrain)
			, m_Terrain(terrain)
		{
		}

		~TerrainComponent() override = default;

		Terrain* getTerrain() const { return m_Terrain; }

	private:
		Terrain* m_Terrain = nullptr; // 非持有指针
	};

}
