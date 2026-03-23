#include "pch.h"
#include "LightCollector.h"

#include <cstring>

namespace engine {

	void LightCollector::fillLightingUBO(SceneNode* rootNode, UBOLighting& ubo) {
		// 清零 UBO
		memset(&ubo, 0, sizeof(UBOLighting));

		if (!rootNode) return;

		// 从节点树收集所有灯光
		std::vector<std::pair<SceneNode*, LightComponent*>> lights;
		collectLights(rootNode, lights);

		int numDir = 0, numPoint = 0, numSpot = 0;

		for (auto& [node, light] : lights) {
			if (!light->isActive()) continue;

			switch (light->getLightType()) {
			case LightType::Directional:
				if (numDir < MAX_DIR_LIGHTS) {
					ubo.dirLights[numDir].direction = glm::vec4(light->getDirection(), light->getIntensity());
					ubo.dirLights[numDir].lightColour = glm::vec4(light->getLightColor(), 0.0f);
					numDir++;
				}
				break;

			case LightType::Point:
				if (numPoint < MAX_POINT_LIGHTS) {
					// 点光源位置从节点的 Transform 获取
					glm::vec3 position = node->getPosition();
					ubo.pointLights[numPoint].position = glm::vec4(position, light->getIntensity());
					ubo.pointLights[numPoint].lightColour = glm::vec4(light->getLightColor(), light->getAttenuationRadius());
					numPoint++;
				}
				break;

			case LightType::Spot:
				if (numSpot < MAX_SPOT_LIGHTS) {
					// 聚光灯位置从节点的 Transform 获取
					glm::vec3 position = node->getPosition();
					ubo.spotLights[numSpot].position = glm::vec4(position, light->getIntensity());
					ubo.spotLights[numSpot].direction = glm::vec4(light->getDirection(), light->getAttenuationRadius());
					ubo.spotLights[numSpot].lightColour = glm::vec4(light->getLightColor(), light->getCutOff());
					ubo.spotLights[numSpot].params = glm::vec4(light->getOuterCutOff(), 0.0f, 0.0f, 0.0f);
					numSpot++;
				}
				break;
			}
		}

		ubo.numDirPointSpotLights = glm::ivec4(numDir, numPoint, numSpot, 0);
	}

	glm::vec3 LightCollector::getDirectionalLightDirection(SceneNode* rootNode) const {
		if (!rootNode) return glm::vec3(-0.25f, -1.0f, -0.25f);

		std::vector<std::pair<SceneNode*, LightComponent*>> lights;
		collectLights(rootNode, lights);

		for (auto& [node, light] : lights) {
			if (light->isActive() && light->getLightType() == LightType::Directional) {
				return light->getDirection();
			}
		}

		// 如果没有激活的方向光，返回默认方向
		return glm::vec3(-0.25f, -1.0f, -0.25f);
	}

	SceneNode* LightCollector::findNodeByName(SceneNode* rootNode, const std::string& name) {
		if (!rootNode) return nullptr;

		if (rootNode->getName() == name) return rootNode;

		for (auto* child : rootNode->getChildren()) {
			SceneNode* found = findNodeByName(child, name);
			if (found) return found;
		}

		return nullptr;
	}

	void LightCollector::collectLights(SceneNode* node, std::vector<std::pair<SceneNode*, LightComponent*>>& outLights) const {
		if (!node) return;

		// 检查当前节点是否有 LightComponent
		auto* light = node->getComponent<LightComponent>();
		if (light) {
			outLights.emplace_back(node, light);
		}

		// 递归子节点
		for (auto* child : node->getChildren()) {
			collectLights(child, outLights);
		}
	}

}
