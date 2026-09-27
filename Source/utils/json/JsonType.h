#pragma once
#include "pch.h"

namespace engine {
	struct SceneInfo
	{
		struct ModelInfo
		{
			std::string modelPath;
			glm::vec3 position;
			glm::vec3 scale;
			glm::vec3 rotationAxis;
			float radianRotation;
			bool isStatic;
			bool isTransparent;
			std::unordered_map<
				std::string,
				std::variant<std::string, glm::vec4, float, bool>
			> customMatTexList;
		};

		struct SkyboxInfo
		{
			std::vector<std::string> skyboxFilePaths;
		};

		struct LightsInfo
		{
			struct DirectionalLight
			{
				bool isActive;
				glm::vec3 direction;
				glm::vec3 lightColor;
			};

			struct SpotLight
			{
				bool isActive;
				glm::vec3 lightColor;
				glm::vec3 position;
				glm::vec3 direction;
				float cutOff;
				float outerCutOff;
			};

			struct PointLight
			{
				bool isActive;
				glm::vec3 position;
				glm::vec3 lightColor;
			};

			DirectionalLight directionalLight;
			SpotLight spotLight;
			std::vector<PointLight> pointLightList;
		};

		struct FluidInfo
		{
			bool isActive = false;
			size_t maxParticles = 0;
			// 模拟边界（世界坐标），粒子从边界盒底部一侧按溃坝方式初始化
			glm::vec3 boundaryMin{ 0.0f };
			glm::vec3 boundaryMax{ 0.0f };
			bool autoStart = true;
		};

		std::vector<ModelInfo> modelInfoList;
		SkyboxInfo skyboxInfo;
		LightsInfo lightsInfo;
		FluidInfo fluidInfo;
	};
}
