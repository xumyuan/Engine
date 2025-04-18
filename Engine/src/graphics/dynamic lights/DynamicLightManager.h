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

		// Control functions
		inline void setSpotLightPosition(const glm::vec3& pos) { m_SpotLight.position = pos; }
		inline void setSpotLightDirection(const glm::vec3& dir) { m_SpotLight.direction = dir; }
		inline void setDirectionalLightDirection(const glm::vec3& dir) { m_DirectionalLight.direction = dir; }
		inline void setPointLightPosition(int index, const glm::vec3& pos) { m_PointLights[index].position = pos; }

		inline const glm::vec3& getDirectionalLightDirection() { return m_DirectionalLight.direction; }

		void init();

		DirectionalLight m_DirectionalLight;
		SpotLight m_SpotLight;
		std::vector<PointLight> m_PointLights;
	};

}