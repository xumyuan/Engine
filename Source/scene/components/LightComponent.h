#pragma once

#include "scene/Component.h"
#include <glm/glm.hpp>

namespace engine {

	/// @brief 灯光类型
	enum class LightType {
		Directional,
		Point,
		Spot
	};

	/// @brief 灯光组件 —— 让场景节点成为光源
	/// 支持方向光、点光源、聚光灯三种灯光类型
	class LightComponent : public Component {
	public:
		LightComponent(LightType lightType = LightType::Point)
			: Component(ComponentType::Light)
			, m_LightType(lightType)
		{
		}

		~LightComponent() override = default;

		// ── 通用属性 ──
		LightType getLightType() const { return m_LightType; }
		const glm::vec3& getLightColor() const { return m_LightColor; }
		float getIntensity() const { return m_Intensity; }
		bool isActive() const { return m_IsActive; }

		void setLightColor(const glm::vec3& color) { m_LightColor = color; }
		void setIntensity(float intensity) { m_Intensity = intensity; }
		void setActive(bool active) { m_IsActive = active; }

		// ── 方向光属性 ──
		const glm::vec3& getDirection() const { return m_Direction; }
		void setDirection(const glm::vec3& dir) { m_Direction = dir; }

		// ── 聚光灯属性 ──
		float getCutOff() const { return m_CutOff; }
		float getOuterCutOff() const { return m_OuterCutOff; }
		void setCutOff(float cutOff) { m_CutOff = cutOff; }
		void setOuterCutOff(float outerCutOff) { m_OuterCutOff = outerCutOff; }

		// ── 点光源 / 聚光灯衰减 ──
		float getAttenuationRadius() const { return m_AttenuationRadius; }
		void setAttenuationRadius(float radius) { m_AttenuationRadius = radius; }

	private:
		LightType m_LightType = LightType::Point;
		glm::vec3 m_LightColor = glm::vec3(1.0f);
		glm::vec3 m_Direction = glm::vec3(0.0f, -1.0f, 0.0f);
		float m_Intensity = 1.0f;
		float m_CutOff = 0.0f;
		float m_OuterCutOff = 0.0f;
		float m_AttenuationRadius = 30.0f;
		bool m_IsActive = true;
	};

}
