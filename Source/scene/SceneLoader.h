#pragma once

#include <string>

namespace engine {

	class Scene3D;

	/// @brief 场景加载器 —— 负责从 JSON 文件解析场景数据并填充到 Scene3D
	/// 将加载职责从 Scene3D 中分离出来，遵循单一职责原则
	class SceneLoader {
	public:
		/// @brief 从 JSON 文件加载场景数据到目标 Scene3D 实例
		/// @param filePath JSON 场景文件路径
		/// @param scene 要填充的 Scene3D 实例
		static void loadFromFile(const std::string& filePath, Scene3D& scene);

	private:
		/// @brief 加载模型列表
		static void loadModels(Scene3D& scene, const struct SceneInfo& sceneInfo);

		/// @brief 加载天空盒
		static void loadSkybox(Scene3D& scene, const struct SceneInfo& sceneInfo);

		/// @brief 加载灯光（方向光、聚光灯、点光源）
		static void loadLights(Scene3D& scene, const struct SceneInfo& sceneInfo);
	};

}
