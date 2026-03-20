#pragma once

#include "scene/Component.h"
#include "graphics/mesh/Model.h"
#include "graphics/renderer/renderpass/RenderPassType.h"

namespace engine {

	/// @brief 网格渲染组件 —— 让场景节点拥有可渲染的 3D 模型
	/// 替代原有的 RenderableModel，将渲染数据作为组件挂载到 SceneNode
	class MeshComponent : public Component {
	public:
		/// @brief 构造网格组件
		/// @param model 模型指针（组件接管所有权，析构时释放）
		/// @param isStatic 是否为静态物体
		/// @param isTransparent 是否为透明物体
		MeshComponent(Model* model, bool isStatic = false, bool isTransparent = false)
			: Component(ComponentType::Mesh)
			, m_Model(model)
			, m_IsStatic(isStatic)
			, m_IsTransparent(isTransparent)
		{
		}

		~MeshComponent() override {
			if (m_Model) {
				delete m_Model;
				m_Model = nullptr;
			}
		}

		/// @brief 绘制模型（假设 shader 已绑定）
		void draw(Shader* shader, RenderPassType pass) const {
			if (m_Model) {
				m_Model->Draw(shader, pass);
			}
		}

		// ── Getters ──
		Model* getModel() const { return m_Model; }
		bool isStatic() const { return m_IsStatic; }
		bool isTransparent() const { return m_IsTransparent; }

		// ── Setters ──
		void setTransparent(bool transparent) { m_IsTransparent = transparent; }
		void setStatic(bool isStatic) { m_IsStatic = isStatic; }

	private:
		Model* m_Model = nullptr;
		bool m_IsStatic = false;
		bool m_IsTransparent = false;
	};

}
