#pragma once
#include "../mesh/Model.h"
#include <glm/glm.hpp>
#include "RenderPass.h"

namespace engine {
	namespace graphics {

		class Renderable3D {
		public:
			Renderable3D(
				glm::vec3& position,
				glm::vec3& scale,
				glm::vec3& rotationAxis,
				float radianRotation,
				Model* model,
				Renderable3D* parent, bool shouldOutline = false, bool transparent = false);

			Renderable3D(
				const glm::vec3& position,
				const glm::vec3& scale,
				const glm::vec3& rotationAxis,
				float radianRotation,
				Model* model,
				Renderable3D* parent, bool shouldOutline = false, bool transparent = false);


			~Renderable3D();

			// TODO: 添加枚举来控制渲染通道，就像它是阴影贴图通道一样，以避免绑定等

			void draw(Shader& shader, RenderPass pass) const;

			inline const glm::vec3& getPosition() const { return m_Position; }
			inline const glm::vec3& getScale() const { return m_Scale; }
			inline const glm::quat& getOrientation() const { return m_Orientation; }
			inline const Renderable3D* getParent() const { return m_Parent; }
			inline bool getShouldOutline() const { return m_ShouldOutline; }
			inline bool getTransparent() const { return m_Transparent; }

			inline void setPosition(glm::vec3& other) { m_Position = other; }
			inline void setScale(glm::vec3& other) { m_Scale = other; }
			inline void setOrientation(float radianRotation, glm::vec3 rotationAxis) { m_Orientation = glm::angleAxis(radianRotation, rotationAxis); }

			inline void setShouldOutline(bool choice) { m_ShouldOutline = choice; }
			inline void setTransparent(bool choice) { m_Transparent = choice; }
		private:
			glm::vec3 m_Position, m_Scale;
			glm::quat m_Orientation;
			Renderable3D* m_Parent;

			// Graphic features
			bool m_ShouldOutline, m_Transparent;

			Model* m_Model;
		};
	}
}



