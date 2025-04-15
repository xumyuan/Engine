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

		std::vector<ModelInfo> modelInfoList;
		SkyboxInfo skyboxInfo;
	};
}
