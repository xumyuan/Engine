#include "pch.h"

#include "DynamicLightManager.h"

namespace engine {

	// TODO: Add functionality so it can update with an entitie's position and orientation
	DynamicLightManager::DynamicLightManager()
		: m_DirectionalLight(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
		m_SpotLight(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f)
	{
		init();
	}

	void DynamicLightManager::init() {
		// Setup lighting configurations
		m_DirectionalLight.isActive = true;
		m_DirectionalLight.direction = glm::vec3(-0.25f, -1.0f, -0.25f);
		m_DirectionalLight.lightColor = glm::vec3(3.25f, 3.25f, 3.25f);

		m_SpotLight.isActive = true;
		m_SpotLight.lightColor = glm::vec3(100.0f, 100.0f, 100.0f);
		m_SpotLight.position = glm::vec3(0.0f, 0.0f, 0.0f);
		m_SpotLight.direction = glm::vec3(1.0f, 0.0f, 0.0f);
		m_SpotLight.cutOff = glm::cos(glm::radians(12.5f));
		m_SpotLight.outerCutOff = glm::cos(glm::radians(15.0f));

		PointLight pointLight1(glm::vec3(300.0f, 300.0f, 300.0f), glm::vec3(120.0f, 95.0f, 120.0f));
		pointLight1.isActive = true;
		//addPointLight(pointLight1);

		PointLight pointLight2(glm::vec3(800.0f, 200.0f, 0.0f), glm::vec3(120.0f, 77.0f, 100.0f));
		pointLight2.isActive = true;
		//addPointLight(pointLight2);
	}

	// TODO: Dynamically change the size of the lights (LIMIT OF 5 CURRENTLY FOR POINTLIGHTS)
	void DynamicLightManager::setupLightingUniforms(Shader* shader) {
		shader->setUniform("numPointLights", (int)m_PointLights.size());
		shader->setUniform("numDirPointSpotLights", glm::ivec4(1,2,1,0));

		m_DirectionalLight.setupUniforms(shader, 0);
		m_SpotLight.setupUniforms(shader, 0);
		int i = 0;
		for (auto iter = m_PointLights.begin(); iter != m_PointLights.end(); iter++, i++) {
			(*iter).setupUniforms(shader, i);
		}
	}

	void DynamicLightManager::addPointLight(PointLight& pointLight) {
		m_PointLights.push_back(pointLight);
	}
	
	void DynamicLightManager::addPointLight(const glm::vec3& position, const glm::vec3& lightColor) {
		PointLight pointLight(position, lightColor);
		pointLight.isActive = true;
		m_PointLights.push_back(pointLight);
	}

	void DynamicLightManager::fillLightingUBO(UBOLighting& ubo) {
		// 清零
		memset(&ubo, 0, sizeof(UBOLighting));

		int numDir = 0, numPoint = 0, numSpot = 0;

		// 方向光
		if (m_DirectionalLight.isActive && numDir < MAX_DIR_LIGHTS) {
			ubo.dirLights[numDir].direction = glm::vec4(m_DirectionalLight.direction, m_DirectionalLight.intensity);
			ubo.dirLights[numDir].lightColour = glm::vec4(m_DirectionalLight.lightColor, 0.0f);
			numDir++;
		}

		// 点光源
		for (size_t i = 0; i < m_PointLights.size() && numPoint < MAX_POINT_LIGHTS; ++i) {
			auto& pl = m_PointLights[i];
			if (pl.isActive) {
				ubo.pointLights[numPoint].position = glm::vec4(pl.position, pl.intensity);
				ubo.pointLights[numPoint].lightColour = glm::vec4(pl.lightColor, pl.attenuationRadius);
				numPoint++;
			}
		}

		// 聚光灯
		if (m_SpotLight.isActive && numSpot < MAX_SPOT_LIGHTS) {
			ubo.spotLights[numSpot].position = glm::vec4(m_SpotLight.position, m_SpotLight.intensity);
			ubo.spotLights[numSpot].direction = glm::vec4(m_SpotLight.direction, m_SpotLight.attenuationRadius);
			ubo.spotLights[numSpot].lightColour = glm::vec4(m_SpotLight.lightColor, m_SpotLight.cutOff);
			ubo.spotLights[numSpot].params = glm::vec4(m_SpotLight.outerCutOff, 0.0f, 0.0f, 0.0f);
			numSpot++;
		}

		ubo.numDirPointSpotLights = glm::ivec4(numDir, numPoint, numSpot, 0);
	}
}
