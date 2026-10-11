#pragma once
#include "pch.h"

namespace engine {
	struct SceneInfo
	{
		int version = 1;

		struct CameraInfo
		{
			glm::vec3 position{ 105.716f, 136.20f, 98.0f };
			float yaw = 0.0f;
			float pitch = 0.0f;
		};

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
				bool isActive = false;
				glm::vec3 direction{ 0.0f, -1.0f, 0.0f };
				glm::vec3 lightColor{ 1.0f };
			};

			struct SpotLight
			{
				bool isActive = false;
				glm::vec3 lightColor{ 1.0f };
				glm::vec3 position{ 0.0f };
				glm::vec3 direction{ 0.0f, 0.0f, -1.0f };
				float cutOff = 12.5f;
				float outerCutOff = 15.0f;
			};

			struct PointLight
			{
				bool isActive = false;
				glm::vec3 position{ 0.0f };
				glm::vec3 lightColor{ 1.0f };
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
			std::string backend = "cpu";
		};

		struct EnvironmentInfo
		{
			SkyboxInfo skybox;
		};

		struct SystemsInfo
		{
			FluidInfo fluid;
		};

		CameraInfo camera;
		EnvironmentInfo environment;
		LightsInfo lighting;
		SystemsInfo systems;
		std::vector<ModelInfo> entities;
	};
}
