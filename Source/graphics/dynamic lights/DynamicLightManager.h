#pragma once

#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"

namespace engine {

	class DynamicLightManager {
	public:
		DynamicLightManager();

		void setupLightingUniforms(Shader* shader);
		void addPointLight(PointLight& pointLight);
		void addPointLight(const glm::vec3& position, const glm::vec3& lightColor);

		// Control functions
		inline void setSpotLightPosition(const glm::vec3& pos) { m_SpotLight.position = pos; }
		inline void setSpotLightDirection(const glm::vec3& dir) { m_SpotLight.direction = dir; }
		inline void setDirectionalLightDirection(const glm::vec3& dir) { m_DirectionalLight.direction = dir; }
		inline void setPointLightPosition(int index, const glm::vec3& pos) { m_PointLights[index].position = pos; }
		
		// 设置方向光
		inline void setDirectionalLight(const glm::vec3& direction, const glm::vec3& lightColor) {
			m_DirectionalLight.direction = direction;
			m_DirectionalLight.lightColor = lightColor;
			m_DirectionalLight.isActive = true;
		}
		
		// 设置聚光灯
		inline void setSpotLight(const glm::vec3& position, const glm::vec3& direction, 
		                         const glm::vec3& lightColor, float cutOff, float outerCutOff) {
			m_SpotLight.position = position;
			m_SpotLight.direction = direction;
			m_SpotLight.lightColor = lightColor;
			m_SpotLight.cutOff = glm::cos(glm::radians(cutOff)); // 转换为弧度的余弦值
			m_SpotLight.outerCutOff = glm::cos(glm::radians(outerCutOff)); // 转换为弧度的余弦值
			m_SpotLight.isActive = true;
		}

		inline const glm::vec3& getDirectionalLightDirection() { return m_DirectionalLight.direction; }

		void init();

		DirectionalLight m_DirectionalLight;
		SpotLight m_SpotLight;
		std::vector<PointLight> m_PointLights;
	};

}