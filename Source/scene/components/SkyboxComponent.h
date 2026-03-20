#pragma once

#include "scene/Component.h"
#include "graphics/Skybox.h"

namespace engine {

	/// @brief 天空盒组件 —— 让场景拥有天空盒背景
	/// 每个场景通常只有一个天空盒组件
	class SkyboxComponent : public Component {
	public:
		/// @brief 构造天空盒组件
		/// @param skybox 天空盒指针（组件接管所有权）
		SkyboxComponent(Skybox* skybox)
			: Component(ComponentType::Skybox)
			, m_Skybox(skybox)
		{
		}

		~SkyboxComponent() override {
			if (m_Skybox) {
				delete m_Skybox;
				m_Skybox = nullptr;
			}
		}

		Skybox* getSkybox() const { return m_Skybox; }

	private:
		Skybox* m_Skybox = nullptr;
	};

}
